/**
 * @file    ble_defs.h
 * @brief   BLE基础定义和数据结构 / BLE basic definitions and data structures
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件包含BLE协议栈的基础定义，包括：
 *          This file contains basic definitions for BLE protocol stack, including:
 *          - BLE时序参数 / BLE timing parameters  
 *          - 物理层参数 / Physical layer parameters
 *          - PDU类型定义 / PDU type definitions
 *          - 控制命令定义 / Control command definitions
 *          - 数据结构定义 / Data structure definitions
 */

#ifndef BLE_DEFS_H
#define BLE_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* BLE时序定义 (单位：微秒) / BLE Timing Definitions (unit: microseconds) */
#define BLE_T_IFS                   150     // 帧间间隔 / Inter Frame Space
#define BLE_T_MSS                   150     // 最小子事件间隔 / Minimum Subevent Space
#define BLE_T_MAFS                  300     // 最小AUX帧间隔 / Minimum AUX Frame Space
#define BLE_WINDOW_WIDENING_US      32      // 每秒32μs的时钟漂移 / 32μs clock drift per second

/* BLE物理层参数 / BLE Physical Layer Parameters */
#define BLE_ACCESS_ADDRESS_ADV      0x8E89BED6  // 广播接入地址 / Advertising Access Address
#define BLE_CRC_INIT_ADV           0x555555     // 广播CRC初始值 / Advertising CRC Init Value
#define BLE_PREAMBLE_LENGTH        1            // 前导码长度(字节) / Preamble Length (bytes)
#define BLE_MAX_PDU_LENGTH         255          // 最大PDU长度 / Maximum PDU Length
#define BLE_MIN_PDU_LENGTH         6            // 最小PDU长度 / Minimum PDU Length

/* BLE频道定义 / BLE Channel Definitions */
#define BLE_NUM_DATA_CHANNELS      37          // 数据信道数量 / Number of data channels
#define BLE_NUM_ADV_CHANNELS       3           // 广播信道数量 / Number of advertising channels
#define BLE_CHANNEL_37             37          // 2402 MHz - 广播信道 / Advertising channel
#define BLE_CHANNEL_38             38          // 2426 MHz - 广播信道 / Advertising channel  
#define BLE_CHANNEL_39             39          // 2480 MHz - 广播信道 / Advertising channel

/* PDU类型定义 / PDU Type Definitions */
#define BLE_PDU_ADV_IND            0x00  // 可连接非定向广播 / Connectable undirected advertising
#define BLE_PDU_ADV_DIRECT_IND     0x01  // 可连接定向广播 / Connectable directed advertising
#define BLE_PDU_ADV_NONCONN_IND    0x02  // 不可连接非定向广播 / Non-connectable undirected advertising
#define BLE_PDU_SCAN_REQ           0x03  // 扫描请求 / Scan request
#define BLE_PDU_SCAN_RSP           0x04  // 扫描响应 / Scan response
#define BLE_PDU_CONNECT_REQ        0x05  // 连接请求 / Connection request
#define BLE_PDU_ADV_SCAN_IND       0x06  // 可扫描非定向广播 / Scannable undirected advertising
#define BLE_PDU_DATA               0x02  // LL数据PDU / LL Data PDU
#define BLE_PDU_CONTROL            0x03  // LL控制PDU / LL Control PDU

/* LL Control PDU Opcodes / LL控制PDU操作码 */
#define LL_CONNECTION_UPDATE_REQ   0x00
#define LL_CHANNEL_MAP_REQ        0x01
#define LL_TERMINATE_IND          0x02
#define LL_ENC_REQ                0x03
#define LL_ENC_RSP                0x04
#define LL_START_ENC_REQ          0x05
#define LL_START_ENC_RSP          0x06
#define LL_UNKNOWN_RSP            0x07
#define LL_FEATURE_REQ            0x08
#define LL_FEATURE_RSP            0x09
#define LL_PAUSE_ENC_REQ          0x0A
#define LL_PAUSE_ENC_RSP          0x0B
#define LL_VERSION_IND            0x0C
#define LL_REJECT_IND             0x0D
#define LL_SLAVE_FEATURE_REQ      0x0E
#define LL_CONNECTION_PARAM_REQ   0x0F
#define LL_CONNECTION_PARAM_RSP   0x10
#define LL_REJECT_EXT_IND         0x11
#define LL_PING_REQ               0x12
#define LL_PING_RSP               0x13
#define LL_LENGTH_REQ             0x14
#define LL_LENGTH_RSP             0x15

/* L2CAP定义 / L2CAP Definitions */
#define L2CAP_CID_ATT             0x0004  // 属性协议通道 / Attribute Protocol Channel
#define L2CAP_CID_LE_SIGNALING    0x0005  // LE信令通道 / LE Signaling Channel  
#define L2CAP_CID_SMP             0x0006  // 安全管理协议通道 / Security Manager Protocol Channel

