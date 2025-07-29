/**
 * @file    ble_ll.h
 * @brief   BLE Link Layer接口定义 / BLE Link Layer Interface Definitions
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件定义了BLE链路层(Link Layer)的接口和数据结构
 *          This file defines the interface and data structures for BLE Link Layer
 *          
 *          主要功能 / Main features:
 *          - 连接管理 / Connection management
 *          - 扫描和广播 / Scanning and advertising
 *          - 数据传输 / Data transmission
 *          - 信道管理 / Channel management
 *          - 时序控制 / Timing control
 */

#ifndef BLE_LL_H
#define BLE_LL_H

#include "ble_defs.h"
#include "sx128x_ble_defs.h"
#include "sx128x.h"

/* 前向声明 / Forward Declaration */
typedef struct ble_conn_context_s ble_conn_context_t;

/* 连接上下文结构 / Connection Context Structure */
struct ble_conn_context_s {
    /* 连接状态 / Connection State */
    ble_conn_state_t conn_state;    // 当前连接状态 / Current connection state
    ble_role_t role;                // 设备角色(主/从) / Device role (master/slave)
    
    /* 设备地址 / Device Addresses */
    uint8_t local_addr[6];          // 本地设备地址 / Local device address
    uint8_t peer_addr[6];           // 对端设备地址 / Peer device address
    ble_addr_type_t peer_addr_type; // 对端地址类型 / Peer address type
    
    /* 连接参数 / Connection Parameters */
    uint32_t access_address;        // 连接接入地址 / Connection access address
    uint32_t crc_init;              // CRC初始值 / CRC initial value
    uint16_t conn_interval;         // 连接间隔(微秒) / Connection interval (microseconds)
    uint16_t slave_latency;         // 从设备延迟 / Slave latency
    uint16_t supervision_timeout;   // 监督超时(毫秒) / Supervision timeout (milliseconds)
    
    /* 信道管理 / Channel Management */
    uint8_t channel_map[5];         // 信道图(37个信道位图) / Channel map (37 channel bitmap)
    uint8_t hop_increment;          // 跳频增量(5-16) / Hop increment (5-16)
    uint8_t last_unmapped_channel;  // 上次未映射的信道 / Last unmapped channel
    uint8_t num_used_channels;      // 使用的信道数量 / Number of used channels
    uint8_t current_channel;        // 当前信道 / Current channel
    
    /* 连接事件管理 / Connection Event Management */
    uint32_t event_counter;         // 事件计数器 / Event counter
    uint64_t anchor_point;          // 锚点(微秒时间戳) / Anchor point (microsecond timestamp)
    uint32_t window_widening;       // 窗口展宽(补偿时钟漂移) / Window widening (clock drift compensation)
    
    /* 序列号管理 / Sequence Number Management */
    uint8_t tx_seq_num;             // 发送序列号 / Transmit sequence number
    uint8_t rx_seq_num;             // 接收序列号 / Receive sequence number
    uint8_t next_expected_seq_num;  // 下一个期望的序列号 / Next expected sequence number
    
    /* 数据缓冲 / Data Buffers */
    uint8_t tx_buffer[255];         // 发送缓冲区 / Transmit buffer
    uint16_t tx_length;             // 发送数据长度 / Transmit data length
    bool tx_pending;                // 发送等待标志 / Transmit pending flag
    uint8_t rx_buffer[255];         // 接收缓冲区 / Receive buffer
    uint16_t rx_length;             // 接收数据长度 / Receive data length
    bool rx_pending;                // 接收等待标志 / Receive pending flag
    
    /* 错误统计 / Error Statistics */
    uint32_t consecutive_crc_errors; // 连续CRC错误次数 / Consecutive CRC errors
    uint32_t total_crc_errors;       // 总CRC错误次数 / Total CRC errors
    uint32_t total_timeouts;         // 总超时次数 / Total timeouts
    
    /* 无线电接口 / Radio Interface */
    void* radio_context;            // 无线电模块上下文(SX1280) / Radio module context (SX1280)
    
    /* 应用回调 / Application Callbacks */
    void (*on_connected)(ble_conn_context_t* ctx);                                   // 连接成功回调 / Connection success callback
    void (*on_disconnected)(ble_conn_context_t* ctx, uint8_t reason);               // 断开连接回调 / Disconnection callback
    void (*on_data_received)(ble_conn_context_t* ctx, uint8_t* data, uint16_t len); // 数据接收回调 / Data received callback
    
    /* 性能优化 / Performance Optimization */
    uint8_t max_packets_per_event;  // 每个事件最大包数 / Maximum packets per event
    bool more_data;                 // 更多数据标志(MD) / More data flag (MD)
    
    /* 调试信息 / Debug Information */
    int8_t last_rssi;               // 最后一次RSSI值 / Last RSSI value
    uint8_t last_status;            // 最后一次状态 / Last status
};

/* 扫描参数 / Scan Parameters */
typedef struct {
    uint16_t scan_interval;  // 扫描间隔(单位: 0.625ms) / Scan interval (unit: 0.625ms)
    uint16_t scan_window;    // 扫描窗口(单位: 0.625ms) / Scan window (unit: 0.625ms)
    uint8_t scan_type;       // 扫描类型(0:被动, 1:主动) / Scan type (0: passive, 1: active)
    bool filter_duplicates;  // 是否过滤重复 / Filter duplicates
} ble_scan_params_t;

