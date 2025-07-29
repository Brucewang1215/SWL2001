/**
 * @file    ble_gatt.h
 * @brief   简化的GATT客户端接口（专门用于手环通信） / Simplified GATT Client Interface (for bracelet communication)
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件实现了简化的GATT客户端功能，专门用于与BLE手环通信
 *          This file implements simplified GATT client functionality specifically for BLE bracelet communication
 *          
 *          特点 / Features:
 *          - 硬编码的手环句柄 / Hardcoded bracelet handles
 *          - 简化的服务发现 / Simplified service discovery
 *          - 文本消息发送 / Text message sending
 *          - 基本ATT操作 / Basic ATT operations
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H

#include "ble_defs.h"
#include "ble_ll.h"

/* GATT/ATT错误码 / GATT/ATT Error Codes */
#define ATT_ERROR_INVALID_HANDLE           0x01  // 无效句柄 / Invalid handle
#define ATT_ERROR_READ_NOT_PERMITTED       0x02  // 读取不允许 / Read not permitted
#define ATT_ERROR_WRITE_NOT_PERMITTED      0x03  // 写入不允许 / Write not permitted
#define ATT_ERROR_INVALID_PDU              0x04  // 无效PDU / Invalid PDU
#define ATT_ERROR_INSUFFICIENT_AUTH        0x05  // 认证不足 / Insufficient authentication
#define ATT_ERROR_REQUEST_NOT_SUPPORTED    0x06  // 请求不支持 / Request not supported
#define ATT_ERROR_INVALID_OFFSET           0x07  // 无效偏移 / Invalid offset
#define ATT_ERROR_INSUFFICIENT_AUTHOR      0x08  // 授权不足 / Insufficient authorization
#define ATT_ERROR_PREPARE_QUEUE_FULL       0x09  // 准备队列已满 / Prepare queue full
#define ATT_ERROR_ATTRIBUTE_NOT_FOUND      0x0A  // 属性未找到 / Attribute not found
#define ATT_ERROR_ATTRIBUTE_NOT_LONG       0x0B  // 属性不是长属性 / Attribute not long
#define ATT_ERROR_INSUFFICIENT_KEY_SIZE    0x0C  // 密钥长度不足 / Insufficient encryption key size
#define ATT_ERROR_INVALID_ATTRIBUTE_LEN    0x0D  // 无效属性长度 / Invalid attribute value length
#define ATT_ERROR_UNLIKELY_ERROR           0x0E  // 不太可能的错误 / Unlikely error
#define ATT_ERROR_INSUFFICIENT_ENCRYPTION  0x0F  // 加密不足 / Insufficient encryption
#define ATT_ERROR_UNSUPPORTED_GROUP_TYPE   0x10  // 不支持的组类型 / Unsupported group type
#define ATT_ERROR_INSUFFICIENT_RESOURCES   0x11  // 资源不足 / Insufficient resources

/* UUID定义 / UUID Definitions */
#define UUID_PRIMARY_SERVICE               0x2800  // 主服务 / Primary Service
#define UUID_SECONDARY_SERVICE             0x2801  // 次服务 / Secondary Service
#define UUID_INCLUDE                       0x2802  // 包含服务 / Include Service
#define UUID_CHARACTERISTIC                0x2803  // 特征 / Characteristic
#define UUID_CHAR_USER_DESCRIPTION         0x2901  // 特征用户描述 / Characteristic User Description
#define UUID_CHAR_CLIENT_CONFIG            0x2902  // 客户端特征配置(CCCD) / Client Characteristic Configuration Descriptor
#define UUID_CHAR_SERVER_CONFIG            0x2903  // 服务端特征配置 / Server Characteristic Configuration
#define UUID_CHAR_FORMAT                   0x2904  // 特征格式 / Characteristic Presentation Format
#define UUID_CHAR_AGGREGATE_FORMAT         0x2905  // 特征聚合格式 / Characteristic Aggregate Format

/* 常见服务UUID（16位） / Common Service UUIDs (16-bit) */
#define UUID_GENERIC_ACCESS_SERVICE        0x1800  // 通用访问服务 / Generic Access Service
#define UUID_GENERIC_ATTRIBUTE_SERVICE     0x1801  // 通用属性服务 / Generic Attribute Service
#define UUID_DEVICE_INFO_SERVICE           0x180A  // 设备信息服务 / Device Information Service
#define UUID_BATTERY_SERVICE               0x180F  // 电池服务 / Battery Service
#define UUID_HEART_RATE_SERVICE            0x180D  // 心率服务 / Heart Rate Service
#define UUID_NORDIC_UART_SERVICE           0xFFE0  // Nordic UART服务 / Nordic UART Service

