#include "bsp_fdcan.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * FDCANInstance 静态存储 + DLC 辅助函数
 * ============================================================ */
static FDCANInstance *fdcan_instance[FDCAN_MX_REGISTER_CNT] = {NULL};
static uint8_t fdcan_idx = 0;

static uint32_t ByteToDLC(uint8_t len) {
    if (len <= 8)  return (uint32_t)len;   // HAL 期望原始 DLC 值，不需要左移！
    if (len <= 12) return FDCAN_DLC_BYTES_12;
    if (len <= 16) return FDCAN_DLC_BYTES_16;
    if (len <= 20) return FDCAN_DLC_BYTES_20;
    if (len <= 24) return FDCAN_DLC_BYTES_24;
    if (len <= 32) return FDCAN_DLC_BYTES_32;
    if (len <= 48) return FDCAN_DLC_BYTES_48;
    return FDCAN_DLC_BYTES_64;
}

static uint8_t DLCToByte(uint32_t dlc) {
    /* 标准 CAN: HAL 直接返回字节数 (0~8) */
    if (dlc <= 8) return (uint8_t)dlc;
    /* FDCAN 格式: DLC 编码在 bit[19:16] */
    if (dlc <= FDCAN_DLC_BYTES_8) return dlc >> 16;
    switch (dlc) {
    case FDCAN_DLC_BYTES_12: return 12;
    case FDCAN_DLC_BYTES_16: return 16;
    case FDCAN_DLC_BYTES_20: return 20;
    case FDCAN_DLC_BYTES_24: return 24;
    case FDCAN_DLC_BYTES_32: return 32;
    case FDCAN_DLC_BYTES_48: return 48;
    case FDCAN_DLC_BYTES_64: return 64;
    default: return 8;
    }
}

/* ============================================================
 * 基础 CAN 初始化
 * ============================================================ */
void can_filter_init(void)
{
	FDCAN_FilterTypeDef fdcan_filter;

	fdcan_filter.IdType = FDCAN_STANDARD_ID;
	fdcan_filter.FilterIndex = 0;
	fdcan_filter.FilterType = FDCAN_FILTER_MASK;
	fdcan_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	fdcan_filter.FilterID1 = 0x00;
	fdcan_filter.FilterID2 = 0x00;

	HAL_FDCAN_ConfigFilter(&hfdcan1, &fdcan_filter);
	HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
	HAL_FDCAN_ConfigFifoWatermark(&hfdcan1, FDCAN_CFG_RX_FIFO0, 1);
}

void bsp_can_init(void)
{
	can_filter_init();
	HAL_FDCAN_Start(&hfdcan1);
	HAL_FDCAN_ActivateNotification(&hfdcan1,
	    FDCAN_IT_RX_FIFO0_WATERMARK
	    | FDCAN_IT_TX_COMPLETE | FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_BUS_OFF
	    | FDCAN_IT_ARB_PROTOCOL_ERROR | FDCAN_IT_DATA_PROTOCOL_ERROR
	    | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING,
	    0x00000F00);
}

void bsp_fdcan_set_baud(hcan_t *hfdcan, uint8_t mode, uint8_t baud)
{
	uint32_t nom_brp=0, nom_seg1=0, nom_seg2=0, nom_sjw=0;
	uint32_t dat_brp=0, dat_seg1=0, dat_seg2=0, dat_sjw=0;

	/*	nominal_baud = 80M/brp/(1+seg1+seg2)
		sample point = (1+seg1)/(1+seg1+sjw)  */
	if(mode == CAN_CLASS)
	{
		switch (baud)
		{
			case CAN_BR_125K: 	nom_brp=4 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break; // sample point 87.5%
			case CAN_BR_200K: 	nom_brp=2 ; nom_seg1=174; nom_seg2=25; nom_sjw=25; break; // sample point 87.5%
			case CAN_BR_250K: 	nom_brp=2 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break; // sample point 87.5%
			case CAN_BR_500K: 	nom_brp=1 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break; // sample point 87.5%
			case CAN_BR_1M:		nom_brp=1 ; nom_seg1=59 ; nom_seg2=20; nom_sjw=20; break; // sample point 75%
		}
		dat_brp=1 ; dat_seg1=29; dat_seg2=10; dat_sjw=10;	// 仲裁域默认1M
		hfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
	}
	/*	data_baud	 = 80M/brp/(1+seg1+seg2)
		sample point = (1+seg1)/(1+seg1+sjw)  */
	if(mode == CAN_FD_BRS)
	{
		switch (baud)
		{
			case CAN_BR_2M: 	dat_brp=1 ; dat_seg1=29; dat_seg2=10; dat_sjw=10; break;	// sample point 75%
			case CAN_BR_2M5: 	dat_brp=1 ; dat_seg1=25; dat_seg2=6 ; dat_sjw=6 ; break;	// sample point 81.25%
			case CAN_BR_3M2: 	dat_brp=1 ; dat_seg1=19; dat_seg2=5 ; dat_sjw=5 ; break;	// sample point 80%
			case CAN_BR_4M: 	dat_brp=1 ; dat_seg1=14; dat_seg2=5 ; dat_sjw=5 ; break;	// sample point 75%
			case CAN_BR_5M:		dat_brp=1 ; dat_seg1=13; dat_seg2=2 ; dat_sjw=2 ; break;	// sample point 87.5%
		}
		nom_brp=1 ; nom_seg1=59 ; nom_seg2=20; nom_sjw=20; // 数据域默认1M
		hfdcan->Init.FrameFormat = FDCAN_FRAME_FD_BRS;
	}

	HAL_FDCAN_DeInit(hfdcan);

	hfdcan->Init.NominalPrescaler = nom_brp;
	hfdcan->Init.NominalTimeSeg1  = nom_seg1;
	hfdcan->Init.NominalTimeSeg2  = nom_seg2;
	hfdcan->Init.NominalSyncJumpWidth = nom_sjw;

	hfdcan->Init.DataPrescaler = dat_brp;
	hfdcan->Init.DataTimeSeg1  = dat_seg1;
	hfdcan->Init.DataTimeSeg2  = dat_seg2;
	hfdcan->Init.DataSyncJumpWidth = dat_sjw;

	HAL_FDCAN_Init(hfdcan);
}

