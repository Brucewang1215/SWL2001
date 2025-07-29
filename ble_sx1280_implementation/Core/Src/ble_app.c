/**
 * @file    ble_app.c
 * @brief   BLE应用层实现
 * @author  AI Assistant
 * @date    2024
 */

#include "ble_app.h"
#include "stm32g0xx_hal.h"
#include <stdio.h>

/* 状态名称字符串 */
static const char* state_names[] = {
    "INIT",
    "IDLE", 
    "SCANNING",
    "CONNECTING",
    "CONNECTED",
    "SENDING",
    "DISCONNECTING",
    "ERROR"
};

/* 事件名称字符串 */
static const char* event_names[] = {
    "NONE",
    "SCAN_RESULT",
    "CONNECTED",
    "DISCONNECTED",
    "DATA_SENT",
    "DATA_RECEIVED",
    "ERROR"
};

/* 内部函数声明 */
static void app_state_transition(app_context_t* app, app_state_t new_state);
static void app_handle_state(app_context_t* app);
static bool app_scan_filter(uint8_t* addr, int8_t rssi, uint8_t* adv_data, uint8_t len);
static void app_on_connected(ble_conn_context_t* ctx);
static void app_on_disconnected(ble_conn_context_t* ctx, uint8_t reason);
static void app_on_data_received(ble_conn_context_t* ctx, uint8_t* data, uint16_t len);

/* 全局应用上下文指针（用于回调） */
static app_context_t* g_app_ctx = NULL;

/**
 * @brief 获取默认配置
 */
void ble_app_get_default_config(app_config_t* config)
{
    memset(config, 0, sizeof(app_config_t));
    
    /* 连接参数 */
    config->conn_interval_ms = 50;        // 50ms
    config->slave_latency = 4;            // 可跳过4个连接事件
    config->supervision_timeout_ms = 5000; // 5秒超时
    
    /* 扫描参数 */
    config->scan_interval_ms = 100;       // 100ms
    config->scan_window_ms = 50;          // 50ms
    config->scan_timeout_ms = 10000;      // 10秒
    
    /* 重试策略 */
    config->max_retry_count = 3;
    config->retry_delay_ms = 1000;
    
    /* 功能配置 */
    config->auto_reconnect = true;
    config->disconnect_after_send = true;
    config->enable_notifications = false;
}

/**
 * @brief 初始化应用
 */
ble_status_t ble_app_init(app_context_t* app, const app_config_t* config)
{
    ble_status_t status;
    
    if (!app || !config) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 清空上下文 */
    memset(app, 0, sizeof(app_context_t));
    
    /* 复制配置 */
    memcpy(&app->config, config, sizeof(app_config_t));
    
    /* 设置全局指针 */
    g_app_ctx = app;
    
    /* 初始化Link Layer */
    status = ble_ll_init(&app->ble_conn, NULL);  // radio_context需要在外部设置
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 设置LL回调 */
    app->ble_conn.on_connected = app_on_connected;
    app->ble_conn.on_disconnected = app_on_disconnected;
    app->ble_conn.on_data_received = app_on_data_received;
    
    /* 初始化GATT客户端 */
    status = ble_gatt_init(&app->gatt_client, &app->ble_conn);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 初始状态 */
    app->state = APP_STATE_IDLE;
    app->state_enter_time = HAL_GetTick();
    
    return BLE_STATUS_OK;
}

/**
 * @brief 反初始化应用
 */
ble_status_t ble_app_deinit(app_context_t* app)
{
    if (!app) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 断开连接 */
    if (app->state == APP_STATE_CONNECTED) {
        ble_app_disconnect(app);
    }
    
    /* 反初始化LL */
    ble_ll_deinit(&app->ble_conn);
    
    /* 清除全局指针 */
    g_app_ctx = NULL;
    
    return BLE_STATUS_OK;
}

/**
 * @brief 开始扫描
 */
ble_status_t ble_app_start_scan(app_context_t* app)
{
    if (!app) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (app->state != APP_STATE_IDLE) {
        return BLE_STATUS_BUSY;
    }
    
    /* 设置扫描参数 */
    ble_scan_params_t scan_params = {
        .scan_interval = app->config.scan_interval_ms * 1.6,  // 转换为0.625ms单位
        .scan_window = app->config.scan_window_ms * 1.6,
        .scan_type = 0,  // 被动扫描
        .filter_duplicates = true
    };
    
    /* 开始扫描 */
    ble_status_t status = ble_ll_start_scanning(&app->ble_conn, 
                                                &scan_params,
                                                app_scan_filter);
    if (status == BLE_STATUS_OK) {
        app_state_transition(app, APP_STATE_SCANNING);
    }
    
    return status;
}

