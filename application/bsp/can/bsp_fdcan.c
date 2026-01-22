#include "bsp_fdcan.h"
/**
************************************************************************
* @brief:      	bsp_can_init(void)
* @param:       void
* @retval:     	void
* @details:    	CAN 使能
************************************************************************
**/
void bsp_can_init(void)
{
	
	can_filter_init();
	HAL_FDCAN_Start(&hfdcan1);                               //开启FDCAN
//	HAL_FDCAN_Start(&hfdcan2);
//	HAL_FDCAN_Start(&hfdcan3);
//	HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_WATERMARK, 0);
//	HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO1_WATERMARK, 0);
//	HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_BUFFER_NEW_MESSAGE, 0);
	HAL_FDCAN_ActivateNotification(&hfdcan1,
                                       0 | FDCAN_IT_RX_FIFO0_WATERMARK | FDCAN_IT_RX_FIFO0_WATERMARK
                                           | FDCAN_IT_TX_COMPLETE | FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_BUS_OFF
                                           | FDCAN_IT_ARB_PROTOCOL_ERROR | FDCAN_IT_DATA_PROTOCOL_ERROR
                                           | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING,
                                       0x00000F00);
}
/**
************************************************************************
* @brief:      	can_filter_init(void)
* @param:       void
* @retval:     	void
* @details:    	CAN滤波器初始化
************************************************************************
**/
void can_filter_init(void)
{
	FDCAN_FilterTypeDef fdcan_filter;
	
	fdcan_filter.IdType = FDCAN_STANDARD_ID;                       //标准ID
	fdcan_filter.FilterIndex = 0;                                  //滤波器索引                   
	fdcan_filter.FilterType = FDCAN_FILTER_MASK;                   
	fdcan_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;           //过滤器0关联到FIFO0  
	fdcan_filter.FilterID1 = 0x00;                               
	fdcan_filter.FilterID2 = 0x00;

	HAL_FDCAN_ConfigFilter(&hfdcan1,&fdcan_filter); 		 				  //接收ID2
	//拒绝接收匹配不成功的标准ID和扩展ID,不接受远程帧
	HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,FDCAN_REJECT,FDCAN_REJECT,FDCAN_REJECT_REMOTE,FDCAN_REJECT_REMOTE);
	HAL_FDCAN_ConfigFifoWatermark(&hfdcan1, FDCAN_CFG_RX_FIFO0, 1);
//	HAL_FDCAN_ConfigFifoWatermark(&hfdcan1, FDCAN_CFG_RX_FIFO1, 1);
//	HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_TX_COMPLETE, FDCAN_TX_BUFFER0);
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


