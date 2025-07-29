/**
 * @file    ble_ll.c
 * @brief   BLE Link Layer实现 / BLE Link Layer Implementation
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件实现了BLE链路层的核心功能
 *          This file implements the core functionality of BLE Link Layer
 *          
 *          主要功能 / Main features:
 *          - 连接管理(扫描、连接、断开) / Connection management (scan, connect, disconnect)
 *          - 数据传输(PDU处理) / Data transmission (PDU handling)
 *          - 信道管理(跳频) / Channel management (frequency hopping)
 *          - 时序控制(微秒级精度) / Timing control (microsecond precision)
 */

#include "ble_ll.h"
#include "stm32g0xx_hal.h"
#include <stdlib.h>

/* 外部定时器句柄（需要在main.c中定义） / External Timer Handles (must be defined in main.c) */
extern TIM_HandleTypeDef htim2;     // 微秒定时器 / Microsecond timer (TIM2)
extern LPTIM_HandleTypeDef hlptim1; // 连接事件定时器 / Connection event timer (LPTIM1)

/* 私有变量 / Private Variables */
static uint32_t g_us_counter_high = 0;  // 微秒计数器高32位 / Upper 32 bits of microsecond counter
static uint8_t g_lfsr_state = 0x53;     // 随机数生成器状态 / LFSR random generator state

/* CRC函数在ble_ll_missing.c中实现，使用完整的查找表 / CRC function implemented in ble_ll_missing.c with complete lookup table */
extern uint32_t ble_ll_calculate_crc24(uint8_t* data, uint16_t len, uint32_t crc_init);

/* 信道映射表 / Channel Frequency Mapping Table */
static const uint32_t channel_freq_table[40] = {
    2402000000, 2404000000, 2406000000, 2408000000, 2410000000,  // 信道 0-4 / Channels 0-4
    2412000000, 2414000000, 2416000000, 2418000000, 2420000000,  // 信道 5-9 / Channels 5-9
    2422000000, 2424000000, 2426000000, 2428000000, 2430000000,  // 信道 10-14 / Channels 10-14
    2432000000, 2434000000, 2436000000, 2438000000, 2440000000,  // 信道 15-19 / Channels 15-19
    2442000000, 2444000000, 2446000000, 2448000000, 2450000000,  // 信道 20-24 / Channels 20-24
    2452000000, 2454000000, 2456000000, 2458000000, 2460000000,  // 信道 25-29 / Channels 25-29
    2462000000, 2464000000, 2466000000, 2468000000, 2470000000,  // 信道 30-34 / Channels 30-34
    2472000000, 2474000000, 2476000000, 2478000000, 2480000000   // 信道 35-39 / Channels 35-39
};

/**
 * @brief 初始化Link Layer / Initialize Link Layer
 * @param ctx 连接上下文 / Connection context
 * @param radio_context 无线电模块上下文 / Radio module context
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_init(ble_conn_context_t* ctx, void* radio_context)
{
    if (!ctx || !radio_context) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 清空上下文 / Clear context */
    memset(ctx, 0, sizeof(ble_conn_context_t));
    
    /* 设置默认值 / Set default values */
    ctx->radio_context = radio_context;
    ctx->conn_state = CONN_STATE_IDLE;
    ctx->role = BLE_ROLE_MASTER;       // 默认为主设备 / Default as master device
    ctx->max_packets_per_event = 4;    // 每个事件最多4个数据包 / Max 4 packets per event
    
    /* 生成随机本地地址 / Generate random local address */
    for (int i = 0; i < 6; i++) {
        ctx->local_addr[i] = ble_ll_get_random();
    }
    ctx->local_addr[5] |= 0xC0;  // 设置为随机静态地址 / Set as random static address (MSB = 11xxxxxx)
    
    /* 初始化信道图（使用所有37个数据信道） / Initialize channel map (use all 37 data channels) */
    memset(ctx->channel_map, 0xFF, 4);   // 前32个信道 / First 32 channels
    ctx->channel_map[4] = 0x1F;          // 后5个信道 / Last 5 channels (bit 0-4)
    ctx->num_used_channels = 37;         // 总共使用37个信道 / Total 37 channels used
    
    /* 初始化定时器 / Initialize timers */
    HAL_TIM_Base_Start(&htim2);  // 启动微秒定时器 / Start microsecond timer
    
    return BLE_STATUS_OK;
}