/* ATT定义 / ATT Definitions */
#define ATT_MTU_DEFAULT           23      // 默认ATT MTU大小 / Default ATT MTU size
#define ATT_MTU_MAX               247     // 最大ATT MTU大小 / Maximum ATT MTU size

/* ATT Opcodes / ATT操作码 */
#define ATT_ERROR_RSP             0x01
#define ATT_EXCHANGE_MTU_REQ      0x02
#define ATT_EXCHANGE_MTU_RSP      0x03
#define ATT_FIND_INFO_REQ         0x04
#define ATT_FIND_INFO_RSP         0x05
#define ATT_FIND_BY_TYPE_REQ      0x06
#define ATT_FIND_BY_TYPE_RSP      0x07
#define ATT_READ_BY_TYPE_REQ      0x08
#define ATT_READ_BY_TYPE_RSP      0x09
#define ATT_READ_REQ              0x0A
#define ATT_READ_RSP              0x0B
#define ATT_READ_BLOB_REQ         0x0C
#define ATT_READ_BLOB_RSP         0x0D
#define ATT_READ_MULTI_REQ        0x0E
#define ATT_READ_MULTI_RSP        0x0F
#define ATT_READ_BY_GROUP_REQ     0x10
#define ATT_READ_BY_GROUP_RSP     0x11
#define ATT_WRITE_REQ             0x12
#define ATT_WRITE_RSP             0x13
#define ATT_WRITE_CMD             0x52
#define ATT_PREPARE_WRITE_REQ     0x16
#define ATT_PREPARE_WRITE_RSP     0x17
#define ATT_EXECUTE_WRITE_REQ     0x18
#define ATT_EXECUTE_WRITE_RSP     0x19
#define ATT_HANDLE_VALUE_NTF      0x1B
#define ATT_HANDLE_VALUE_IND      0x1D
#define ATT_HANDLE_VALUE_CFM      0x1E

/* 错误码定义 / Error Code Definitions */
typedef enum {
    BLE_STATUS_OK = 0,              // 操作成功 / Operation successful
    BLE_STATUS_ERROR,               // 一般错误 / General error
    BLE_STATUS_TIMEOUT,             // 操作超时 / Operation timeout
    BLE_STATUS_NO_MEMORY,           // 内存不足 / Insufficient memory
    BLE_STATUS_INVALID_PARAMS,      // 无效参数 / Invalid parameters
    BLE_STATUS_NOT_CONNECTED,       // 未连接 / Not connected
    BLE_STATUS_ALREADY_CONNECTED,   // 已连接 / Already connected
    BLE_STATUS_BUSY,                // 设备忙 / Device busy
    BLE_STATUS_UNKNOWN_DEVICE,      // 未知设备 / Unknown device
    BLE_STATUS_PROTOCOL_ERROR,      // 协议错误 / Protocol error
} ble_status_t;

/* 连接状态 / Connection States */
typedef enum {
    CONN_STATE_IDLE = 0,            // 空闲状态 / Idle state
    CONN_STATE_SCANNING,            // 扫描状态 / Scanning state
    CONN_STATE_INITIATING,          // 发起连接状态 / Initiating connection state
    CONN_STATE_CONNECTION,          // 连接中状态 / Connecting state
    CONN_STATE_CONNECTED,           // 已连接状态 / Connected state
    CONN_STATE_DISCONNECTING,       // 断开连接中状态 / Disconnecting state
} ble_conn_state_t;

/* BLE地址类型 / BLE Address Types */
typedef enum {
    BLE_ADDR_TYPE_PUBLIC = 0,       // 公共地址 / Public address
    BLE_ADDR_TYPE_RANDOM = 1,       // 随机地址 / Random address
} ble_addr_type_t;

/* BLE角色 / BLE Roles */
typedef enum {
    BLE_ROLE_MASTER = 0,            // 主设备(Central) / Master device (Central)
    BLE_ROLE_SLAVE = 1,             // 从设备(Peripheral) / Slave device (Peripheral)
} ble_role_t;

/* PDU头结构 / PDU Header Structure */
typedef struct {
    uint8_t type     : 4;   // PDU类型 / PDU type
    uint8_t rfu      : 2;   // 保留位 / Reserved for future use
    uint8_t tx_add   : 1;   // 发送地址类型(0=公共,1=随机) / TX address type (0=public, 1=random)
    uint8_t rx_add   : 1;   // 接收地址类型(0=公共,1=随机) / RX address type (0=public, 1=random)
    uint8_t length;         // PDU负载长度 / PDU payload length
} __attribute__((packed)) ble_pdu_header_t;

