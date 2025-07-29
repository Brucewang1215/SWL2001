/**
 * @file    ble_ll_missing.c
 * @brief   BLE Link Layer缺失函数的实现 / BLE Link Layer Missing Functions Implementation
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件实现了ble_ll.c中引用但未实现的函数
 *          This file implements functions referenced but not implemented in ble_ll.c
 *          
 *          主要包括 / Main contents:
 *          - CRC24计算(完整查找表) / CRC24 calculation (complete lookup table)
 *          - PDU发送和处理 / PDU sending and processing
 *          - 连接请求处理 / Connection request handling
 *          - 接入地址验证 / Access address validation
 */

#include "ble_ll.h"
#include "sx128x.h"
#include "stm32g0xx_hal.h"

/* 完整的CRC24查找表（BLE多项式: 0x100065B） / Complete CRC24 lookup table (BLE polynomial: 0x100065B) */
static const uint32_t crc24_table[256] = {
    0x000000, 0x01B4C0, 0x036980, 0x02DD40, 0x06D300, 0x0767C0, 0x05BA80, 0x040E40,
    0x0DA600, 0x0C12C0, 0x0ECF80, 0x0F7B40, 0x0B7500, 0x0AC1C0, 0x081C80, 0x09A840,
    0x1B4C00, 0x1AF8C0, 0x182580, 0x199140, 0x1D9F00, 0x1C2BC0, 0x1EF680, 0x1F4240,
    0x16EA00, 0x175EC0, 0x158380, 0x143740, 0x103900, 0x118DC0, 0x135080, 0x12E440,
    0x369800, 0x372CC0, 0x35F180, 0x344540, 0x304B00, 0x31FFC0, 0x332280, 0x329640,
    0x3B3E00, 0x3A8AC0, 0x385780, 0x39E340, 0x3DED00, 0x3C59C0, 0x3E8480, 0x3F3040,
    0x2DD400, 0x2C60C0, 0x2EBD80, 0x2F0940, 0x2B0700, 0x2AB3C0, 0x286E80, 0x29DA40,
    0x207200, 0x21C6C0, 0x231B80, 0x22AF40, 0x26A100, 0x2715C0, 0x25C880, 0x247C40,
    0x6D3000, 0x6C84C0, 0x6E5980, 0x6FED40, 0x6BE300, 0x6A57C0, 0x688A80, 0x693E40,
    0x609600, 0x6122C0, 0x63FF80, 0x624B40, 0x664500, 0x67F1C0, 0x652C80, 0x649840,
    0x767C00, 0x77C8C0, 0x751580, 0x74A140, 0x70AF00, 0x711BC0, 0x73C680, 0x727240,
    0x7BDA00, 0x7A6EC0, 0x78B380, 0x790740, 0x7D0900, 0x7CBDC0, 0x7E6080, 0x7FD440,
    0x5BA800, 0x5A1CC0, 0x58C180, 0x597540, 0x5D7B00, 0x5CCFC0, 0x5E1280, 0x5FA640,
    0x560E00, 0x57BAC0, 0x556780, 0x54D340, 0x50DD00, 0x5169C0, 0x53B480, 0x520040,
    0x40E400, 0x4150C0, 0x438D80, 0x423940, 0x463700, 0x4783C0, 0x455E80, 0x44EA40,
    0x4D4200, 0x4CF6C0, 0x4E2B80, 0x4F9F40, 0x4B9100, 0x4A25C0, 0x48F880, 0x494C40,
    0xDA6000, 0xDBD4C0, 0xD90980, 0xD8BD40, 0xDCB300, 0xDD07C0, 0xDFDA80, 0xDE6E40,
    0xD7C600, 0xD672C0, 0xD4AF80, 0xD51B40, 0xD11500, 0xD0A1C0, 0xD27C80, 0xD3C840,
    0xC12C00, 0xC098C0, 0xC24580, 0xC3F140, 0xC7FF00, 0xC64BC0, 0xC49680, 0xC52240,
    0xCC8A00, 0xCD3EC0, 0xCFE380, 0xCE5740, 0xCA5900, 0xCBEDC0, 0xC93080, 0xC88440,
    0xECF800, 0xED4CC0, 0xEF9180, 0xEE2540, 0xEA2B00, 0xEB9FC0, 0xE94280, 0xE8F640,
    0xE15E00, 0xE0EAC0, 0xE23780, 0xE38340, 0xE78D00, 0xE639C0, 0xE4E480, 0xE55040,
    0xF7B400, 0xF600C0, 0xF4DD80, 0xF56940, 0xF16700, 0xF0D3C0, 0xF20E80, 0xF3BA40,
    0xFA1200, 0xFBA6C0, 0xF97B80, 0xF8CF40, 0xFCC100, 0xFD75C0, 0xFF8880, 0xFE3C40,
    0xB67000, 0xB7C4C0, 0xB51980, 0xB4AD40, 0xB0A300, 0xB117C0, 0xB3CA80, 0xB27E40,
    0xBBD600, 0xBA62C0, 0xB8BF80, 0xB90B40, 0xBD0500, 0xBCB1C0, 0xBE6C80, 0xBFD840,
    0xAD3C00, 0xAC88C0, 0xAE5580, 0xAFE140, 0xABEF00, 0xAA5BC0, 0xA88680, 0xA93240,
    0xA09A00, 0xA12EC0, 0xA3F380, 0xA24740, 0xA64900, 0xA7FDC0, 0xA52080, 0xA49440,
    0x80E800, 0x815CC0, 0x838180, 0x823540, 0x863B00, 0x878FC0, 0x855280, 0x84E640,
    0x8D4E00, 0x8CFAC0, 0x8E2780, 0x8F9340, 0x8B9D00, 0x8A29C0, 0x88F480, 0x894040,
    0x9BA400, 0x9A10C0, 0x98CD80, 0x997940, 0x9D7700, 0x9CC3C0, 0x9E1E80, 0x9FAA40,
    0x960200, 0x97B6C0, 0x956B80, 0x94DF40, 0x90D100, 0x9165C0, 0x93B880, 0x920C40
};

