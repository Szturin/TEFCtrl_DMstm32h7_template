"""
sync_keil.py - 自动同步 application/ 目录文件到 Keil .uvprojx

用法：在项目根目录执行
    python sync_keil.py

效果：扫描 application/bsp、application/modules、application/task 三个目录，
      自动更新 MDK-ARM/*.uvprojx 中 user/bsp、user/modules、user/task 三个 Group 的文件列表。
      新增文件后运行一次即可，无需手动在 Keil GUI 中添加。
"""

import os
import re

# ==============================================================================
# 配置：目录 → Keil Group 名 的映射
# ==============================================================================
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
UVPROJX = os.path.join(PROJECT_ROOT, "MDK-ARM", "TEFCtrl_DMstm32H7_template.uvprojx")

# 扫描目录（相对于项目根目录） → 对应的 Keil Group 名
SYNC_GROUPS = {
    "application/bsp":  "user/bsp",
    "application/modules":  "user/modules",
    "application/task": "user/task",
}

# 文件类型映射（Keil FileType）
FILE_TYPE = {
    ".c":   "1",
    ".s":   "2",
    ".asm": "2",
    ".cpp": "8",
    ".cxx": "8",
    ".h":   "5",
    ".hpp": "5",
}

# 忽略的文件/目录
IGNORE_DIRS  = {"__pycache__", ".git"}
IGNORE_FILES = {".gitkeep", ".DS_Store"}
IGNORE_EXTS  = {".md", ".txt", ".py", ".json", ".map", ".o", ".d"}

# ==============================================================================

def scan_dir(base_dir):
    """递归扫描目录，返回 (文件名, 相对于项目根目录的路径) 列表，按路径排序"""
    result = []
    for root, dirs, files in os.walk(base_dir):
        # 过滤忽略目录
        dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
        for fname in sorted(files):
            ext = os.path.splitext(fname)[1].lower()
            if ext in IGNORE_EXTS or fname in IGNORE_FILES:
                continue
            if ext not in FILE_TYPE:
                continue
            full_path = os.path.join(root, fname)
            rel_path  = os.path.relpath(full_path, PROJECT_ROOT)
            # 转为 Keil 使用的反斜杠格式，加 ..\  前缀（相对 MDK-ARM 目录）
            keil_path = "..\\" + rel_path.replace("/", "\\")
            result.append((fname, keil_path, FILE_TYPE[ext]))
    return result


def build_files_xml(files):
    """生成 <Files>...</Files> XML 片段"""
    lines = ["          <Files>"]
    for fname, keil_path, ftype in files:
        lines += [
            "            <File>",
            f"              <FileName>{fname}</FileName>",
            f"              <FileType>{ftype}</FileType>",
            f"              <FilePath>{keil_path}</FilePath>",
            "            </File>",
        ]
    lines.append("          </Files>")
    return "\n".join(lines)


def replace_group_files(content, group_name, new_files_xml):
    """替换指定 Group 的 <Files> 内容"""
    pattern = (
        r'(<GroupName>' + re.escape(group_name) + r'</GroupName>\s*)'
        r'<Files>.*?</Files>'
    )
    # 用 lambda 避免 replacement 字符串中反斜杠被 re 解释为转义
    def repl(m):
        return m.group(1) + new_files_xml
    new_content, count = re.subn(pattern, repl, content, flags=re.DOTALL)
    return new_content, count


DSP_SRC_XML = """\
        <Group>
          <GroupName>lib</GroupName>
          <Files>
            <File>
              <FileName>MatrixFunctions.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Middlewares/ST/ARM/DSP/Source/MatrixFunctions/MatrixFunctions.c</FilePath>
            </File>
            <File>
              <FileName>FastMathFunctions.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Middlewares/ST/ARM/DSP/Source/FastMathFunctions/FastMathFunctions.c</FilePath>
            </File>
            <File>
              <FileName>CommonTables.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Middlewares/ST/ARM/DSP/Source/CommonTables/CommonTables.c</FilePath>
            </File>
          </Files>
        </Group>"""


def fix_linker(content):
    """
    CubeMX 重新生成 uvprojx 后可能清空 lib Group，此函数恢复。
    AC6 (ARMCLANG) 无法链接 AC5 编译的预编译 .lib，改为直接编译
    DSP 矩阵函数源码（MatrixFunctions.c）。
    """
    # 清空 IncludeLibs / IncludeLibsPath（已不需要预编译 .lib）
    def clear_tag(tag, text):
        return re.sub(rf'<{tag}>[^<]*</{tag}>', f'<{tag}></{tag}>', text)

    content = clear_tag("IncludeLibs",     content)
    content = clear_tag("IncludeLibsPath", content)

    # 确保 lib Group 存在（以 GroupName>lib< 判断）
    if "<GroupName>lib</GroupName>" not in content:
        content = content.replace(
            "      </Groups>",
            DSP_SRC_XML + "\n      </Groups>"
        )
        print("[修复] lib Group 已添加（DSP MatrixFunctions.c 源码编译）")
    else:
        print("[确认] lib Group 已存在，无需修复")

    return content


def main():
    if not os.path.exists(UVPROJX):
        print(f"[错误] 找不到 uvprojx 文件：{UVPROJX}")
        return

    with open(UVPROJX, "r", encoding="utf-8") as f:
        content = f.read()

    total_added = 0
    for rel_dir, group_name in SYNC_GROUPS.items():
        abs_dir = os.path.join(PROJECT_ROOT, rel_dir.replace("/", os.sep))
        if not os.path.exists(abs_dir):
            print(f"[跳过] 目录不存在：{rel_dir}")
            continue

        files = scan_dir(abs_dir)
        new_files_xml = build_files_xml(files)
        content, count = replace_group_files(content, group_name, new_files_xml)

        if count:
            print(f"[更新] {group_name:15s} ← {len(files):2d} 个文件  ({rel_dir})")
            total_added += len(files)
        else:
            print(f"[警告] Group '{group_name}' 在 uvprojx 中未找到，跳过")

    content = fix_linker(content)

    with open(UVPROJX, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"\n完成，共同步 {total_added} 个文件 → {os.path.basename(UVPROJX)}")


if __name__ == "__main__":
    main()
