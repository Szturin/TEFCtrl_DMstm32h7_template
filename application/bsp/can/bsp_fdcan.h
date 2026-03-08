#ifndef __BSP_FDCAN_H__
#define __BSP_FDCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fdcan.h"

#define hcan_t FDCAN_HandleTypeDef

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

/* 常用宏定义 */
#define FDCAN_MX_REGISTER_CNT 28      // 最大注册实例数量
#define FDCAN_MAX_DATA_LEN    64      // FDCAN最大载荷长度

void bsp_can_init(void);
void can_filter_init(void);
void bsp_fdcan_set_baud(hcan_t *hfdcan, uint8_t mode, uint8_t baud);
uint8_t fdcanx_send_data(hcan_t *hfdcan, uint16_t id, uint8_t *data, uint32_t len);
uint8_t fdcanx_receive(hcan_t *hfdcan, uint16_t *rec_id, uint8_t *buf);
void fdcan1_rx_callback(void);
void fdcan2_rx_callback(void);
void fdcan3_rx_callback(void);

/*C语言下CAN总线面向对象设计*/
typedef struct _FDCANInstance{
    FDCAN_HandleTypeDef *fdcan_handle;
    FDCAN_TxHeaderTypeDef txconf;
    uint32_t tx_id;
    uint8_t tx_buff[64];
    uint8_t rx_buff[64];
    uint32_t rx_id;
    uint8_t rx_len;
    void (*fdcan_module_callback)(struct _FDCANInstance*);
    void *id;
} FDCANInstance;

typedef struct {
    FDCAN_HandleTypeDef *fdcan_handle;
    uint32_t tx_id;
    uint32_t rx_id;
    uint8_t  data_len;
    void (*fdcan_module_callback)(FDCANInstance *);
    void *id;
} FDCAN_Init_Config_s;

FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config);
uint8_t FDCANTransmit(FDCANInstance *_instance, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FDCAN_H_ */

