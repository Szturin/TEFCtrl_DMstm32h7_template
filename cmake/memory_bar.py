# -*- coding: utf-8 -*-
"""
memory_bar.py  -- show memory usage as progress bars after build
Usage: python memory_bar.py <elf_file>
"""
import sys, subprocess, os, io

# Windows UTF-8 terminal
if sys.platform == "win32":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    try:
        import ctypes
        ctypes.windll.kernel32.SetConsoleOutputCP(65001)
    except Exception:
        pass

BAR_WIDTH = 28

# STM32H723 memory map
REGIONS = [
    ("FLASH",   0x08000000, 0x08000000 + 1024*1024),
    ("DTCMRAM", 0x20000000, 0x20000000 + 128*1024),
    ("RAM_D1",  0x24000000, 0x24000000 + 128*1024),
    ("RAM_D2",  0x30000000, 0x30000000 + 32*1024),
    ("RAM_D3",  0x38000000, 0x38000000 + 16*1024),
    ("ITCMRAM", 0x00000004, 0x00000004 + 64*1024),  # start at 4 避开 NULL 地址
]

REGION_SIZE = {
    "FLASH":   1024*1024,
    "DTCMRAM": 128*1024,
    "RAM_D1":  128*1024,
    "RAM_D2":  32*1024,
    "RAM_D3":  16*1024,
    "ITCMRAM": 64*1024,
}

def color_bar(used, total, width=BAR_WIDTH):
    ratio = min(used / total, 1.0) if total else 0
    filled = int(ratio * width)
    if ratio > 0.90:   c = "\033[91m"
    elif ratio > 0.70: c = "\033[93m"
    else:              c = "\033[92m"
    r = "\033[0m"
    return f"[{c}{'█'*filled}{r}{'░'*(width-filled)}]"

def human(n):
    if n >= 1024*1024: return f"{n/1024/1024:.1f} MB"
    if n >= 1024:      return f"{n/1024:.1f} KB"
    return f"{n} B"

def parse_sections(elf):
    """
    解析 objdump -h 输出，返回 list of (name, vma, size)。
    只保留有内容且 size>0 的 ALLOC section（过滤 debug/metadata）。
    """
    try:
        raw = subprocess.check_output(
            ["arm-none-eabi-objdump", "-h", elf],
            stderr=subprocess.DEVNULL, text=True)
    except Exception:
        return []

    result = []
    lines = raw.splitlines()
    for i, line in enumerate(lines):
        parts = line.split()
        if not parts or not parts[0].isdigit():
            continue
        if len(parts) < 4:
            continue
        try:
            name = parts[1]
            size = int(parts[2], 16)
            vma  = int(parts[3], 16)
        except ValueError:
            continue

        if size == 0:
            continue

        # 检查下一行是否含 ALLOC（排除纯调试 section）
        flags_line = lines[i+1] if i+1 < len(lines) else ""
        if "ALLOC" not in flags_line:
            continue

        result.append((name, vma, size))
    return result

def calc_usage(sections):
    usage = {name: 0 for name, _, _ in REGIONS}
    for sec_name, vma, size in sections:
        for reg_name, start, end in REGIONS:
            if start <= vma < end:
                usage[reg_name] += size
                break
    return usage

def main():
    if len(sys.argv) < 2:
        print("Usage: python memory_bar.py <elf_file>")
        sys.exit(1)

    elf = sys.argv[1]
    if not os.path.exists(elf):
        print(f"File not found: {elf}")
        sys.exit(1)

    sections = parse_sections(elf)
    usage    = calc_usage(sections)

    # arm-none-eabi-size 获取 text/data/bss
    text_kb = data_b = bss_b = 0
    try:
        out = subprocess.check_output(
            ["arm-none-eabi-size", elf],
            stderr=subprocess.DEVNULL, text=True).strip().splitlines()
        if len(out) >= 2:
            v = out[1].split()
            text_kb, data_b, bss_b = int(v[0]), int(v[1]), int(v[2])
    except Exception:
        pass

    print()
    print("  ╔══════════════════════════════════════════════════════╗")
    print("  ║              Memory Usage  (STM32H723)              ║")
    print("  ╠══════════════════════════════════════════════════════╣")

    for reg_name, _, _ in REGIONS:
        used  = usage[reg_name]
        total = REGION_SIZE[reg_name]
        pct   = used / total * 100 if total else 0
        b     = color_bar(used, total)
        print(f"  ║  {reg_name:<8} {b}  {human(used):>8} / {human(total):<7}  {pct:5.1f}%  ║")

    print("  ╠══════════════════════════════════════════════════════╣")
    flash_total = text_kb + data_b
    ram_total   = data_b  + bss_b
    print(f"  ║  .text {human(text_kb):>7}  .data {human(data_b):>6}  .bss {human(bss_b):>7}       ║")
    print(f"  ║  Flash load: {human(flash_total):<10}  RAM load: {human(ram_total):<10}   ║")
    print("  ╚══════════════════════════════════════════════════════╝")
    print()

if __name__ == "__main__":
    main()