/**
 * @brief 停止扫描
 */
ble_status_t ble_app_stop_scan(app_context_t* app)
{
    if (!app) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (app->state != APP_STATE_SCANNING) {
        return BLE_STATUS_ERROR;
    }
    
    ble_status_t status = ble_ll_stop_scanning(&app->ble_conn);
    if (status == BLE_STATUS_OK) {
        app_state_transition(app, APP_STATE_IDLE);
    }
    
    return status;
}

/**
 * @brief 连接设备
 */
ble_status_t ble_app_connect(app_context_t* app, uint8_t* addr)
{
    if (!app || !addr) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (app->state != APP_STATE_IDLE && app->state != APP_STATE_SCANNING) {
        return BLE_STATUS_BUSY;
    }
    
    /* 保存目标地址 */
    memcpy(app->config.target_addr, addr, 6);
    
    /* 设置连接参数 */
    ble_conn_params_t conn_params = {
        .conn_interval = app->config.conn_interval_ms,
        .slave_latency = app->config.slave_latency,
        .supervision_timeout = app->config.supervision_timeout_ms
    };
    
    /* 发起连接 */
    ble_status_t status = ble_ll_connect(&app->ble_conn, addr, &conn_params);
    if (status == BLE_STATUS_OK) {
        app_state_transition(app, APP_STATE_CONNECTING);
    }
    
    return status;
}

/**
 * @brief 断开连接
 */
ble_status_t ble_app_disconnect(app_context_t* app)
{
    if (!app) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (app->state != APP_STATE_CONNECTED && app->state != APP_STATE_SENDING) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    ble_status_t status = ble_ll_disconnect(&app->ble_conn, 0x13);  // Remote User Terminated
    if (status == BLE_STATUS_OK) {
        app_state_transition(app, APP_STATE_DISCONNECTING);
    }
    
    return status;
}

/**
 * @brief 发送文本
 */
ble_status_t ble_app_send_text(app_context_t* app, const char* text)
{
    if (!app || !text) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (app->state != APP_STATE_CONNECTED) {
        return BLE_STATUS_NOT_CONNECTED;
    }
    
    /* 保存文本 */
    strncpy(app->text_buffer, text, sizeof(app->text_buffer) - 1);
    app->text_buffer[sizeof(app->text_buffer) - 1] = '\0';
    app->text_pending = true;
    
    /* 切换到发送状态 */
    app_state_transition(app, APP_STATE_SENDING);
    
    return BLE_STATUS_OK;
}

/**
 * @brief 处理应用事件（主循环调用）
 */
void ble_app_process(app_context_t* app)
{
    if (!app) return;
    
    /* 处理Link Layer事件 */
    ble_ll_process_events(&app->ble_conn);
    
    /* 处理应用状态 */
    app_handle_state(app);
}

/**
 * @brief 获取当前状态
 */
app_state_t ble_app_get_state(app_context_t* app)
{
    return app ? app->state : APP_STATE_ERROR;
}

/**
 * @brief 获取统计信息
 */
void ble_app_get_stats(app_context_t* app, uint32_t* sent, 
                      uint32_t* received, uint32_t* connect_time)
{
    if (!app) return;
    
    if (sent) *sent = app->packets_sent;
    if (received) *received = app->packets_received;
    if (connect_time) *connect_time = app->connect_time_ms;
}

/**
 * @brief 设置目标设备
 */
void ble_app_set_target_device(app_context_t* app, uint8_t* addr, bracelet_type_t type)
{
    if (!app || !addr) return;
    
    memcpy(app->config.target_addr, addr, 6);
    app->config.bracelet_type = type;
}

/**
 * @brief 状态转换
 */
static void app_state_transition(app_context_t* app, app_state_t new_state)
{
    if (app->state != new_state) {
        app->prev_state = app->state;
        app->state = new_state;
        app->state_enter_time = HAL_GetTick();
        
        printf("[APP] State: %s -> %s\n", 
               state_names[app->prev_state], 
               state_names[app->state]);
    }
}

