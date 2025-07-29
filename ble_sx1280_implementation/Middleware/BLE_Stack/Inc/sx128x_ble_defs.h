/**
 * @file    sx128x_ble_defs.h
 * @brief   SX128x BLE相关定义 / SX128x BLE-related definitions
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件包含了BLE Stack使用的SX128x相关定义
 *          This file contains SX128x-related definitions used by BLE Stack
 */

#ifndef SX128X_BLE_DEFS_H
#define SX128X_BLE_DEFS_H

/* SX128x包类型 / SX128x Packet Types */
#define SX128X_PKT_TYPE_BLE             0x04    // BLE包类型 / BLE packet type

/* SX128x待机模式 / SX128x Standby Modes */
#define SX128X_STANDBY_RC               0x00    // RC振荡器待机 / RC oscillator standby
#define SX128X_STANDBY_XOSC             0x01    // XOSC振荡器待机 / XOSC oscillator standby

/* SX128x中断标志 / SX128x Interrupt Flags */
#define SX128X_IRQ_TX_DONE              0x01    // 发送完成 / TX done
#define SX128X_IRQ_RX_DONE              0x02    // 接收完成 / RX done
#define SX128X_IRQ_SYNC_WORD_VALID      0x04    // 同步字有效 / Sync word valid
#define SX128X_IRQ_SYNC_WORD_ERROR      0x08    // 同步字错误 / Sync word error
#define SX128X_IRQ_HEADER_VALID         0x10    // 头部有效 / Header valid
#define SX128X_IRQ_HEADER_ERROR         0x20    // 头部错误 / Header error
#define SX128X_IRQ_CRC_ERROR            0x40    // CRC错误 / CRC error
#define SX128X_IRQ_CAD_DONE             0x80    // CAD完成 / CAD done
#define SX128X_IRQ_CAD_DETECTED         0x100   // CAD检测到 / CAD detected
#define SX128X_IRQ_RX_TX_TIMEOUT        0x200   // RX/TX超时 / RX/TX timeout

/* BLE调制参数 / BLE Modulation Parameters */
typedef enum {
    SX128X_BLE_BR_1_000_BW_1_2 = 0x0C,    // 1Mbps, 1.2MHz带宽 / 1Mbps, 1.2MHz bandwidth
    SX128X_BLE_BR_0_500_BW_0_6 = 0x08,    // 500kbps, 0.6MHz带宽 / 500kbps, 0.6MHz bandwidth
    SX128X_BLE_BR_0_250_BW_0_3 = 0x06,    // 250kbps, 0.3MHz带宽 / 250kbps, 0.3MHz bandwidth
    SX128X_BLE_BR_0_125_BW_0_3 = 0x04     // 125kbps, 0.3MHz带宽 / 125kbps, 0.3MHz bandwidth
} sx128x_ble_br_bw_t;

/* BLE调制指数 / BLE Modulation Index */
typedef enum {
    SX128X_BLE_MOD_IND_0_50 = 0x00,       // 调制指数0.5 / Modulation index 0.5
    SX128X_BLE_MOD_IND_0_75 = 0x01,       // 调制指数0.75 / Modulation index 0.75
    SX128X_BLE_MOD_IND_1_00 = 0x02,       // 调制指数1.0 / Modulation index 1.0
} sx128x_ble_mod_ind_t;

/* BLE脉冲整形 / BLE Pulse Shaping */
typedef enum {
    SX128X_BLE_PULSE_SHAPE_OFF = 0x00,    // 无脉冲整形 / No pulse shaping
    SX128X_BLE_PULSE_SHAPE_BT_1_0 = 0x10  // BT=1.0脉冲整形 / BT=1.0 pulse shaping
} sx128x_ble_pulse_shape_t;

/* BLE连接状态 / BLE Connection State */
typedef enum {
    SX128X_BLE_SCANNER = 0x00,            // 扫描器模式 / Scanner mode
    SX128X_BLE_ADVERTISER = 0x01,         // 广播器模式 / Advertiser mode
    SX128X_BLE_MASTER = 0x00,             // 主设备模式 / Master mode
    SX128X_BLE_SLAVE = 0x01               // 从设备模式 / Slave mode
} sx128x_ble_con_state_t;

/* BLE CRC类型 / BLE CRC Type */
typedef enum {
    SX128X_BLE_CRC_OFF = 0x00,            // 无CRC / No CRC
    SX128X_BLE_CRC_3B = 0x10              // 3字节CRC / 3-byte CRC
} sx128x_ble_crc_type_t;

/* BLE包类型 / BLE Packet Type */
typedef enum {
    SX128X_BLE_PRBS_9 = 0x00,             // PRBS9序列 / PRBS9 sequence
    SX128X_BLE_PRBS_15 = 0x01,            // PRBS15序列 / PRBS15 sequence
    SX128X_BLE_PRBS_23 = 0x02,            // PRBS23序列 / PRBS23 sequence
    SX128X_BLE_PRBS_31 = 0x03,            // PRBS31序列 / PRBS31 sequence
    SX128X_BLE_CUSTOM = 0x04              // 自定义数据 / Custom data
} sx128x_ble_pkt_type_t;

/* BLE白化 / BLE Whitening */
typedef enum {
    SX128X_BLE_WHITENING_DISABLE = 0x00,  // 禁用白化 / Disable whitening
    SX128X_BLE_WHITENING_ENABLE = 0x08    // 启用白化 / Enable whitening
} sx128x_ble_dc_free_t;

/* BLE调制参数结构 / BLE Modulation Parameters Structure */
typedef struct {
    sx128x_ble_br_bw_t br_bw;             // 比特率和带宽 / Bit rate and bandwidth
    sx128x_ble_mod_ind_t mod_ind;         // 调制指数 / Modulation index
    sx128x_ble_pulse_shape_t pulse_shape; // 脉冲整形 / Pulse shaping
} sx128x_mod_params_ble_t;

/* BLE包参数结构 / BLE Packet Parameters Structure */
typedef struct {
    sx128x_ble_con_state_t con_state;     // 连接状态 / Connection state
    sx128x_ble_crc_type_t crc_type;       // CRC类型 / CRC type
    sx128x_ble_pkt_type_t pkt_type;       // 包类型 / Packet type
    sx128x_ble_dc_free_t dc_free;         // 白化配置 / Whitening configuration
} sx128x_pkt_params_ble_t;

/* BLE包状态结构 / BLE Packet Status Structure */
typedef struct {
    int8_t rssi_sync;                     // 同步RSSI / Sync RSSI
    int8_t rssi_avg;                      // 平均RSSI / Average RSSI
    uint8_t errors;                       // 错误计数 / Error count
    uint8_t status;                       // 状态 / Status
} sx128x_pkt_status_ble_t;

/* 兼容性定义 / Compatibility Definitions */
typedef sx128x_pkt_status_ble_t sx128x_pkt_status_gfsk_flrc_ble_t;

#endif /* SX128X_BLE_DEFS_H */