/* 扫描过滤器回调 / Scan Filter Callback */
typedef bool (*ble_scan_filter_cb)(uint8_t* addr, int8_t rssi, uint8_t* adv_data, uint8_t len);
// 返回true表示设备符合过滤条件 / Return true if device matches filter criteria

/* Link Layer API函数 / Link Layer API Functions */

// 初始化Link Layer / Initialize Link Layer
ble_status_t ble_ll_init(ble_conn_context_t* ctx, void* radio_context);

// 反初始化Link Layer / Deinitialize Link Layer
ble_status_t ble_ll_deinit(ble_conn_context_t* ctx);

// 设置本地设备地址 / Set local device address
ble_status_t ble_ll_set_address(ble_conn_context_t* ctx, uint8_t* addr);

// 开始扫描 / Start scanning
ble_status_t ble_ll_start_scanning(ble_conn_context_t* ctx, ble_scan_params_t* params, ble_scan_filter_cb filter);

// 停止扫描 / Stop scanning
ble_status_t ble_ll_stop_scanning(ble_conn_context_t* ctx);

// 发起连接 / Initiate connection
ble_status_t ble_ll_connect(ble_conn_context_t* ctx, uint8_t* peer_addr, ble_conn_params_t* params);

// 断开连接 / Disconnect
ble_status_t ble_ll_disconnect(ble_conn_context_t* ctx, uint8_t reason);

// 发送数据 / Send data
ble_status_t ble_ll_send_data(ble_conn_context_t* ctx, uint8_t* data, uint16_t len);

// 处理事件(主循环调用) / Process events (called from main loop)
void ble_ll_process_events(ble_conn_context_t* ctx);

// 无线电中断处理 / Radio interrupt handler
void ble_ll_radio_irq_handler(ble_conn_context_t* ctx);

// 连接事件触发 / Connection event trigger
void ble_ll_connection_event_trigger(ble_conn_context_t* ctx);

/* 内部函数 / Internal Functions */

// 获取信道频率 / Get channel frequency
uint32_t ble_ll_get_frequency(uint8_t channel);

// 计算下一个数据信道 / Calculate next data channel
uint8_t ble_ll_calculate_next_channel(ble_conn_context_t* ctx);

// 计算CRC / Calculate CRC
uint32_t ble_ll_calculate_crc(uint8_t* data, uint16_t len, uint32_t crc_init);

// 数据白化 / Data whitening
void ble_ll_whiten_data(uint8_t* data, uint16_t len, uint8_t channel);

// 生成接入地址 / Generate access address
uint32_t ble_ll_generate_access_address(void);

// 生成CRC初始值 / Generate CRC init value
uint32_t ble_ll_generate_crc_init(void);

// 获取随机数 / Get random number
uint8_t ble_ll_get_random(void);

/* 时间管理 / Time Management */

// 获取微秒级时间戳 / Get microsecond timestamp
uint64_t ble_ll_get_timestamp_us(void);

// 微秒级延时 / Microsecond delay
void ble_ll_delay_us(uint32_t us);

// 等待到指定时间戳 / Wait until specified timestamp
void ble_ll_wait_until_us(uint64_t timestamp);

/* 缺失函数的声明(在ble_ll_missing.c中实现) / Missing Function Declarations (implemented in ble_ll_missing.c) */

// 扫描指定设备 / Scan for specific device
bool ll_scan_for_device(ble_conn_context_t* ctx, uint8_t* target_addr);

// 发送PDU / Send PDU
ble_status_t ll_send_pdu(ble_conn_context_t* ctx, void* pdu, uint16_t len);

// 发送连接请求 / Send connection request
ble_status_t ll_send_connect_request(ble_conn_context_t* ctx, ll_connect_req_t* req);

// 检查是否有待发送数据 / Check if has data to transmit
bool ll_has_tx_data(ble_conn_context_t* ctx);

// 准备发送PDU / Prepare TX PDU
void ll_prepare_tx_pdu(ble_conn_context_t* ctx, ble_data_pdu_t* pdu);

// 处理接收的PDU / Process received PDU
void ll_process_rx_pdu(ble_conn_context_t* ctx, ble_data_pdu_t* pdu);

// 处理控制PDU / Process control PDU
void ll_process_control_pdu(ble_conn_context_t* ctx, uint8_t* data, uint8_t len);

// 发送空PDU / Send empty PDU
ble_status_t ble_send_empty_pdu(ble_conn_context_t* ctx);

// 发送L2CAP帧 / Send L2CAP frame
ble_status_t ll_send_l2cap_frame(ble_conn_context_t* ctx, l2cap_frame_t* frame);

// 验证接入地址 / Validate access address
bool ll_validate_access_address(uint32_t aa);

// 计算CRC24 / Calculate CRC24
uint32_t ble_ll_calculate_crc24(uint8_t* data, uint16_t len, uint32_t crc_init);

#endif /* BLE_LL_H */