/**
************************************************************************
* @brief:      	fdcanx_send_data(FDCAN_HandleTypeDef *hfdcan, uint16_t id, uint8_t *data, uint32_t len)
* @param:       hfdcan：FDCAN句柄
* @param:       id：CAN设备ID
* @param:       data：发送的数据
* @param:       len：发送的数据长度
* @retval:     	void
* @details:    	发送数据
************************************************************************
**/
uint8_t fdcanx_send_data(hcan_t *hfdcan, uint16_t id, uint8_t *data, uint32_t len)
{
    FDCAN_TxHeaderTypeDef pTxHeader;
    pTxHeader.Identifier=id;
    pTxHeader.IdType=FDCAN_STANDARD_ID;
    pTxHeader.TxFrameType=FDCAN_DATA_FRAME;

	// FDCAN数据长度必须使用DLC编码
	if(len==0)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_0;
	else if(len==1)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_1;
	else if(len==2)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_2;
	else if(len==3)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_3;
	else if(len==4)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_4;
	else if(len==5)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_5;
	else if(len==6)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_6;
	else if(len==7)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_7;
	else if(len==8)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_8;
	else if(len==12)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_12;
	else if(len==16)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_16;
	else if(len==20)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_20;
	else if(len==24)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_24;
	else if(len==32)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_32;
	else if(len==48)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_48;
	else if(len==64)
		pTxHeader.DataLength = FDCAN_DLC_BYTES_64;
	else
		pTxHeader.DataLength = FDCAN_DLC_BYTES_8;  // 默认8字节

    pTxHeader.ErrorStateIndicator=FDCAN_ESI_ACTIVE;

    // 根据当前配置的帧格式设置BRS和FD标志
    if(hfdcan->Init.FrameFormat == FDCAN_FRAME_CLASSIC)
    {
        pTxHeader.BitRateSwitch=FDCAN_BRS_OFF;
        pTxHeader.FDFormat=FDCAN_CLASSIC_CAN;
    }
    else  // FDCAN_FRAME_FD_BRS
    {
        pTxHeader.BitRateSwitch=FDCAN_BRS_ON;
        pTxHeader.FDFormat=FDCAN_FD_CAN;
    }

    pTxHeader.TxEventFifoControl=FDCAN_NO_TX_EVENTS;
    pTxHeader.MessageMarker=0;

    FDCAN_TxHeaderTypeDef TxHeader;

    pTxHeader.Identifier = id;
    pTxHeader.IdType = FDCAN_STANDARD_ID; // 必须是 STANDARD_ID，严禁使用 EXTENDED_ID
    pTxHeader.TxFrameType = FDCAN_DATA_FRAME;
    pTxHeader.DataLength = FDCAN_DLC_BYTES_8; // 长度为8
    pTxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    pTxHeader.BitRateSwitch = FDCAN_BRS_OFF; // 关闭波特率切换
    pTxHeader.FDFormat = FDCAN_CLASSIC_CAN;  // 必须是 CLASSIC_CAN
    pTxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    pTxHeader.MessageMarker = 0;

	if(HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &pTxHeader, data)!=HAL_OK)
		return 1;//发送
	return 0;
}
/**
************************************************************************
* @brief:      	fdcanx_receive(FDCAN_HandleTypeDef *hfdcan, uint8_t *buf)
* @param:       hfdcan：FDCAN句柄
* @param:       buf：接收数据缓存
* @retval:     	接收的数据长度
* @details:    	接收数据
************************************************************************
**/
uint8_t fdcanx_receive(hcan_t *hfdcan, uint16_t *rec_id, uint8_t *buf)
{	
	FDCAN_RxHeaderTypeDef pRxHeader;
	uint8_t len;
	
	if(HAL_FDCAN_GetRxMessage(hfdcan,FDCAN_RX_FIFO0, &pRxHeader, buf)==HAL_OK)
	{
		*rec_id = pRxHeader.Identifier;
		if(pRxHeader.DataLength<=FDCAN_DLC_BYTES_8)
			len = pRxHeader.DataLength;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_12)
			len = 12;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_16)
			len = 16;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_20)
			len = 20;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_24)
			len = 24;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_32)
			len = 32;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_48)
			len = 48;
		else if(pRxHeader.DataLength==FDCAN_DLC_BYTES_64)
			len = 64;
		
		return len;//接收数据
	}
	return 0;	
}



__weak void fdcan1_rx_callback(void)
{

}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan == &hfdcan1)
    {
		fdcan1_rx_callback();
    }
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
	if(ErrorStatusITs & FDCAN_IR_BO)
	{
		CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
	}
	if(ErrorStatusITs & FDCAN_IR_EP)
	{
		// MX_FDCAN1_Init();
		// bsp_can_init();
	}
}


#include "main.h"
#include "fdcan.h"
#include "stdlib.h"
#include "string.h"

static FDCANInstance *fdcan_instance[FDCAN_MX_REGISTER_CNT] = {NULL}; // 全局动态列表
static uint8_t fdcan_idx = 0;

/* ---------------- 内部私有函数 ---------------- */

/**
 * @brief 字节长度转FDCAN DLC编码
 */
static uint32_t ByteToDLC(uint8_t len){
    if(len <= 8) return len << 16;
    if(len <= 12) return FDCAN_DLC_BYTES_12;
    if(len <= 16) return FDCAN_DLC_BYTES_16;
    if(len <= 20) return FDCAN_DLC_BYTES_20;
    if(len <= 24) return FDCAN_DLC_BYTES_24;
    if(len <= 32) return FDCAN_DLC_BYTES_32;
    if(len <= 48) return FDCAN_DLC_BYTES_48;
    return FDCAN_DLC_BYTES_64;
}