/**
 * @brief 反初始化Link Layer / Deinitialize Link Layer
 * @param ctx 连接上下文 / Connection context
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_deinit(ble_conn_context_t* ctx)
{
    if (!ctx) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 如果已连接，先断开 / If connected, disconnect first */
    if (ctx->conn_state == CONN_STATE_CONNECTED) {
        ble_ll_disconnect(ctx, 0x16);  // 远端用户终止连接 / Remote User Terminated Connection
    }
    
    /* 停止定时器 / Stop timers */
    HAL_TIM_Base_Stop(&htim2);         // 停止微秒定时器 / Stop microsecond timer
    HAL_LPTIM_Counter_Stop(&hlptim1);  // 停止连接事件定时器 / Stop connection event timer
    
    return BLE_STATUS_OK;
}

/**
 * @brief 设置本地地址 / Set local address
 * @param ctx 连接上下文 / Connection context
 * @param addr 要设置的6字节地址 / 6-byte address to set
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_set_address(ble_conn_context_t* ctx, uint8_t* addr)
{
    if (!ctx || !addr) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    memcpy(ctx->local_addr, addr, 6);
    return BLE_STATUS_OK;
}

/**
 * @brief 开始扫描 / Start scanning
 * @param ctx 连接上下文 / Connection context
 * @param params 扫描参数 / Scan parameters
 * @param filter 扫描过滤器回调 / Scan filter callback
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_start_scanning(ble_conn_context_t* ctx, 
                                   ble_scan_params_t* params,
                                   ble_scan_filter_cb filter)
{
    if (!ctx || !params) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->conn_state != CONN_STATE_IDLE) {
        return BLE_STATUS_BUSY;
    }
    
    ctx->conn_state = CONN_STATE_SCANNING;
    
    /* 配置SX1280为接收模式 / Configure SX1280 for receive mode */
    sx128x_set_standby(ctx->radio_context, SX128X_STANDBY_RC);    // 切换到待机模式 / Switch to standby mode
    sx128x_set_pkt_type(ctx->radio_context, SX128X_PKT_TYPE_BLE); // 设置为BLE包类型 / Set BLE packet type
    
    /* 设置BLE PHY参数 / Set BLE PHY parameters */
    sx128x_mod_params_ble_t mod_params = {
        .br_bw = SX128X_BLE_BR_1_000_BW_1_2,      // 比特率1Mbps，带宽1.2MHz / Bit rate 1Mbps, bandwidth 1.2MHz
        .mod_ind = SX128X_BLE_MOD_IND_0_50,        // 调制指数0.5 / Modulation index 0.5
        .pulse_shape = SX128X_BLE_PULSE_SHAPE_OFF  // 无脉冲整形 / No pulse shaping
    };
    sx128x_set_ble_mod_params(ctx->radio_context, &mod_params);
    
    /* 设置包参数 / Set packet parameters */
    sx128x_pkt_params_ble_t pkt_params = {
        .con_state = SX128X_BLE_SCANNER,           // 设置为扫描器模式 / Set as scanner mode
        .crc_type = SX128X_BLE_CRC_3B,             // 3字节CRC / 3-byte CRC
        .pkt_type = SX128X_BLE_PRBS_9,             // 使用PRBS9序列 / Use PRBS9 sequence
        .dc_free = SX128X_BLE_WHITENING_ENABLE     // 启用数据白化 / Enable data whitening
    };
    sx128x_set_ble_pkt_params(ctx->radio_context, &pkt_params);
    
    /* 设置广播接入地址 / Set advertising access address */
    uint8_t sync_word[4] = {0xD6, 0xBE, 0x89, 0x8E};  // 0x8E89BED6小端字节序 / Little-endian byte order
    sx128x_set_ble_sync_word(ctx->radio_context, sync_word);
    
    /* 设置CRC种子 / Set CRC seed */
    sx128x_set_ble_crc_seed(ctx->radio_context, BLE_CRC_INIT_ADV);  // 广播CRC初始值0x555555 / Advertising CRC init value 0x555555
    
    /* 开始在广播信道37上扫描 / Start scanning on advertising channel 37 */
    sx128x_set_rf_freq(ctx->radio_context, channel_freq_table[37]);  // 2402 MHz
    sx128x_set_ble_whitening_seed(ctx->radio_context, 37 | 0x40);    // 信道37的白化种子 / Whitening seed for channel 37
    sx128x_set_rx(ctx->radio_context);                               // 进入接收模式 / Enter receive mode
    
    return BLE_STATUS_OK;
}