/**
 * @brief 计算BLE CRC24 / Calculate BLE CRC24
 * @param data 要计算CRC的数据 / Data to calculate CRC for
 * @param len 数据长度 / Data length
 * @param crc_init CRC初始值 / CRC initial value
 * @return 24位CRC值 / 24-bit CRC value
 * 
 * @details 使用查找表方法快速计算BLE CRC24
 *          Uses lookup table method for fast BLE CRC24 calculation
 */
uint32_t ble_ll_calculate_crc24(uint8_t* data, uint16_t len, uint32_t crc_init)
{
    uint32_t crc = crc_init & 0xFFFFFF;  // 确保初始值为24位 / Ensure initial value is 24-bit
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t tbl_idx = ((crc >> 16) ^ data[i]) & 0xFF;      // 计算查找表索引 / Calculate lookup table index
        crc = ((crc << 8) ^ crc24_table[tbl_idx]) & 0xFFFFFF;  // 更新CRC并保持24位 / Update CRC and keep 24-bit
    }
    
    return crc;
}

/**
 * @brief 扫描特定设备 / Scan for specific device
 * @param ctx 连接上下文 / Connection context
 * @param target_addr 目标设备地址 / Target device address
 * @return true如果找到设备，false否则 / true if device found, false otherwise
 */