/**
 * @brief FDCAN DLC编码转字节长度
 */
static uint8_t DLCToByte(uint32_t dlc) {
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

/**
 * @brief 为单独的实例配置硬件过滤器
 */
static void FDCANAddFilter(FDCANInstance *_instance)
{
    FDCAN_FilterTypeDef FilterConfig;
    FilterConfig.IdType = FDCAN_STANDARD_ID;
    FilterConfig.FilterIndex = 0; // 简单的索引分配
    FilterConfig.FilterType = FDCAN_FILTER_MASK;
    FilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    FilterConfig.FilterID1 = 0;
    FilterConfig.FilterID2 = 0; // 精确匹配接收ID

    HAL_FDCAN_ConfigFilter(_instance->fdcan_handle, &FilterConfig);
}

/* ---------------- 外部接口函数 ---------------- */
/**
 * @brief 动态设置FDCAN波特率 (基于80MHz时钟)
 * @note 注意，不要自行把这个封装到实例中！波特率针对的是整条can线路，不适合作为实例的属性！
 */
 /*
void bsp_fdcan_set_baud(FDCAN_HandleTypeDef *hfdcan,
                        uint8_t mode, uint8_t baud) {
    uint32_t nom_brp=1, nom_seg1=59, nom_seg2=20, nom_sjw=20;
    uint32_t dat_brp=1, dat_seg1=29, dat_seg2=10, dat_sjw=10;

    HAL_FDCAN_Stop(hfdcan); // 修改前必须停止

    if (mode == CAN_CLASS) {
        hfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
        switch (baud) {
        case CAN_BR_125K: nom_brp=4; nom_seg1=139; nom_seg2=20; nom_sjw=20; break;
        case CAN_BR_500K: nom_brp=1; nom_seg1=139; nom_seg2=20; nom_sjw=20; break;
        case CAN_BR_1M:   nom_brp=1; nom_seg1=59;  nom_seg2=20; nom_sjw=20; break;
        }
    } else {
        hfdcan->Init.FrameFormat = FDCAN_FRAME_FD_BRS;
        switch (baud) {
        case CAN_BR_2M: dat_brp=1; dat_seg1=29; dat_seg2=10; dat_sjw=10; break;
        case CAN_BR_5M: dat_brp=1; dat_seg1=13; dat_seg2=2;  dat_sjw=2;  break;
        }
    }

    hfdcan->Init.NominalPrescaler = nom_brp;
    hfdcan->Init.NominalTimeSeg1 = nom_seg1;
    hfdcan->Init.NominalTimeSeg2 = nom_seg2;
    hfdcan->Init.NominalSyncJumpWidth = nom_sjw;
    hfdcan->Init.DataPrescaler = dat_brp;
    hfdcan->Init.DataTimeSeg1 = dat_seg1;
    hfdcan->Init.DataTimeSeg2 = dat_seg2;
    hfdcan->Init.DataSyncJumpWidth = dat_sjw;

    HAL_FDCAN_Init(hfdcan); // 重新应用时序
}
*/
/**
 * @brief 注册FDCAN实例
 * @note 核心函数！！！
 */
FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config){
    if(fdcan_idx >= FDCAN_MX_REGISTER_CNT) return NULL; // fdcan_索引是否超过最大允许实例数

    // CAN总线全局初始化配置，检测硬件状态是否为READY,确认第一次注册实例，总线级别初始化
    // @brief 此代码段与实例相关解耦！决定哪条CAN总线的全局默认配置
    if(config->fdcan_handle->State == HAL_FDCAN_STATE_READY){
        // 1. 全局过滤配置：拒绝未定义报文
        HAL_FDCAN_ConfigGlobalFilter(config->fdcan_handle, FDCAN_REJECT, FDCAN_REJECT,FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
        // 2. 水位线配置：1条消息即触发中断
        HAL_FDCAN_ConfigFifoWatermark(config->fdcan_handle, FDCAN_CFG_RX_FIFO0, 1);

        // 3. 开启硬件
        HAL_FDCAN_Start(config->fdcan_handle);
        // 4. 激活通知中断
       // HAL_FDCAN_ActivateNotification(config->fdcan_handle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

        HAL_FDCAN_ActivateNotification(config->fdcan_handle,
                                       0 | FDCAN_IT_RX_FIFO0_WATERMARK | FDCAN_IT_RX_FIFO0_WATERMARK
                                       | FDCAN_IT_TX_COMPLETE | FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_BUS_OFF
                                       | FDCAN_IT_ARB_PROTOCOL_ERROR | FDCAN_IT_DATA_PROTOCOL_ERROR
                                       | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING,
                                       0x00000F00);
    }

    FDCANInstance  *instance = (FDCANInstance*)malloc(sizeof(FDCANInstance));
    memset(instance, 0, sizeof(FDCANInstance));

    // 配置内容
    instance->fdcan_handle = config->fdcan_handle;
    instance->tx_id = config->tx_id;
    instance->rx_id = config->rx_id;
    instance->fdcan_module_callback = config->fdcan_module_callback;
    instance->id = config->id;

    // 配置发送头
    instance->txconf.Identifier = config->tx_id;
    instance->txconf.IdType = FDCAN_STANDARD_ID;
    instance->txconf.TxFrameType = FDCAN_DATA_FRAME;
    instance->txconf.DataLength = ByteToDLC(config->data_len);
    instance->txconf.ErrorStateIndicator = FDCAN_ESI_ACTIVE;

    // 根据总线实际初始化模式动态配置FD功能
    if (config->fdcan_handle->Init.FrameFormat == FDCAN_FRAME_CLASSIC) {
        instance->txconf.BitRateSwitch = FDCAN_BRS_OFF;
        instance->txconf.FDFormat = FDCAN_CLASSIC_CAN;
    } else {
        instance->txconf.BitRateSwitch = FDCAN_BRS_ON;
        instance->txconf.FDFormat = FDCAN_FD_CAN;
    }

    instance->txconf.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    instance->txconf.MessageMarker = 0;

    // 配置滤波器
    FDCANAddFilter(instance);
    fdcan_instance[fdcan_idx++] = instance;

    return instance; // 必须返回实例指针
}

/**
 * @brief FDCAN发送函数
 */
uint8_t FDCANTransmit(FDCANInstance *_instance, uint16_t len) {
    _instance->txconf.DataLength = ByteToDLC(len);
    if (HAL_FDCAN_AddMessageToTxFifoQ(_instance->fdcan_handle, &_instance->txconf, _instance->tx_buff) != HAL_OK) {
        return 0;
    }
    return 1;
}


/* ---------------- HAL中断/回调处理部分 ---------------- */
/**
 * @brief 中断分发回调
 */
/*
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {

    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) { // 中断(消息)标志位检测
        FDCAN_RxHeaderTypeDef rx_header;
        uint8_t rx_data[64];//接受缓存创建（申请）

        // 循环提取FIFO中所有报文，直到FIFO为空，防止FIFO溢出
        while (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0,
                                      &rx_header, rx_data) == HAL_OK) {
            // 遍历 CAN总线双匹配算法，定位具体实例，无需重复写接受的逻辑，实现多总线，多设备逻辑泛化
            for (uint8_t i = 0; i < fdcan_idx; i++) { // 遍历所有注册的can实例
                // 精确匹配：硬件句柄 + 报文ID
                if (hfdcan == fdcan_instance[i]->fdcan_handle && // 确认来自哪条can总线
                    rx_header.Identifier == fdcan_instance[i]->rx_id) { // 确认来自该can总线哪个id(具体设备)

                    // 长度DLC转换
                    fdcan_instance[i]->rx_len = DLCToByte(rx_header.DataLength);
                    memcpy(fdcan_instance[i]->rx_buff, rx_data,
                           fdcan_instance[i]->rx_len);// 从临时栈空间拷贝到实例的物理空间中
                    if (fdcan_instance[i]->fdcan_module_callback != NULL) {//执行实例对应的应用级回调函数
                        fdcan_instance[i]->fdcan_module_callback(fdcan_instance[i]);
                    }

                    break;
                }
            }
        }
    }
}*/







