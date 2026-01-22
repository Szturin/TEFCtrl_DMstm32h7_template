#ifndef __BSP_FDCAN_H__
#define __BSP_FDCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fdcan.h"

#define hcan_t FDCAN_HandleTypeDef

/* 常用宏定义 */
#define FDCAN_MX_REGISTER_CNT 28      // 最大注册实例数量
#define FDCAN_MAX_DATA_LEN    64      // FDCAN最大载荷长度

/* 波特率模式与速率宏 */
#define CAN_CLASS   0
#define CAN_FD_BRS  1

#define CAN_BR_125K 0
#define CAN_BR_200K 1
#define CAN_BR_250K 2
#define CAN_BR_500K 3
#define CAN_BR_1M   4
#define CAN_BR_2M   5
#define CAN_BR_2M5  6
#define CAN_BR_3M2  7
#define CAN_BR_4M   8
#define CAN_BR_5M   9

void bsp_can_init(void);
void can_filter_init(void);
void bsp_fdcan_set_baud(hcan_t *hfdcan, uint8_t mode, uint8_t baud);
uint8_t fdcanx_send_data(hcan_t *hfdcan, uint16_t id, uint8_t *data, uint32_t len);
uint8_t fdcanx_receive(hcan_t *hfdcan, uint16_t *rec_id, uint8_t *buf);
void fdcan1_rx_callback(void);
void fdcan2_rx_callback(void);
void fdcan3_rx_callback(void);


/*C语言下CAN总线面向对象设计*/
//@todo 后期改为纯C++实现

typedef struct _FDCANInstance{
    FDCAN_HandleTypeDef *fdcan_handle; //can硬件句柄
    FDCAN_TxHeaderTypeDef txconf; // 发送配置
    uint32_t tx_id;             // 发送ID
    uint8_t tx_buff[64]; // 发送缓冲区，FDCAN最大64字节
    uint8_t rx_buff[64]; // 接受缓冲区
    uint32_t rx_id; // 指定接受的报文id
    uint8_t rx_len;// 实际接收的数据长度s

    //接受回调，传入实例指针
    void (*fdcan_module_callback)(struct _FDCANInstance*);
    void *id; // 归属模块指针(如点击结构体地址)
}FDCANInstance; // 一个CAN实例对应一个CAN设备,CAN实例是CAN设备的“通信属性”

//CAN实例初始化配置结构体，需要我们外部手动设置成员属性（public），其余为实例的私有
typedef struct {
    FDCAN_HandleTypeDef *fdcan_handle; //can硬件句柄
    uint32_t tx_id; // 发送ID
    uint32_t rx_id;
    uint8_t  data_len;                 // 默认发送长度
    void (*fdcan_module_callback)(FDCANInstance *);
    void *id;
} FDCAN_Init_Config_s;

// 外部调用接口，注册 ->发送 / 设置总线波特率
FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config);
uint8_t FDCANTransmit(FDCANInstance *_instance, uint16_t len);
void bsp_fdcan_set_baud(FDCAN_HandleTypeDef *hfdcan, uint8_t mode, uint8_t baud);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FDCAN_H_ */