/**
 * @brief 停止扫描 / Stop scanning
 * @param ctx 连接上下文 / Connection context
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_stop_scanning(ble_conn_context_t* ctx)
{
    if (!ctx) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->conn_state != CONN_STATE_SCANNING) {
        return BLE_STATUS_ERROR;
    }
    
    sx128x_set_standby(ctx->radio_context, SX128X_STANDBY_RC);
    ctx->conn_state = CONN_STATE_IDLE;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发起连接 / Initiate connection
 * @param ctx 连接上下文 / Connection context
 * @param peer_addr 对端设备地址 / Peer device address
 * @param params 连接参数 / Connection parameters
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_connect(ble_conn_context_t* ctx, 
                           uint8_t* peer_addr,
                           ble_conn_params_t* params)
{
    if (!ctx || !peer_addr || !params) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->conn_state != CONN_STATE_IDLE && 
        ctx->conn_state != CONN_STATE_SCANNING) {
        return BLE_STATUS_BUSY;
    }
    
    /* 保存连接参数 / Save connection parameters */
    memcpy(ctx->peer_addr, peer_addr, 6);
    ctx->conn_interval = params->conn_interval * 1250;  // 转换为微秒(1.25ms单位) / Convert to microseconds (1.25ms unit)
    ctx->slave_latency = params->slave_latency;
    ctx->supervision_timeout = params->supervision_timeout;
    
    /* 生成连接参数 / Generate connection parameters */
    ctx->access_address = ble_ll_generate_access_address();       // 生成唯一的接入地址 / Generate unique access address
    ctx->crc_init = ble_ll_generate_crc_init() & 0xFFFFFF;       // 生成24位CRC初始值 / Generate 24-bit CRC init value
    ctx->hop_increment = 5 + (ble_ll_get_random() % 12);         // 跳频增量(5-16) / Hop increment (5-16)
    
    ctx->conn_state = CONN_STATE_INITIATING;
    
    /* 准备发送CONNECT_REQ / Prepare to send CONNECT_REQ */
    // 实际发送在扫描到目标设备后进行 / Actual sending occurs after target device is found
    
    return BLE_STATUS_OK;
}

/**
 * @brief 断开连接 / Disconnect
 * @param ctx 连接上下文 / Connection context
 * @param reason 断开原因 / Disconnect reason
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_disconnect(ble_conn_context_t* ctx, uint8_t reason)
{
    if (!ctx) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->conn_state != CONN_STATE_CONNECTED) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    /* 发送LL_TERMINATE_IND / Send LL_TERMINATE_IND */
    uint8_t terminate_pdu[3] = {
        0x03,             // LLID: LL控制PDU / LLID: LL Control PDU
        0x02,             // 长度 / Length
        LL_TERMINATE_IND, // 终止指示 / Terminate indication
        reason            // 终止原因 / Termination reason
    };
    
    ctx->tx_length = 4;
    memcpy(ctx->tx_buffer, terminate_pdu, 4);
    ctx->tx_pending = true;
    
    /* 等待发送完成 / Wait for transmission to complete */
    // 实际发送在下一个连接事件中进行 / Actual transmission occurs in next connection event
    
    ctx->conn_state = CONN_STATE_DISCONNECTING;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发送数据 / Send data
 * @param ctx 连接上下文 / Connection context
 * @param data 要发送的数据 / Data to send
 * @param len 数据长度 / Data length
 * @return 操作状态 / Operation status
 */