/**
 * @brief 处理状态机
 */
static void app_handle_state(app_context_t* app)
{
    uint32_t time_in_state = HAL_GetTick() - app->state_enter_time;
    
    switch (app->state) {
        case APP_STATE_INIT:
            /* 初始化完成后自动进入IDLE */
            app_state_transition(app, APP_STATE_IDLE);
            break;
            
        case APP_STATE_IDLE:
            /* 等待命令 */
            break;
            
        case APP_STATE_SCANNING:
            /* 检查扫描超时 */
            if (time_in_state > app->config.scan_timeout_ms) {
                printf("[APP] Scan timeout\n");
                ble_app_stop_scan(app);
            }
            break;
            
        case APP_STATE_CONNECTING:
            /* 检查连接超时 */
            if (time_in_state > 5000) {  // 5秒超时
                printf("[APP] Connection timeout\n");
                
                if (++app->retry_count < app->config.max_retry_count) {
                    /* 重试 */
                    HAL_Delay(app->config.retry_delay_ms);
                    app_state_transition(app, APP_STATE_SCANNING);
                } else {
                    /* 失败 */
                    app_state_transition(app, APP_STATE_ERROR);
                    if (app->on_error) {
                        app->on_error(BLE_STATUS_TIMEOUT);
                    }
                }
            }
            break;
            
        case APP_STATE_CONNECTED:
            /* 处理待发送的文本 */
            if (app->text_pending) {
                app_state_transition(app, APP_STATE_SENDING);
            }
            
            /* 定期发送空包保持连接 */
            if (time_in_state > 10000) {  // 10秒
                ble_ll_send_data(&app->ble_conn, NULL, 0);
                app->state_enter_time = HAL_GetTick();
            }
            break;
            
        case APP_STATE_SENDING:
            /* 发送文本 */
            if (app->text_pending) {
                ble_status_t status = ble_gatt_write_text(&app->gatt_client, 
                                                         app->text_buffer);
                if (status == BLE_STATUS_OK) {
                    app->packets_sent++;
                    app->text_pending = false;
                    
                    if (app->on_text_sent) {
                        app->on_text_sent();
                    }
                    
                    /* 根据配置决定是否断开 */
                    if (app->config.disconnect_after_send) {
                        HAL_Delay(100);  // 短延时确保数据发送
                        ble_app_disconnect(app);
                    } else {
                        app_state_transition(app, APP_STATE_CONNECTED);
                    }
                } else {
                    printf("[APP] Send failed: %d\n", status);
                    app_state_transition(app, APP_STATE_ERROR);
                }
            }
            break;
            
        case APP_STATE_DISCONNECTING:
            /* 等待断开完成 */
            if (time_in_state > 1000) {  // 1秒超时
                app_state_transition(app, APP_STATE_IDLE);
            }
            break;
            
        case APP_STATE_ERROR:
            /* 错误恢复 */
            if (time_in_state > 3000) {  // 3秒后重试
                printf("[APP] Recovering from error\n");
                app->retry_count = 0;
                app_state_transition(app, APP_STATE_IDLE);
            }
            break;
    }
}

/**
 * @brief 扫描过滤器回调
 */
static bool app_scan_filter(uint8_t* addr, int8_t rssi, uint8_t* adv_data, uint8_t len)
{
    if (!g_app_ctx) return false;
    
    /* 检查是否是目标设备 */
    if (memcmp(addr, g_app_ctx->config.target_addr, 6) == 0) {
        printf("[APP] Found target device, RSSI: %d dBm\n", rssi);
        return true;
    }
    
    /* 如果没有指定目标，显示所有设备 */
    if (memcmp(g_app_ctx->config.target_addr, "\x00\x00\x00\x00\x00\x00", 6) == 0) {
        printf("[APP] Device: %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %d dBm\n",
               addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], rssi);
    }
    
    return false;
}

/**
 * @brief 连接成功回调
 */
