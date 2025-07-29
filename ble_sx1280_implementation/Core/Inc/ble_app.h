/**
 * @file    ble_app.h
 * @brief   BLE应用层接口
 * @author  AI Assistant
 * @date    2024
 */

#ifndef BLE_APP_H
#define BLE_APP_H

#include "ble_defs.h"
#include "ble_ll.h"
#include "ble_gatt.h"

/* 应用状态定义 */
typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_IDLE,
    APP_STATE_SCANNING,
    APP_STATE_CONNECTING,
    APP_STATE_CONNECTED,
    APP_STATE_SENDING,
    APP_STATE_DISCONNECTING,
    APP_STATE_ERROR
} app_state_t;

/* 应用事件定义 */
typedef enum {
    APP_EVENT_NONE = 0,
    APP_EVENT_SCAN_RESULT,
    APP_EVENT_CONNECTED,
    APP_EVENT_DISCONNECTED,
    APP_EVENT_DATA_SENT,
    APP_EVENT_DATA_RECEIVED,
    APP_EVENT_ERROR
} app_event_t;

/* 应用配置 */
typedef struct {
    /* 目标设备 */
    uint8_t target_addr[6];
    bracelet_type_t bracelet_type;
    
    /* 连接参数 */
    uint16_t conn_interval_ms;
    uint16_t slave_latency;
    uint16_t supervision_timeout_ms;
    
    /* 扫描参数 */
    uint16_t scan_interval_ms;
    uint16_t scan_window_ms;
    uint32_t scan_timeout_ms;
    
    /* 重试策略 */
    uint8_t max_retry_count;
    uint32_t retry_delay_ms;
    
    /* 功能配置 */
    bool auto_reconnect;
    bool disconnect_after_send;
    bool enable_notifications;
} app_config_t;

/* 应用上下文 */
typedef struct {
    /* 状态管理 */
    app_state_t state;
    app_state_t prev_state;
    app_event_t last_event;
    
    /* 配置 */
    app_config_t config;
    
    /* BLE栈 */
    ble_conn_context_t ble_conn;
    gatt_client_context_t gatt_client;
    
    /* 数据缓冲 */
    char text_buffer[256];
    bool text_pending;
    uint8_t rx_buffer[256];
    uint16_t rx_length;
    
    /* 统计信息 */
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t connect_time_ms;
    uint8_t retry_count;
    
    /* 时间戳 */
    uint32_t last_activity_time;
    uint32_t state_enter_time;
    
    /* 回调函数 */
    void (*on_connected)(void);
    void (*on_disconnected)(uint8_t reason);
    void (*on_text_sent)(void);
    void (*on_text_received)(const char* text);
    void (*on_error)(ble_status_t error);
} app_context_t;

/* 应用API函数 */
ble_status_t ble_app_init(app_context_t* app, const app_config_t* config);
ble_status_t ble_app_deinit(app_context_t* app);
ble_status_t ble_app_start_scan(app_context_t* app);
ble_status_t ble_app_stop_scan(app_context_t* app);
ble_status_t ble_app_connect(app_context_t* app, uint8_t* addr);
ble_status_t ble_app_disconnect(app_context_t* app);
ble_status_t ble_app_send_text(app_context_t* app, const char* text);
void ble_app_process(app_context_t* app);
app_state_t ble_app_get_state(app_context_t* app);
void ble_app_get_stats(app_context_t* app, uint32_t* sent, uint32_t* received, uint32_t* connect_time);

/* 实用函数 */
const char* ble_app_state_to_string(app_state_t state);
const char* ble_app_event_to_string(app_event_t event);
void ble_app_set_target_device(app_context_t* app, uint8_t* addr, bracelet_type_t type);

/* 默认配置 */
void ble_app_get_default_config(app_config_t* config);

/* 调试函数 */
#ifdef BLE_DEBUG_ENABLED
void ble_app_dump_state(app_context_t* app);
void ble_app_dump_stats(app_context_t* app);
#endif

#endif /* BLE_APP_H */