ble_status_t ble_ll_send_data(ble_conn_context_t* ctx, uint8_t* data, uint16_t len)
{
    if (!ctx || !data || len == 0 || len > 251) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->conn_state != CONN_STATE_CONNECTED) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    if (ctx->tx_pending) {
        return BLE_STATUS_BUSY;
    }
    
    /* 构造L2CAP数据PDU / Construct L2CAP data PDU */
    ctx->tx_buffer[0] = 0x02;     // LLID: L2CAP数据 / LLID: L2CAP data
    ctx->tx_buffer[1] = len + 4;  // 长度包括L2CAP头 / Length includes L2CAP header
    
    /* L2CAP头 / L2CAP header */
    ctx->tx_buffer[2] = len & 0xFF;                    // L2CAP长度低字节 / L2CAP length low byte
    ctx->tx_buffer[3] = (len >> 8) & 0xFF;             // L2CAP长度高字节 / L2CAP length high byte
    ctx->tx_buffer[4] = L2CAP_CID_ATT & 0xFF;          // ATT通道ID低字节 / ATT channel ID low byte
    ctx->tx_buffer[5] = (L2CAP_CID_ATT >> 8) & 0xFF;   // ATT通道ID高字节 / ATT channel ID high byte
    
    /* 复制数据 / Copy data */
    memcpy(&ctx->tx_buffer[6], data, len);  // 从L2CAP头后开始复制 / Copy after L2CAP header
    
    ctx->tx_length = len + 6;
    ctx->tx_pending = true;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 处理连接事件
 */