/* 手环相关句柄（硬编码） / Bracelet Related Handles (Hardcoded) */
typedef struct {
    uint16_t service_handle;    // 服务句柄 / Service handle
    uint16_t tx_char_handle;    // 发送特征句柄(手环接收) / TX characteristic handle (bracelet receives)
    uint16_t rx_char_handle;    // 接收特征句柄(手环发送) / RX characteristic handle (bracelet sends)
    uint16_t cccd_handle;       // CCCD句柄(用于启用通知) / CCCD handle (for enabling notifications)
} gatt_bracelet_handles_t;

/* GATT客户端上下文 / GATT Client Context */
typedef struct {
    ble_conn_context_t* ll_ctx;              // Link Layer上下文 / Link Layer context
    uint16_t mtu;                            // 最大传输单元 / Maximum Transmission Unit
    uint8_t pending_op;                      // 等待中的操作码 / Pending operation code
    uint16_t pending_handle;                 // 等待中的句柄 / Pending handle
    uint8_t response_buffer[ATT_MTU_MAX];    // 响应缓冲区 / Response buffer
    uint16_t response_length;                // 响应长度 / Response length
    bool response_received;                  // 响应接收标志 / Response received flag
    bracelet_type_t bracelet_type;          // 手环类型 / Bracelet type
    gatt_bracelet_handles_t handles;         // 手环句柄 / Bracelet handles
} gatt_client_context_t;

/* ATT请求/响应结构 / ATT Request/Response Structure */
typedef struct {
    uint8_t opcode;     // 操作码 / Operation code
    union {
        // 按类型读取请求 / Read By Type Request
        struct {
            uint16_t starting_handle;   // 起始句柄 / Starting handle
            uint16_t ending_handle;     // 结束句柄 / Ending handle
            uint16_t uuid16;            // 16位UUID / 16-bit UUID
        } read_by_type;
        
        // 读取请求 / Read Request
        struct {
            uint16_t handle;            // 属性句柄 / Attribute handle
        } read;
        
        // 写入请求 / Write Request
        struct {
            uint16_t handle;                        // 属性句柄 / Attribute handle
            uint8_t value[ATT_MTU_MAX - 3];       // 属性值 / Attribute value
        } write;
        
        // 错误响应 / Error Response
        struct {
            uint8_t req_opcode;         // 请求操作码 / Request opcode
            uint16_t handle;            // 属性句柄 / Attribute handle
            uint8_t error_code;         // 错误码 / Error code
        } error;
        
        // 通知/指示 / Notification/Indication
        struct {
            uint16_t handle;                        // 属性句柄 / Attribute handle
            uint8_t value[ATT_MTU_MAX - 3];       // 属性值 / Attribute value
        } notification;
    } params;
} att_msg_t;

/* GATT API函数 / GATT API Functions */

// 初始化GATT客户端 / Initialize GATT client
ble_status_t ble_gatt_init(gatt_client_context_t* ctx, ble_conn_context_t* ll_ctx);

// 发现手环类型 / Discover bracelet type
ble_status_t ble_gatt_discover_bracelet(gatt_client_context_t* ctx, bracelet_type_t* type);

// 发送文本消息到手环 / Send text message to bracelet
ble_status_t ble_gatt_write_text(gatt_client_context_t* ctx, const char* text);

// 写入数据到指定句柄 / Write data to specified handle
ble_status_t ble_gatt_write_data(gatt_client_context_t* ctx, uint16_t handle, uint8_t* data, uint16_t len);

// 从指定句柄读取数据 / Read data from specified handle
ble_status_t ble_gatt_read_data(gatt_client_context_t* ctx, uint16_t handle, uint8_t* data, uint16_t* len);

// 启用通知 / Enable notifications
ble_status_t ble_gatt_enable_notifications(gatt_client_context_t* ctx, uint16_t char_handle);

// 处理接收到的数据 / Handle received data
void ble_gatt_handle_rx_data(gatt_client_context_t* ctx, uint8_t* data, uint16_t len);

/* 内部函数 / Internal Functions */

// 发送ATT请求 / Send ATT request
ble_status_t gatt_send_att_request(gatt_client_context_t* ctx, att_msg_t* req);

// 等待ATT响应 / Wait for ATT response
ble_status_t gatt_wait_att_response(gatt_client_context_t* ctx, uint8_t expected_opcode, uint32_t timeout_ms);

// 构建写入请求 / Build write request
void gatt_build_write_request(att_msg_t* msg, uint16_t handle, uint8_t* value, uint16_t len);

// 构建读取请求 / Build read request
void gatt_build_read_request(att_msg_t* msg, uint16_t handle);

// 处理通知 / Process notification
void gatt_process_notification(gatt_client_context_t* ctx, att_msg_t* msg);

/* 手环特定函数 / Bracelet Specific Functions */

// 获取手环句柄 / Get bracelet handles
const gatt_bracelet_handles_t* gatt_get_bracelet_handles(bracelet_type_t type);

// 小米手环认证 / Xiaomi bracelet authentication
ble_status_t gatt_authenticate_xiaomi(gatt_client_context_t* ctx);

#endif /* BLE_GATT_H */