bool ll_scan_for_device(ble_conn_context_t* ctx, uint8_t* target_addr)
{
    uint8_t rx_buffer[255];
    uint8_t rx_len;
    
    /* 检查是否有接收到的数据 / Check if data received */
    if (sx128x_get_irq_status(ctx->radio_context) & SX128X_IRQ_RX_DONE) {
        /* 读取数据 / Read data */
        sx128x_get_rx_buffer_status(ctx->radio_context, &rx_len, NULL);
        sx128x_read_buffer(ctx->radio_context, 0x80, rx_buffer, rx_len);
        
        /* 清除中断 / Clear interrupt */
        sx128x_clear_irq_status(ctx->radio_context, SX128X_IRQ_RX_DONE);
        
        /* 解析广播包 / Parse advertising packet */
        ble_adv_pdu_t* adv = (ble_adv_pdu_t*)rx_buffer;
        
        /* 检查PDU类型 / Check PDU type */
        if ((adv->header.type == BLE_PDU_ADV_IND ||          // 可连接非定向广播 / Connectable undirected advertising
             adv->header.type == BLE_PDU_ADV_DIRECT_IND ||   // 可连接定向广播 / Connectable directed advertising
             adv->header.type == BLE_PDU_ADV_SCAN_IND) &&    // 可扫描非定向广播 / Scannable undirected advertising
            adv->header.length >= 6) {                       // 至少包含地址 / At least contains address
            
            /* 比较地址 / Compare address */
            if (memcmp(&adv->payload[0], target_addr, 6) == 0) {
                /* 保存RSSI / Save RSSI */
                sx128x_pkt_status_ble_t pkt_status;
                sx128x_get_ble_pkt_status(ctx->radio_context, &pkt_status);
                ctx->last_rssi = pkt_status.rssi_sync;  // 保存信号强度 / Save signal strength
                
                return true;
            }
        }
        
        /* 继续扫描 / Continue scanning */
        sx128x_set_rx(ctx->radio_context);
    }
    
    /* 切换广播信道 / Switch advertising channel */
    static uint8_t adv_channel_idx = 0;      // 广播信道索引(0-2) / Advertising channel index (0-2)
    static uint32_t last_channel_switch = 0; // 上次切换时间 / Last switch time
    uint32_t now = HAL_GetTick();
    
    if (now - last_channel_switch > 10) {  /* 每10ms切换信道 / Switch channel every 10ms */
        last_channel_switch = now;
        adv_channel_idx = (adv_channel_idx + 1) % 3;  // 循环遍历3个广播信道 / Cycle through 3 advertising channels
        
        uint8_t channel = 37 + adv_channel_idx;        // 信道 37, 38, 39 / Channels 37, 38, 39
        uint32_t freq = ble_ll_get_frequency(channel); // 获取频率 / Get frequency
        
        sx128x_set_rf_freq(ctx->radio_context, freq);
        sx128x_set_gfsk_ble_whitening_seed(ctx->radio_context, channel | 0x40);
        sx128x_set_rx(ctx->radio_context);             // 重新进入接收模式 / Re-enter receive mode
    }
    
    return false;
}

/**
 * @brief 发送Link Layer PDU / Send Link Layer PDU
 * @param ctx 连接上下文 / Connection context
 * @param pdu PDU数据 / PDU data
 * @param len PDU长度 / PDU length
 * @return 操作状态 / Operation status
 */