static void ll_handle_connection_event(ble_conn_context_t* ctx)
{
    uint8_t channel;
    uint32_t freq;
    bool tx_success = false;
    bool rx_success = false;
    
    /* 计算当前数据信道 */
    channel = ble_ll_calculate_next_channel(ctx);
    freq = channel_freq_table[channel];
    
    /* 配置无线电 */
    sx128x_set_rf_freq(ctx->radio_context, freq);
    sx128x_set_ble_whitening_seed(ctx->radio_context, channel | 0x40);
    
    /* Master TX */
    if (ctx->tx_pending || ctx->event_counter == 0) {
        ble_data_pdu_t* tx_pdu = (ble_data_pdu_t*)ctx->tx_buffer;
        
        /* 设置PDU头 */
        if (!ctx->tx_pending) {
            /* 空PDU */
            tx_pdu->llid = 0x01;  // LL Control PDU
            tx_pdu->length = 0;
        }
        
        tx_pdu->nesn = ctx->next_expected_seq_num;
        tx_pdu->sn = ctx->tx_seq_num;
        tx_pdu->md = ctx->more_data ? 1 : 0;
        
        /* 等待锚点 */
        if (ctx->event_counter == 0) {
            ble_ll_wait_until_us(ctx->anchor_point);
        }
        
        /* 发送 */
        sx128x_set_buffer_base_address(ctx->radio_context, 0x00, 0x80);
        sx128x_write_buffer(ctx->radio_context, 0x00, 
                          (uint8_t*)tx_pdu, tx_pdu->length + 2);
        sx128x_set_tx(ctx->radio_context);
        
        /* 等待TX完成 */
        uint32_t timeout = 1000;  // 1ms超时
        while (timeout-- && 
               !(sx128x_get_irq_status(ctx->radio_context) & SX128X_IRQ_TX_DONE)) {
            ble_ll_delay_us(1);
        }
        
        if (timeout > 0) {
            tx_success = true;
            sx128x_clear_irq_status(ctx->radio_context, SX128X_IRQ_TX_DONE);
        }
    }
    
    /* T_IFS延时 */
    ble_ll_delay_us(BLE_T_IFS);
    
    /* Master RX */
    sx128x_set_rx_with_timeout(ctx->radio_context, 2);  // 2ms超时
    
    uint32_t rx_start = ble_ll_get_timestamp_us();
    while ((ble_ll_get_timestamp_us() - rx_start) < 2000) {
        if (sx128x_get_irq_status(ctx->radio_context) & SX128X_IRQ_RX_DONE) {
            uint8_t rx_len;
            sx128x_get_rx_buffer_status(ctx->radio_context, &rx_len, NULL);
            sx128x_read_buffer(ctx->radio_context, 0x80, ctx->rx_buffer, rx_len);
            
            rx_success = true;
            ctx->consecutive_crc_errors = 0;
            
            /* 处理接收的PDU */
            ble_data_pdu_t* rx_pdu = (ble_data_pdu_t*)ctx->rx_buffer;
            
            /* 检查序列号 */
            if (rx_pdu->sn == ctx->next_expected_seq_num) {
                /* 新数据 */
                ctx->next_expected_seq_num ^= 1;
                
                if (rx_pdu->length > 0) {
                    /* 处理数据 */
                    if (rx_pdu->llid == 0x02) {  // L2CAP数据
                        if (ctx->on_data_received) {
                            ctx->on_data_received(ctx, &rx_pdu->payload[4], 
                                                rx_pdu->length - 4);
                        }
                    } else if (rx_pdu->llid == 0x03) {  // LL控制PDU
                        /* 处理控制PDU */
                        switch (rx_pdu->payload[0]) {
                            case LL_TERMINATE_IND:
                                ctx->conn_state = CONN_STATE_IDLE;
                                if (ctx->on_disconnected) {
                                    ctx->on_disconnected(ctx, rx_pdu->payload[1]);
                                }
                                return;
                                
                            case LL_VERSION_IND:
                                /* 忽略版本信息 */
                                break;
                                
                            case LL_FEATURE_REQ:
                                /* 回复feature response */
                                {
                                    uint8_t feature_rsp[11];
                                    feature_rsp[0] = 0x03;  // LLID = Control PDU
                                    feature_rsp[1] = 9;     // Length
                                    feature_rsp[2] = LL_FEATURE_RSP;
                                    /* 支持的特性（简化：不支持任何扩展特性） */
                                    memset(&feature_rsp[3], 0, 8);
                                    
                                    /* 保存到发送缓冲区 */
                                    memcpy(ctx->tx_buffer, feature_rsp, 11);
                                    ctx->tx_length = 11;
                                    ctx->tx_pending = true;
                                }
                                break;
                        }
                    }
                }
            }
            
            /* 确认收到 / Acknowledge receipt */
            if (rx_pdu->nesn != ctx->tx_seq_num) {
                /* 对方确认了我们的数据 / Peer acknowledged our data */
                ctx->tx_seq_num ^= 1;      // 翻转发送序列号 / Toggle transmit sequence number
                ctx->tx_pending = false;   // 清除发送等待标志 / Clear transmit pending flag
            }
            
            /* 检查MD位 / Check MD bit */
            ctx->more_data = rx_pdu->md;  // 更新More Data标志 / Update More Data flag
            
            break;
        }
    }
    
    if (!rx_success) {
        ctx->consecutive_crc_errors++;
        ctx->total_crc_errors++;
    }
    
    /* 更新连接事件参数 / Update connection event parameters */
    ctx->event_counter++;                      // 增加事件计数器 / Increment event counter
    ctx->anchor_point += ctx->conn_interval;  // 计算下一个锚点 / Calculate next anchor point
    
    /* 检查连接超时 / Check connection timeout */
    if (ctx->consecutive_crc_errors > 6) {
        /* 连接丢失 / Connection lost */
        ctx->conn_state = CONN_STATE_IDLE;
        if (ctx->on_disconnected) {
            ctx->on_disconnected(ctx, 0x08);  // 连接超时 / Connection Timeout
        }
    }
}

/**
 * @brief 处理事件（主循环调用） / Process events (called from main loop)
 * @param ctx 连接上下文 / Connection context
 * 
 * @details 该函数需要在主循环中不断调用，用于处理各种连接状态下的事件
 *          This function needs to be called continuously in main loop to process events in various connection states
 */