static void app_on_connected(ble_conn_context_t* ctx)
{
    if (!g_app_ctx) return;
    
    printf("[APP] Connected!\n");
    
    g_app_ctx->connect_time_ms = HAL_GetTick();
    g_app_ctx->retry_count = 0;
    
    app_state_transition(g_app_ctx, APP_STATE_CONNECTED);
    
    /* 发现手环服务 */
    bracelet_type_t type;
    if (ble_gatt_discover_bracelet(&g_app_ctx->gatt_client, &type) == BLE_STATUS_OK) {
        printf("[APP] Detected bracelet type: %d\n", type);
        g_app_ctx->config.bracelet_type = type;
    }
    
    /* 启用通知（如果配置） */
    if (g_app_ctx->config.enable_notifications) {
        ble_gatt_enable_notifications(&g_app_ctx->gatt_client,
                                     g_app_ctx->gatt_client.handles.rx_char_handle);
    }
    
    /* 应用回调 */
    if (g_app_ctx->on_connected) {
        g_app_ctx->on_connected();
    }
}

/**
 * @brief 断开连接回调
 */
static void app_on_disconnected(ble_conn_context_t* ctx, uint8_t reason)
{
    if (!g_app_ctx) return;
    
    printf("[APP] Disconnected, reason: 0x%02X\n", reason);
    
    if (g_app_ctx->state == APP_STATE_CONNECTED || 
        g_app_ctx->state == APP_STATE_SENDING) {
        g_app_ctx->connect_time_ms = HAL_GetTick() - g_app_ctx->connect_time_ms;
    }
    
    app_state_transition(g_app_ctx, APP_STATE_IDLE);
    
    /* 应用回调 */
    if (g_app_ctx->on_disconnected) {
        g_app_ctx->on_disconnected(reason);
    }
    
    /* 自动重连（如果配置） */
    if (g_app_ctx->config.auto_reconnect && reason != 0x13) {  // 非用户主动断开
        HAL_Delay(g_app_ctx->config.retry_delay_ms);
        ble_app_start_scan(g_app_ctx);
    }
}

/**
 * @brief 数据接收回调
 */
static void app_on_data_received(ble_conn_context_t* ctx, uint8_t* data, uint16_t len)
{
    if (!g_app_ctx) return;
    
    /* 处理GATT数据 */
    ble_gatt_handle_rx_data(&g_app_ctx->gatt_client, data, len);
    
    g_app_ctx->packets_received++;
    
    /* 保存接收的数据 */
    if (len <= sizeof(g_app_ctx->rx_buffer)) {
        memcpy(g_app_ctx->rx_buffer, data, len);
        g_app_ctx->rx_length = len;
        
        /* 尝试作为文本处理 */
        g_app_ctx->rx_buffer[len] = '\0';
        
        /* 应用回调 */
        if (g_app_ctx->on_text_received) {
            g_app_ctx->on_text_received((char*)g_app_ctx->rx_buffer);
        }
    }
}

/**
 * @brief 状态转字符串
 */
const char* ble_app_state_to_string(app_state_t state)
{
    if (state < sizeof(state_names) / sizeof(state_names[0])) {
        return state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief 事件转字符串
 */
const char* ble_app_event_to_string(app_event_t event)
{
    if (event < sizeof(event_names) / sizeof(event_names[0])) {
        return event_names[event];
    }
    return "UNKNOWN";
}

#ifdef BLE_DEBUG_ENABLED
/**
 * @brief 调试：输出状态
 */
void ble_app_dump_state(app_context_t* app)
{
    if (!app) return;
    
    printf("\n=== APP STATE ===\n");
    printf("State: %s\n", ble_app_state_to_string(app->state));
    printf("Time in state: %lu ms\n", HAL_GetTick() - app->state_enter_time);
    printf("Target: %02X:%02X:%02X:%02X:%02X:%02X\n",
           app->config.target_addr[5], app->config.target_addr[4],
           app->config.target_addr[3], app->config.target_addr[2],
           app->config.target_addr[1], app->config.target_addr[0]);
    printf("Bracelet type: %d\n", app->config.bracelet_type);
    printf("Text pending: %s\n", app->text_pending ? "Yes" : "No");
    printf("================\n");
}

/**
 * @brief 调试：输出统计
 */
void ble_app_dump_stats(app_context_t* app)
{
    if (!app) return;
    
    printf("\n=== APP STATS ===\n");
    printf("Packets sent: %lu\n", app->packets_sent);
    printf("Packets received: %lu\n", app->packets_received);
    printf("Connect time: %lu ms\n", app->connect_time_ms);
    printf("Retry count: %u\n", app->retry_count);
    printf("================\n");
}
#endif