ble_status_t ll_send_pdu(ble_conn_context_t* ctx, void* pdu, uint16_t len)
{
    /* 等待合适的时机发送 / Wait for appropriate time to send */
    sx128x_set_standby(ctx->radio_context, SX128X_STANDBY_RC);  // 切换到待机模式 / Switch to standby mode
    
    /* 写入数据 / Write data */
    sx128x_set_buffer_base_address(ctx->radio_context, 0x00, 0x80);  // 设置缓冲区基地址 / Set buffer base address
    sx128x_write_buffer(ctx->radio_context, 0x00, (uint8_t*)pdu, len);
    
    /* 发送 / Transmit */
    sx128x_set_tx(ctx->radio_context);
    
    /* 等待发送完成 / Wait for transmission complete */
    uint32_t timeout = 1000;  // 10ms超时 / 10ms timeout
    while (timeout-- && !(sx128x_get_irq_status(ctx->radio_context) & SX128X_IRQ_TX_DONE)) {
        ble_ll_delay_us(10);  // 延时10微秒 / Delay 10 microseconds
    }
    
    if (timeout == 0) {
        return BLE_STATUS_TIMEOUT;
    }
    
    sx128x_clear_irq_status(ctx->radio_context, SX128X_IRQ_TX_DONE);
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发送连接请求 / Send connection request
 * @param ctx 连接上下文 / Connection context
 * @param req 连接请求结构 / Connection request structure
 * @return 操作状态 / Operation status
 */
ble_status_t ll_send_connect_request(ble_conn_context_t* ctx, ll_connect_req_t* req)
{
    /* 计算CRC / Calculate CRC */
    uint32_t crc = ble_ll_calculate_crc24((uint8_t*)req, sizeof(ll_connect_req_t) - 3, 
                                          BLE_CRC_INIT_ADV);  // 使用广播CRC初始值 / Use advertising CRC init value
    
    /* 添加CRC到PDU / Add CRC to PDU */
    uint8_t pdu_with_crc[sizeof(ll_connect_req_t) + 3];
    memcpy(pdu_with_crc, req, sizeof(ll_connect_req_t));
    pdu_with_crc[sizeof(ll_connect_req_t)] = crc & 0xFF;             // CRC低字节 / CRC low byte
    pdu_with_crc[sizeof(ll_connect_req_t) + 1] = (crc >> 8) & 0xFF;  // CRC中字节 / CRC middle byte
    pdu_with_crc[sizeof(ll_connect_req_t) + 2] = (crc >> 16) & 0xFF; // CRC高字节 / CRC high byte
    
    /* 发送PDU / Send PDU */
    return ll_send_pdu(ctx, pdu_with_crc, sizeof(pdu_with_crc));
}

/**
 * @brief 检查是否有待发送的数据 / Check if has data to transmit
 * @param ctx 连接上下文 / Connection context
 * @return true如果有待发送数据，false否则 / true if has pending data, false otherwise
 */
bool ll_has_tx_data(ble_conn_context_t* ctx)
{
    return ctx->tx_pending;
}

/**
 * @brief 准备TX PDU / Prepare TX PDU
 * @param ctx 连接上下文 / Connection context
 * @param pdu 要准备的PDU / PDU to prepare
 */
void ll_prepare_tx_pdu(ble_conn_context_t* ctx, ble_data_pdu_t* pdu)
{
    if (ctx->tx_pending) {
        /* 使用已准备的数据 / Use prepared data */
        memcpy(pdu, ctx->tx_buffer, ctx->tx_length);
    } else {
        /* 准备空PDU / Prepare empty PDU */
        pdu->llid = 0x01;  /* LL控制PDU / LL Control PDU */
        pdu->nesn = ctx->next_expected_seq_num;  // 下一个期望序列号 / Next expected sequence number
        pdu->sn = ctx->tx_seq_num;                // 发送序列号 / Transmit sequence number
        pdu->md = 0;                              // 无更多数据 / No more data
        pdu->length = 0;                          // 空PDU / Empty PDU
    }
}

/**
 * @brief 处理接收的PDU / Process received PDU
 * @param ctx 连接上下文 / Connection context
 * @param pdu 接收到的PDU / Received PDU
 */
void ll_process_rx_pdu(ble_conn_context_t* ctx, ble_data_pdu_t* pdu)
{
    /* 检查序列号 / Check sequence number */
    if (pdu->sn != ctx->next_expected_seq_num) {
        /* 重复的包，忽略 / Duplicate packet, ignore */
        return;
    }
    
    /* 更新期望的序列号 / Update expected sequence number */
    ctx->next_expected_seq_num ^= 1;  // 翻转序列号 / Toggle sequence number
    
    /* 根据LLID处理不同类型的PDU / Process different PDU types based on LLID */
    switch (pdu->llid) {
        case 0x01:  /* LL控制PDU / LL Control PDU */
            ll_process_control_pdu(ctx, pdu->payload, pdu->length);
            break;
            
        case 0x02:  /* L2CAP开始 / L2CAP start */
        case 0x03:  /* L2CAP继续 / L2CAP continuation */
            /* 提取L2CAP数据 / Extract L2CAP data */
            if (pdu->length >= 4) {
                uint16_t l2cap_len = pdu->payload[0] | (pdu->payload[1] << 8);  // L2CAP长度 / L2CAP length
                uint16_t l2cap_cid = pdu->payload[2] | (pdu->payload[3] << 8);  // L2CAP通道ID / L2CAP channel ID
                
                if (l2cap_cid == L2CAP_CID_ATT && ctx->on_data_received) {
                    /* ATT数据，传递给上层 / ATT data, pass to upper layer */
                    ctx->on_data_received(ctx, &pdu->payload[4], l2cap_len);
                }
            }
            break;
    }
    
    /* 检查对方的确认 / Check peer's acknowledgment */
    if (pdu->nesn != ctx->tx_seq_num) {
        /* 对方确认了我们的数据 / Peer acknowledged our data */
        ctx->tx_seq_num ^= 1;      // 翻转发送序列号 / Toggle transmit sequence number
        ctx->tx_pending = false;   // 清除发送等待标志 / Clear pending flag
    }
}

/**
 * @brief 处理LL控制PDU / Process LL control PDU
 * @param ctx 连接上下文 / Connection context
 * @param data PDU数据 / PDU data
 * @param len 数据长度 / Data length
 */
void ll_process_control_pdu(ble_conn_context_t* ctx, uint8_t* data, uint8_t len)
{
    if (len == 0) return;
    
    uint8_t opcode = data[0];
    
    switch (opcode) {
        case LL_TERMINATE_IND:
            if (len >= 2) {
                ctx->conn_state = CONN_STATE_IDLE;
                if (ctx->on_disconnected) {
                    ctx->on_disconnected(ctx, data[1]);
                }
            }
            break;
            
        case LL_VERSION_IND:
            /* 忽略版本信息 / Ignore version information */
            break;
            
        case LL_FEATURE_REQ:
            /* 回复Feature Response / Reply with Feature Response */
            {
                uint8_t feature_rsp[9] = {
                    0x03,  /* LLID = 控制PDU / LLID = Control PDU */
                    0x09,  /* 长度 / Length */
                    LL_FEATURE_RSP,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 无特性 / No features */
                };
                ctx->tx_buffer[0] = feature_rsp[0];
                memcpy(&ctx->tx_buffer[1], &feature_rsp[1], 9);
                ctx->tx_length = 10;
                ctx->tx_pending = true;
            }
            break;
            
        case LL_UNKNOWN_RSP:
            /* 对方不识别我们的请求 / Peer doesn't recognize our request */
            break;
            
        default:
            /* 未知的控制PDU，回复LL_UNKNOWN_RSP / Unknown control PDU, reply with LL_UNKNOWN_RSP */
            {
                uint8_t unknown_rsp[3] = {
                    0x03,  /* LLID = 控制PDU / LLID = Control PDU */
                    0x02,  /* 长度 / Length */
                    LL_UNKNOWN_RSP,
                    opcode  // 未知的操作码 / Unknown opcode
                };
                memcpy(ctx->tx_buffer, unknown_rsp, 4);
                ctx->tx_length = 4;
                ctx->tx_pending = true;
            }
            break;
    }
}

/**
 * @brief 发送空PDU（保持连接） / Send empty PDU (keep connection alive)
 * @param ctx 连接上下文 / Connection context
 * @return 操作状态 / Operation status
 */
ble_status_t ble_send_empty_pdu(ble_conn_context_t* ctx)
{
    if (ctx->conn_state != CONN_STATE_CONNECTED) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    /* 构造空的LL数据PDU / Construct empty LL data PDU */
    ctx->tx_buffer[0] = 0x01;  /* LLID = LL控制PDU(空) / LLID = LL Control PDU (empty) */
    ctx->tx_buffer[1] = 0x00;  /* 长度 = 0 / Length = 0 */
    ctx->tx_length = 2;
    ctx->tx_pending = true;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发送L2CAP帧 / Send L2CAP frame
 * @param ctx 连接上下文 / Connection context
 * @param frame L2CAP帧 / L2CAP frame
 * @return 操作状态 / Operation status
 */
ble_status_t ll_send_l2cap_frame(ble_conn_context_t* ctx, l2cap_frame_t* frame)
{
    if (ctx->conn_state != CONN_STATE_CONNECTED) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    if (ctx->tx_pending) {
        return BLE_STATUS_BUSY;
    }
    
    /* 构造LL数据PDU / Construct LL data PDU */
    ctx->tx_buffer[0] = 0x02;  /* LLID = L2CAP数据 / LLID = L2CAP data */
    ctx->tx_buffer[1] = frame->header.length + 4;  /* LL长度 = L2CAP头 + 数据 / LL length = L2CAP header + data */
    
    /* 复制L2CAP头 / Copy L2CAP header */
    ctx->tx_buffer[2] = frame->header.length & 0xFF;         // L2CAP长度低字节 / L2CAP length low byte
    ctx->tx_buffer[3] = (frame->header.length >> 8) & 0xFF;  // L2CAP长度高字节 / L2CAP length high byte
    ctx->tx_buffer[4] = frame->header.cid & 0xFF;            // 通道ID低字节 / Channel ID low byte
    ctx->tx_buffer[5] = (frame->header.cid >> 8) & 0xFF;     // 通道ID高字节 / Channel ID high byte
    
    /* 复制L2CAP数据 / Copy L2CAP data */
    memcpy(&ctx->tx_buffer[6], frame->payload, frame->header.length);
    
    ctx->tx_length = frame->header.length + 6;
    ctx->tx_pending = true;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 验证接入地址 / Validate access address
 * @param aa 要验证的接入地址 / Access address to validate
 * @return true如果有效，false否则 / true if valid, false otherwise
 * 
 * @details 根据BLE规范验证接入地址的有效性
 *          Validates access address according to BLE specification
 */
bool ll_validate_access_address(uint32_t aa)
{
    /* 1. 不能是广播接入地址 / Cannot be advertising access address */
    if (aa == BLE_ACCESS_ADDRESS_ADV) {
        return false;
    }
    
    /* 2. 不能有超过6个连续的0或1 / Cannot have more than 6 consecutive 0s or 1s */
    uint32_t temp = aa;
    uint8_t consecutive_ones = 0;
    uint8_t consecutive_zeros = 0;
    uint8_t max_consecutive = 0;
    
    for (int i = 0; i < 32; i++) {
        if (temp & 1) {
            consecutive_ones++;
            consecutive_zeros = 0;
        } else {
            consecutive_zeros++;
            consecutive_ones = 0;
        }
        
        if (consecutive_ones > max_consecutive) {
            max_consecutive = consecutive_ones;
        }
        if (consecutive_zeros > max_consecutive) {
            max_consecutive = consecutive_zeros;
        }
        
        if (max_consecutive > 6) {
            return false;
        }
        
        temp >>= 1;
    }
    
    /* 3. 至少有3个0到1或1到0的跳变 / Must have at least 3 transitions from 0 to 1 or 1 to 0 */
    uint8_t transitions = 0;
    uint8_t prev_bit = aa & 1;
    temp = aa >> 1;
    
    for (int i = 1; i < 32; i++) {
        uint8_t current_bit = temp & 1;
        if (current_bit != prev_bit) {
            transitions++;
        }
        prev_bit = current_bit;
        temp >>= 1;
    }
    
    if (transitions < 3) {
        return false;
    }
    
    /* 4. 最后6位至少有2个跳变 / Last 6 bits must have at least 2 transitions */
    uint8_t last_six_transitions = 0;
    prev_bit = (aa >> 26) & 1;
    for (int i = 27; i < 32; i++) {
        uint8_t current_bit = (aa >> i) & 1;
        if (current_bit != prev_bit) {
            last_six_transitions++;
        }
        prev_bit = current_bit;
    }
    
    if (last_six_transitions < 2) {
        return false;
    }
    
    return true;
}

/**
 * @brief 获取频率 / Get frequency
 * @param channel 信道号(0-39) / Channel number (0-39)
 * @return 频率(Hz) / Frequency in Hz
 */
uint32_t ble_ll_get_frequency(uint8_t channel)
{
    if (channel <= 39) {
        if (channel <= 36) {
            /* 数据信道 0-36 / Data channels 0-36 */
            return 2402000000 + (channel * 2000000);  // 2MHz间隔 / 2MHz spacing
        } else {
            /* 广播信道 37-39 / Advertising channels 37-39 */
            switch (channel) {
                case 37: return 2402000000;  // 2402 MHz
                case 38: return 2426000000;  // 2426 MHz
                case 39: return 2480000000;  // 2480 MHz
                default: return 2402000000;
            }
        }
    }
    return 2402000000;
}