void ble_ll_process_events(ble_conn_context_t* ctx)
{
    if (!ctx) return;
    
    switch (ctx->conn_state) {
        case CONN_STATE_SCANNING:
            /* 检查是否收到广播包 / Check if advertising packet received */
            if (sx128x_get_irq_status(ctx->radio_context) & SX128X_IRQ_RX_DONE) {
                uint8_t rx_len;
                uint8_t rx_buffer[255];
                
                sx128x_get_rx_buffer_status(ctx->radio_context, &rx_len, NULL);
                sx128x_read_buffer(ctx->radio_context, 0x80, rx_buffer, rx_len);
                sx128x_clear_irq_status(ctx->radio_context, SX128X_IRQ_RX_DONE);
                
                /* 解析广播包 / Parse advertising packet */
                ble_adv_pdu_t* adv = (ble_adv_pdu_t*)rx_buffer;
                
                /* 检查是否是目标设备 / Check if target device */
                if (ctx->conn_state == CONN_STATE_INITIATING &&
                    memcmp(&adv->payload[0], ctx->peer_addr, 6) == 0) {
                    /* 发送连接请求 / Send connection request */
                    ll_connect_req_t conn_req;
                    
                    /* 构建连接请求 / Build connection request */
                    conn_req.header = BLE_PDU_CONNECT_REQ;
                    conn_req.length = 34;                                     // 固定长度 / Fixed length
                    memcpy(conn_req.init_addr, ctx->local_addr, 6);         // 发起方地址 / Initiator address
                    memcpy(conn_req.adv_addr, ctx->peer_addr, 6);           // 广播方地址 / Advertiser address
                    conn_req.access_address = ctx->access_address;          // 连接接入地址 / Connection access address
                    conn_req.crc_init = ctx->crc_init & 0xFFFFFF;          // CRC初始值 / CRC init value
                    conn_req.win_size = 2;                                   // 2.5ms窗口 / 2.5ms window
                    conn_req.win_offset = 0;                                 // 窗口偏移 / Window offset
                    conn_req.interval = ctx->conn_interval / 1250;          // 转换为1.25ms单位 / Convert to 1.25ms units
                    conn_req.latency = ctx->slave_latency;                  // 从设备延迟 / Slave latency
                    conn_req.timeout = ctx->supervision_timeout / 10;       // 转换为10ms单位 / Convert to 10ms units
                    memcpy(conn_req.channel_map, ctx->channel_map, 5);      // 信道图 / Channel map
                    conn_req.hop = ctx->hop_increment;                      // 跳频增量 / Hop increment
                    conn_req.sca = 0;                                       // 睡眠时钟精度±50ppm / Sleep Clock Accuracy ±50ppm
                    
                    /* 发送连接请求 / Send connection request */
                    if (ll_send_connect_request(ctx, &conn_req) == BLE_STATUS_OK) {
                        /* 计算第一个锚点 / Calculate first anchor point */
                        ctx->anchor_point = ble_ll_get_timestamp_us() + 1250;  // 1.25ms后 / 1.25ms later
                        ctx->event_counter = 0;
                        ctx->conn_state = CONN_STATE_CONNECTION;
                        
                        /* 切换到第一个数据信道 / Switch to first data channel */
                        ctx->current_channel = ble_ll_calculate_next_channel(ctx);
                    }
                }
                
                /* 继续扫描 / Continue scanning */
                sx128x_set_rx(ctx->radio_context);  // 重新进入接收模式 / Re-enter receive mode
            }
            break;
            
        case CONN_STATE_CONNECTED:
            /* 检查是否到了连接事件时间 */
            if (ble_ll_get_timestamp_us() >= ctx->anchor_point) {
                ll_handle_connection_event(ctx);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 计算下一个数据信道
 */
uint8_t ble_ll_calculate_next_channel(ble_conn_context_t* ctx)
{
    uint8_t unmapped_channel;
    uint8_t remapping_index;
    uint8_t channel;
    
    /* 计算未映射信道 */
    unmapped_channel = (ctx->last_unmapped_channel + ctx->hop_increment) % 37;
    ctx->last_unmapped_channel = unmapped_channel;
    
    /* 检查信道是否可用 */
    if (ctx->channel_map[unmapped_channel >> 3] & (1 << (unmapped_channel & 0x07))) {
        return unmapped_channel;
    }
    
    /* 重映射 */
    remapping_index = unmapped_channel % ctx->num_used_channels;
    
    /* 找到第N个可用信道 */
    uint8_t count = 0;
    for (channel = 0; channel < 37; channel++) {
        if (ctx->channel_map[channel >> 3] & (1 << (channel & 0x07))) {
            if (count == remapping_index) {
                return channel;
            }
            count++;
        }
    }
    
    return 0;  // 不应该到达
}

/**
 * @brief 生成接入地址
 */
uint32_t ble_ll_generate_access_address(void)
{
    uint32_t aa;
    
    do {
        /* 生成随机地址 */
        aa = (ble_ll_get_random() << 24) |
             (ble_ll_get_random() << 16) |
             (ble_ll_get_random() << 8) |
              ble_ll_get_random();
        
        /* 使用验证函数检查BLE规范要求 / Use validation function to check BLE spec requirements */
        if (ll_validate_access_address(aa)) {
            break;  // 符合要求，退出循环 / Meet requirements, exit loop
        }
    } while (1);
    
    return aa;
}

/**
 * @brief 生成CRC初始值 / Generate CRC init value
 * @return 24位CRC初始值 / 24-bit CRC init value
 */
uint32_t ble_ll_generate_crc_init(void)
{
    return (ble_ll_get_random() << 16) |
           (ble_ll_get_random() << 8) |
            ble_ll_get_random();
}

/**
 * @brief 获取随机数 / Get random number
 * @return 8位随机数 / 8-bit random number
 * 
 * @details 使用线性反馈移位寄存器(LFSR)生成伪随机数
 *          Uses Linear Feedback Shift Register (LFSR) to generate pseudo-random numbers
 */
uint8_t ble_ll_get_random(void)
{
    /* 简单的LFSR随机数生成器 / Simple LFSR random number generator */
    uint8_t bit = ((g_lfsr_state >> 0) ^    // 位0 / Bit 0
                   (g_lfsr_state >> 2) ^    // 位2 / Bit 2
                   (g_lfsr_state >> 3) ^    // 位3 / Bit 3
                   (g_lfsr_state >> 5)) & 1; // 位5 / Bit 5
    g_lfsr_state = (g_lfsr_state >> 1) | (bit << 7);  // 移位并反馈 / Shift and feedback
    return g_lfsr_state;
}

/**
 * @brief 获取微秒时间戳 / Get microsecond timestamp
 * @return 64位微秒时间戳 / 64-bit microsecond timestamp
 * 
 * @details 组合TIM2计数器和高32位溢出计数器构成64位时间戳
 *          Combines TIM2 counter and upper 32-bit overflow counter to form 64-bit timestamp
 */
uint64_t ble_ll_get_timestamp_us(void)
{
    uint32_t cnt_low, cnt_high;
    
    do {
        cnt_high = g_us_counter_high;       // 读取高32位 / Read upper 32 bits
        cnt_low = TIM2->CNT;                // 读取TIM2计数器 / Read TIM2 counter
    } while (cnt_high != g_us_counter_high);  // 防止读取时溢出 / Prevent overflow during read
    
    return ((uint64_t)cnt_high << 32) | cnt_low;  // 组合64位值 / Combine to 64-bit value
}

/**
 * @brief 微秒延时 / Microsecond delay
 * @param us 延时微秒数 / Delay in microseconds
 * 
 * @note 使用TIM2实现精确微秒延时 / Uses TIM2 for precise microsecond delay
 */
void ble_ll_delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us);
}

/**
 * @brief 等待到指定时间戳
 */
void ble_ll_wait_until_us(uint64_t timestamp)
{
    while (ble_ll_get_timestamp_us() < timestamp);
}

/**
 * @brief 无线电中断处理
 */
void ble_ll_radio_irq_handler(ble_conn_context_t* ctx)
{
    /* 在中断中设置标志，主循环中处理 */
    // 实际处理在ble_ll_process_events中
}

/**
 * @brief 连接事件触发（定时器中断调用）
 */
void ble_ll_connection_event_trigger(ble_conn_context_t* ctx)
{
    /* 在中断中设置标志，主循环中处理 */
    // 实际处理在ble_ll_process_events中
}

/**
 * @brief 定时器溢出中断处理
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        g_us_counter_high++;
    }
}