/* ============================================================
 * 发送 / 接收
 * ============================================================ */
uint8_t fdcanx_send_data(hcan_t *hfdcan, uint16_t id, uint8_t *data, uint32_t len)
{
    FDCAN_TxHeaderTypeDef pTxHeader;
    pTxHeader.Identifier=id;
    pTxHeader.IdType=FDCAN_STANDARD_ID;
    pTxHeader.TxFrameType=FDCAN_DATA_FRAME;
    pTxHeader.DataLength=ByteToDLC((uint8_t)len);
    pTxHeader.ErrorStateIndicator=FDCAN_ESI_ACTIVE;

    if(hfdcan->Init.FrameFormat == FDCAN_FRAME_CLASSIC)
    {
        pTxHeader.BitRateSwitch=FDCAN_BRS_OFF;
        pTxHeader.FDFormat=FDCAN_CLASSIC_CAN;
    }
    else
    {
        pTxHeader.BitRateSwitch=FDCAN_BRS_ON;
        pTxHeader.FDFormat=FDCAN_FD_CAN;
    }

    pTxHeader.TxEventFifoControl=FDCAN_NO_TX_EVENTS;
    pTxHeader.MessageMarker=0;

	if(HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &pTxHeader, data)!=HAL_OK)
		return 1;
	return 0;
}

uint8_t fdcanx_receive(hcan_t *hfdcan, uint16_t *rec_id, uint8_t *buf)
{
	FDCAN_RxHeaderTypeDef pRxHeader;

	if(HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &pRxHeader, buf)==HAL_OK)
	{
		*rec_id = pRxHeader.Identifier;
		return DLCToByte(pRxHeader.DataLength);
	}
	return 0;
}

/* ============================================================
 * HAL 中断回调
 * ============================================================ */
/**
 * @brief 统一 FIFO0 接收回调（HAL 真实回调函数）
 *        while 循环读空 FIFO，匹配 FDCANInstance 分发
 */
// DEBUG: 暴露最近一次 RX 的原始 DLC 和数据（供外部打印）
volatile uint32_t dbg_fdcan_raw_dlc = 0xDEAD;
volatile uint8_t  dbg_fdcan_raw_data[8] = {};
volatile uint16_t dbg_fdcan_raw_id = 0;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[64];

    while (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
    {
        // DEBUG: 记录原始值
        dbg_fdcan_raw_dlc = rx_header.DataLength;
        dbg_fdcan_raw_id  = rx_header.Identifier;
        memcpy((void*)dbg_fdcan_raw_data, rx_data, 8);

        uint16_t rx_id = rx_header.Identifier;
        uint8_t  rx_len = DLCToByte(rx_header.DataLength);

        for (uint8_t i = 0; i < fdcan_idx; i++)
        {
            if (hfdcan == fdcan_instance[i]->fdcan_handle &&
                rx_id  == fdcan_instance[i]->rx_id)
            {
                fdcan_instance[i]->rx_len = rx_len;
                memcpy(fdcan_instance[i]->rx_buff, rx_data, rx_len);
                if (fdcan_instance[i]->fdcan_module_callback)
                    fdcan_instance[i]->fdcan_module_callback(fdcan_instance[i]);
                break;
            }
        }
    }
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
	if(ErrorStatusITs & FDCAN_IR_BO)
	{
		CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
	}
}

/* ============================================================
 * FDCANInstance 面向对象接口（供 dji_motor 等模块使用）
 * ============================================================ */
FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config) {
    if (fdcan_idx >= FDCAN_MX_REGISTER_CNT) return NULL;

    FDCANInstance *instance = (FDCANInstance *)malloc(sizeof(FDCANInstance));
    memset(instance, 0, sizeof(FDCANInstance));

    instance->fdcan_handle = config->fdcan_handle;
    instance->tx_id        = config->tx_id;
    instance->rx_id        = config->rx_id;
    instance->fdcan_module_callback = config->fdcan_module_callback;
    instance->id           = config->id;

    instance->txconf.Identifier         = config->tx_id;
    instance->txconf.IdType             = FDCAN_STANDARD_ID;
    instance->txconf.TxFrameType        = FDCAN_DATA_FRAME;
    instance->txconf.DataLength         = ByteToDLC(config->data_len);
    instance->txconf.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    instance->txconf.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    instance->txconf.MessageMarker      = 0;

    if (config->fdcan_handle->Init.FrameFormat == FDCAN_FRAME_CLASSIC) {
        instance->txconf.BitRateSwitch = FDCAN_BRS_OFF;
        instance->txconf.FDFormat      = FDCAN_CLASSIC_CAN;
    } else {
        instance->txconf.BitRateSwitch = FDCAN_BRS_ON;
        instance->txconf.FDFormat      = FDCAN_FD_CAN;
    }

    fdcan_instance[fdcan_idx++] = instance;
    return instance;
}

uint8_t FDCANTransmit(FDCANInstance *_instance, uint16_t len) {
    _instance->txconf.DataLength = ByteToDLC((uint8_t)len);
    return (HAL_FDCAN_AddMessageToTxFifoQ(_instance->fdcan_handle,
                                          &_instance->txconf,
                                          _instance->tx_buff) == HAL_OK) ? 1 : 0;
}