/* 广播PDU结构 / Advertising PDU Structure */
typedef struct {
    ble_pdu_header_t header;    // PDU头 / PDU header
    uint8_t payload[37];        // 负载(最大37字节) / Payload (max 37 bytes)
} __attribute__((packed)) ble_adv_pdu_t;

/* 数据PDU结构 / Data PDU Structure */
typedef struct {
    uint8_t llid     : 2;   // 链路层标识 / Link Layer ID (01=继续,10=开始,11=控制)
    uint8_t nesn     : 1;   // 下一个期望序列号 / Next Expected Sequence Number
    uint8_t sn       : 1;   // 序列号 / Sequence Number
    uint8_t md       : 1;   // 更多数据标志 / More Data flag
    uint8_t rfu      : 3;   // 保留位 / Reserved for future use
    uint8_t length;         // 负载长度 / Payload length
    uint8_t payload[251];   // 负载(最大251字节) / Payload (max 251 bytes)
} __attribute__((packed)) ble_data_pdu_t;

/* 连接请求结构 / Connection Request Structure */
typedef struct {
    uint8_t header;             // PDU头 / PDU header
    uint8_t length;             // 长度(固定34) / Length (fixed 34)
    uint8_t init_addr[6];       // 发起方地址 / Initiator address
    uint8_t adv_addr[6];        // 广播方地址 / Advertiser address
    uint32_t access_address;    // 连接接入地址 / Connection access address
    uint32_t crc_init : 24;     // CRC初始值(24位) / CRC init value (24 bits)
    uint8_t win_size;           // 窗口大小(*1.25ms) / Window size (*1.25ms)
    uint16_t win_offset;        // 窗口偏移(*1.25ms) / Window offset (*1.25ms)
    uint16_t interval;          // 连接间隔(*1.25ms) / Connection interval (*1.25ms)
    uint16_t latency;           // 从设备延迟 / Slave latency
    uint16_t timeout;           // 监督超时(*10ms) / Supervision timeout (*10ms)
    uint8_t channel_map[5];     // 信道图(37个信道) / Channel map (37 channels)
    uint8_t hop : 5;            // 跳频增量(5-16) / Hop increment (5-16)
    uint8_t sca : 3;            // 睡眠时钟精度 / Sleep Clock Accuracy
} __attribute__((packed)) ll_connect_req_t;

/* L2CAP头结构 / L2CAP Header Structure */
typedef struct {
    uint16_t length;    // 数据长度 / Data length
    uint16_t cid;       // 通道标识符 / Channel Identifier
} __attribute__((packed)) l2cap_header_t;

/* L2CAP帧结构 / L2CAP Frame Structure */
typedef struct {
    l2cap_header_t header;      // L2CAP头 / L2CAP header
    uint8_t payload[251];       // 负载数据 / Payload data
} __attribute__((packed)) l2cap_frame_t;

/* ATT PDU结构 / ATT PDU Structure */
typedef struct {
    uint8_t opcode;                     // 操作码 / Operation code
    uint8_t params[ATT_MTU_MAX - 1];   // 参数 / Parameters
} __attribute__((packed)) att_pdu_t;

/* 连接参数 / Connection Parameters */
typedef struct {
    uint16_t conn_interval;      // 连接间隔，单位: 1.25ms (范围: 7.5ms - 4s) / Connection interval, unit: 1.25ms (range: 7.5ms - 4s)
    uint16_t slave_latency;      // 从设备延迟 (范围: 0 - 499) / Slave latency (range: 0 - 499)
    uint16_t supervision_timeout; // 监督超时，单位: 10ms (范围: 100ms - 32s) / Supervision timeout, unit: 10ms (range: 100ms - 32s)
} ble_conn_params_t;

/* 设备信息 / Device Information */
typedef struct {
    uint8_t addr[6];            // 设备地址 / Device address
    ble_addr_type_t addr_type;  // 地址类型 / Address type
    int8_t rssi;                // 信号强度 / Signal strength (RSSI)
    uint8_t name[32];           // 设备名称 / Device name
    uint8_t name_len;           // 名称长度 / Name length
} ble_device_info_t;

/* 手环类型定义 / Bracelet Type Definitions */
typedef enum {
    BRACELET_TYPE_UNKNOWN = 0,      // 未知类型 / Unknown type
    BRACELET_TYPE_XIAOMI,           // 小米手环 / Xiaomi bracelet
    BRACELET_TYPE_NORDIC_UART,      // Nordic UART服务手环 / Nordic UART service bracelet
    BRACELET_TYPE_CUSTOM,           // 自定义手环 / Custom bracelet
} bracelet_type_t;

#endif /* BLE_DEFS_H */