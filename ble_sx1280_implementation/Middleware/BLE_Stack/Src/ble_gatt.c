/**
 * @file    ble_gatt.c
 * @brief   简化的GATT客户端实现 / Simplified GATT Client Implementation
 * @author  AI Assistant
 * @date    2024
 * 
 * @details 本文件实现了专门用于手环通信的GATT客户端功能
 *          This file implements GATT client functionality specifically for bracelet communication
 *          
 *          主要功能 / Main features:
 *          - 手环类型自动识别 / Automatic bracelet type identification
 *          - 文本消息发送 / Text message sending
 *          - 通知接收处理 / Notification reception handling
 *          - 简化的服务发现 / Simplified service discovery
 */

#include "ble_gatt.h"
#include "stm32g0xx_hal.h"

/* 已知手环的句柄配置 / Known bracelet handle configurations */
static const gatt_bracelet_handles_t bracelet_handles[] = {
    /* 小米手环4/5 / Xiaomi Mi Band 4/5 */
    [BRACELET_TYPE_XIAOMI] = {
        .service_handle = 0x0010,    // 服务句柄 / Service handle
        .tx_char_handle = 0x0016,    // 发送特征句柄(手环接收) / TX characteristic handle (bracelet receives)
        .rx_char_handle = 0x0013,    // 接收特征句柄(手环发送) / RX characteristic handle (bracelet sends)
        .cccd_handle    = 0x0014     // 客户端特征配置描述符句柄 / Client Characteristic Configuration Descriptor handle
    },
    
    /* Nordic UART Service / Nordic UART服务 */
    [BRACELET_TYPE_NORDIC_UART] = {
        .service_handle = 0x000C,    // UART服务句柄 / UART service handle
        .tx_char_handle = 0x000E,    // TX特征句柄 / TX characteristic handle
        .rx_char_handle = 0x0011,    // RX特征句柄 / RX characteristic handle
        .cccd_handle    = 0x0012     // CCCD句柄 / CCCD handle
    },
    
    /* 自定义手环 / Custom bracelet */
    [BRACELET_TYPE_CUSTOM] = {
        .service_handle = 0x0020,    // 自定义服务句柄 / Custom service handle
        .tx_char_handle = 0x0022,    // 自定义TX句柄 / Custom TX handle
        .rx_char_handle = 0x0024,    // 自定义RX句柄 / Custom RX handle
        .cccd_handle    = 0x0025     // 自定义CCCD句柄 / Custom CCCD handle
    }
};

/**
 * @brief 初始化GATT客户端 / Initialize GATT client
 * @param ctx GATT客户端上下文 / GATT client context
 * @param ll_ctx Link Layer上下文 / Link Layer context
 * @return 操作状态 / Operation status
 */
ble_status_t ble_gatt_init(gatt_client_context_t* ctx, ble_conn_context_t* ll_ctx)
{
    if (!ctx || !ll_ctx) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 清空上下文 / Clear context */
    memset(ctx, 0, sizeof(gatt_client_context_t));
    
    /* 设置默认值 / Set default values */
    ctx->ll_ctx = ll_ctx;
    ctx->mtu = ATT_MTU_DEFAULT;         // 默认MTU为23字节 / Default MTU is 23 bytes
    ctx->bracelet_type = BRACELET_TYPE_UNKNOWN;  // 未知手环类型 / Unknown bracelet type
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发现手环服务 / Discover bracelet services
 * @param ctx GATT客户端上下文 / GATT client context
 * @param type 返回的手环类型 / Returned bracelet type
 * @return 操作状态 / Operation status
 * 
 * @details 通过读取设备名称或服务UUID来识别手环类型
 *          Identifies bracelet type by reading device name or service UUID
 */
ble_status_t ble_gatt_discover_bracelet(gatt_client_context_t* ctx, bracelet_type_t* type)
{
    att_msg_t req, rsp;
    ble_status_t status;
    
    /* 读取设备名称特征（通常在0x0003） / Read device name characteristic (usually at 0x0003) */
    gatt_build_read_request(&req, 0x0003);
    
    status = gatt_send_att_request(ctx, &req);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    status = gatt_wait_att_response(ctx, ATT_READ_RSP, 1000);
    if (status == BLE_STATUS_OK) {
        /* 解析设备名称来识别手环类型 / Parse device name to identify bracelet type */
        uint8_t* name = ctx->response_buffer + 1;  // 跳过opcode / Skip opcode
        
        if (strstr((char*)name, "Mi Band") != NULL) {
            *type = BRACELET_TYPE_XIAOMI;           // 检测到小米手环 / Xiaomi bracelet detected
        } else if (strstr((char*)name, "Nordic") != NULL) {
            *type = BRACELET_TYPE_NORDIC_UART;      // 检测到Nordic UART设备 / Nordic UART device detected
        } else {
            *type = BRACELET_TYPE_CUSTOM;           // 未知设备，使用自定义配置 / Unknown device, use custom config
        }
        
        ctx->bracelet_type = *type;
        ctx->handles = bracelet_handles[*type];     // 加载对应的句柄配置 / Load corresponding handle configuration
        
        return BLE_STATUS_OK;
    }
    
    /* 如果读取名称失败，尝试通过服务UUID识别 / If reading name fails, try to identify by service UUID */
    /* 构建Read By Type请求查找主服务 / Build Read By Type request to find primary services */
    req.opcode = ATT_READ_BY_TYPE_REQ;
    req.params.read_by_type.starting_handle = 0x0001;   // 从句柄1开始 / Start from handle 1
    req.params.read_by_type.ending_handle = 0xFFFF;     // 到最大句柄 / To maximum handle
    req.params.read_by_type.uuid16 = UUID_PRIMARY_SERVICE;  // 查找主服务UUID / Find primary service UUID
    
    status = gatt_send_att_request(ctx, &req);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    status = gatt_wait_att_response(ctx, ATT_READ_BY_TYPE_RSP, 1000);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 解析响应查找已知服务 / Parse response to find known services */
    uint8_t* p = ctx->response_buffer + 1;  // 跳过opcode / Skip opcode
    uint8_t len = *p++;  // 每个attribute数据的长度 / Length of each attribute data
    
    while (p - ctx->response_buffer < ctx->response_length) {
        uint16_t handle = p[0] | (p[1] << 8);   // 属性句柄 / Attribute handle
        uint16_t uuid = p[2] | (p[3] << 8);     // 服务UUID / Service UUID
        
        /* 检查已知服务UUID / Check known service UUIDs */
        if (uuid == 0xFEE0) {  // 小米手环服务 / Xiaomi bracelet service
            *type = BRACELET_TYPE_XIAOMI;
            ctx->bracelet_type = *type;
            ctx->handles = bracelet_handles[*type];
            return BLE_STATUS_OK;
        } else if (uuid == UUID_NORDIC_UART_SERVICE) {  // Nordic UART服务 / Nordic UART service
            *type = BRACELET_TYPE_NORDIC_UART;
            ctx->bracelet_type = *type;
            ctx->handles = bracelet_handles[*type];
            return BLE_STATUS_OK;
        }
        
        p += len;  // 移动到下一个属性 / Move to next attribute
    }
    
    /* 未找到已知服务，使用默认配置 / Known service not found, use default configuration */
    *type = BRACELET_TYPE_CUSTOM;
    ctx->bracelet_type = *type;
    ctx->handles = bracelet_handles[*type];
    
    return BLE_STATUS_OK;
}

/**
 * @brief 发送文本到手环 / Send text to bracelet
 * @param ctx GATT客户端上下文 / GATT client context
 * @param text 要发送的文本 / Text to send
 * @return 操作状态 / Operation status
 * 
 * @details 如果文本过长，会自动分片发送
 *          If text is too long, it will be automatically fragmented
 */
ble_status_t ble_gatt_write_text(gatt_client_context_t* ctx, const char* text)
{
    if (!ctx || !text) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    if (ctx->bracelet_type == BRACELET_TYPE_UNKNOWN) {
        return BLE_STATUS_ERROR;  // 手环类型未知 / Bracelet type unknown
    }
    
    uint16_t text_len = strlen(text);
    uint16_t offset = 0;
    
    /* 分片发送长文本 / Fragment and send long text */
    while (offset < text_len) {
        uint16_t chunk_len = text_len - offset;
        if (chunk_len > ctx->mtu - 3) {
            chunk_len = ctx->mtu - 3;  // 减去3字节的ATT头 / Subtract 3 bytes for ATT header
        }
        
        ble_status_t status = ble_gatt_write_data(ctx, 
                                                 ctx->handles.tx_char_handle,   // 使用手环TX句柄 / Use bracelet TX handle
                                                 (uint8_t*)&text[offset],       // 当前分片数据 / Current fragment data
                                                 chunk_len);                    // 分片长度 / Fragment length
        if (status != BLE_STATUS_OK) {
            return status;
        }
        
        offset += chunk_len;
        
        /* 短延时避免拥塞 / Short delay to avoid congestion */
        HAL_Delay(20);  // 20ms延时 / 20ms delay
    }
    
    return BLE_STATUS_OK;
}

/**
 * @brief 写入数据到指定句柄 / Write data to specified handle
 * @param ctx GATT客户端上下文 / GATT client context
 * @param handle 目标句柄 / Target handle
 * @param data 要写入的数据 / Data to write
 * @param len 数据长度 / Data length
 * @return 操作状态 / Operation status
 */
ble_status_t ble_gatt_write_data(gatt_client_context_t* ctx, 
                                 uint16_t handle,
                                 uint8_t* data,
                                 uint16_t len)
{
    att_msg_t req;
    ble_status_t status;
    
    if (!ctx || !data || len == 0 || len > ctx->mtu - 3) {
        return BLE_STATUS_INVALID_PARAMS;  // 参数错误或数据太长 / Invalid params or data too long
    }
    
    /* 构建写请求 / Build write request */
    gatt_build_write_request(&req, handle, data, len);
    
    /* 发送请求 / Send request */
    status = gatt_send_att_request(ctx, &req);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 等待响应 / Wait for response */
    status = gatt_wait_att_response(ctx, ATT_WRITE_RSP, 1000);  // 1秒超时 / 1 second timeout
    
    return status;
}

/**
 * @brief 读取数据 / Read data
 * @param ctx GATT客户端上下文 / GATT client context
 * @param handle 要读取的句柄 / Handle to read from
 * @param data 读取数据的缓冲区 / Buffer for read data
 * @param len 返回的数据长度 / Returned data length
 * @return 操作状态 / Operation status
 */
ble_status_t ble_gatt_read_data(gatt_client_context_t* ctx,
                                uint16_t handle,
                                uint8_t* data,
                                uint16_t* len)
{
    att_msg_t req;
    ble_status_t status;
    
    if (!ctx || !data || !len) {
        return BLE_STATUS_INVALID_PARAMS;
    }
    
    /* 构建读请求 / Build read request */
    gatt_build_read_request(&req, handle);
    
    /* 发送请求 / Send request */
    status = gatt_send_att_request(ctx, &req);
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 等待响应 / Wait for response */
    status = gatt_wait_att_response(ctx, ATT_READ_RSP, 1000);  // 1秒超时 / 1 second timeout
    if (status != BLE_STATUS_OK) {
        return status;
    }
    
    /* 复制数据 / Copy data */
    *len = ctx->response_length - 1;  // 减去opcode / Subtract opcode
    memcpy(data, ctx->response_buffer + 1, *len);  // 跳过opcode复制数据 / Skip opcode and copy data
    
    return BLE_STATUS_OK;
}

/**
 * @brief 启用通知 / Enable notifications
 * @param ctx GATT客户端上下文 / GATT client context
 * @param char_handle 要启用通知的特征句柄 / Characteristic handle to enable notifications for
 * @return 操作状态 / Operation status
 * 
 * @details 写入CCCD (Client Characteristic Configuration Descriptor)来启用通知
 *          Writes to CCCD (Client Characteristic Configuration Descriptor) to enable notifications
 */
ble_status_t ble_gatt_enable_notifications(gatt_client_context_t* ctx, uint16_t char_handle)
{
    uint8_t cccd_value[2] = {0x01, 0x00};  // Enable notifications / 启用通知值
    uint16_t cccd_handle = 0;
    
    /* 查找CCCD句柄（通常在特征句柄+1） / Find CCCD handle (usually at characteristic handle + 1) */
    if (char_handle == ctx->handles.rx_char_handle) {
        cccd_handle = ctx->handles.cccd_handle;    // 使用已知的CCCD句柄 / Use known CCCD handle
    } else {
        cccd_handle = char_handle + 1;  // 假设CCCD在下一个句柄 / Assume CCCD at next handle
    }
    
    /* 写入CCCD / Write to CCCD */
    return ble_gatt_write_data(ctx, cccd_handle, cccd_value, 2);
}

/**
 * @brief 处理接收的数据 / Handle received data
 * @param ctx GATT客户端上下文 / GATT client context
 * @param data 接收到的数据 / Received data
 * @param len 数据长度 / Data length
 * 
 * @details 解析并处理各种ATT PDU
 *          Parses and processes various ATT PDUs
 */
void ble_gatt_handle_rx_data(gatt_client_context_t* ctx, uint8_t* data, uint16_t len)
{
    if (!ctx || !data || len == 0) {
        return;
    }
    
    uint8_t opcode = data[0];  // ATT操作码 / ATT opcode
    
    switch (opcode) {
        case ATT_ERROR_RSP:
            /* 错误响应 / Error response */
            ctx->response_buffer[0] = opcode;
            ctx->response_length = len;
            ctx->response_received = true;
            break;
            
        case ATT_EXCHANGE_MTU_RSP:
            /* MTU响应 / MTU response */
            ctx->mtu = data[1] | (data[2] << 8);   // 获取服务器支持的MTU / Get server supported MTU
            if (ctx->mtu > ATT_MTU_MAX) {
                ctx->mtu = ATT_MTU_MAX;            // 限制在最大值 / Limit to maximum value
            }
            ctx->response_received = true;
            break;
            
        case ATT_READ_RSP:
        case ATT_READ_BLOB_RSP:
        case ATT_READ_BY_TYPE_RSP:
        case ATT_READ_BY_GROUP_RSP:
        case ATT_WRITE_RSP:
            /* 各种响应 / Various responses */
            memcpy(ctx->response_buffer, data, len);   // 保存响应数据 / Save response data
            ctx->response_length = len;                // 保存响应长度 / Save response length
            ctx->response_received = true;             // 设置接收标志 / Set received flag
            break;
            
        case ATT_HANDLE_VALUE_NTF:
            /* 通知 / Notification */
            {
                att_msg_t msg;
                msg.opcode = opcode;
                msg.params.notification.handle = data[1] | (data[2] << 8);    // 提取句柄 / Extract handle
                memcpy(msg.params.notification.value, &data[3], len - 3);     // 复制通知数据 / Copy notification data
                gatt_process_notification(ctx, &msg);                          // 处理通知 / Process notification
            }
            break;
            
        case ATT_HANDLE_VALUE_IND:
            /* 指示（需要确认） / Indication (requires confirmation) */
            {
                /* 发送确认 / Send confirmation */
                uint8_t confirm = ATT_HANDLE_VALUE_CFM;
                ble_ll_send_data(ctx->ll_ctx, &confirm, 1);  // 发送确认PDU / Send confirmation PDU
                
                /* 处理指示 / Process indication */
                att_msg_t msg;
                msg.opcode = opcode;
                msg.params.notification.handle = data[1] | (data[2] << 8);    // 提取句柄 / Extract handle
                memcpy(msg.params.notification.value, &data[3], len - 3);     // 复制指示数据 / Copy indication data
                gatt_process_notification(ctx, &msg);                          // 处理指示 / Process indication
            }
            break;
            
        default:
            /* 未知PDU / Unknown PDU */
            break;
    }
}

/**
 * @brief 发送ATT请求 / Send ATT request
 * @param ctx GATT客户端上下文 / GATT client context
 * @param req 要发送的ATT请求 / ATT request to send
 * @return 操作状态 / Operation status
 * 
 * @details 构建并发送ATT协议数据单元
 *          Builds and sends ATT protocol data unit
 */
ble_status_t gatt_send_att_request(gatt_client_context_t* ctx, att_msg_t* req)
{
    uint8_t pdu[ATT_MTU_MAX];
    uint16_t pdu_len = 0;
    
    /* 清除之前的响应 / Clear previous response */
    ctx->response_received = false;
    ctx->response_length = 0;
    
    /* 构建PDU / Build PDU */
    pdu[pdu_len++] = req->opcode;  // 添加操作码 / Add opcode
    
    switch (req->opcode) {
        case ATT_READ_BY_TYPE_REQ:
            /* 按类型读取请求 / Read by type request */
            pdu[pdu_len++] = req->params.read_by_type.starting_handle & 0xFF;           // 起始句柄低字节 / Starting handle low byte
            pdu[pdu_len++] = (req->params.read_by_type.starting_handle >> 8) & 0xFF;    // 起始句柄高字节 / Starting handle high byte
            pdu[pdu_len++] = req->params.read_by_type.ending_handle & 0xFF;             // 结束句柄低字节 / Ending handle low byte
            pdu[pdu_len++] = (req->params.read_by_type.ending_handle >> 8) & 0xFF;      // 结束句柄高字节 / Ending handle high byte
            pdu[pdu_len++] = req->params.read_by_type.uuid16 & 0xFF;                    // UUID低字节 / UUID low byte
            pdu[pdu_len++] = (req->params.read_by_type.uuid16 >> 8) & 0xFF;             // UUID高字节 / UUID high byte
            break;
            
        case ATT_READ_REQ:
            /* 读取请求 / Read request */
            pdu[pdu_len++] = req->params.read.handle & 0xFF;            // 句柄低字节 / Handle low byte
            pdu[pdu_len++] = (req->params.read.handle >> 8) & 0xFF;     // 句柄高字节 / Handle high byte
            break;
            
        case ATT_WRITE_REQ:
            /* 写入请求 / Write request */
            pdu[pdu_len++] = req->params.write.handle & 0xFF;           // 句柄低字节 / Handle low byte
            pdu[pdu_len++] = (req->params.write.handle >> 8) & 0xFF;    // 句柄高字节 / Handle high byte
            memcpy(&pdu[pdu_len], req->params.write.value, ctx->mtu - 3);  // 复制写入数据 / Copy write data
            pdu_len += ctx->mtu - 3;
            break;
            
        default:
            return BLE_STATUS_INVALID_PARAMS;  // 不支持的操作码 / Unsupported opcode
    }
    
    /* 保存待确认信息 / Save pending confirmation info */
    ctx->pending_op = req->opcode;
    ctx->pending_handle = (req->opcode == ATT_WRITE_REQ) ? 
                         req->params.write.handle :      // 写请求使用写句柄 / Write request uses write handle
                         req->params.read.handle;        // 读请求使用读句柄 / Read request uses read handle
    
    /* 发送PDU / Send PDU */
    return ble_ll_send_data(ctx->ll_ctx, pdu, pdu_len);
}

/**
 * @brief 等待ATT响应 / Wait for ATT response
 * @param ctx GATT客户端上下文 / GATT client context
 * @param expected_opcode 期望的响应操作码 / Expected response opcode
 * @param timeout_ms 超时时间(毫秒) / Timeout in milliseconds
 * @return 操作状态 / Operation status
 */
ble_status_t gatt_wait_att_response(gatt_client_context_t* ctx,
                                    uint8_t expected_opcode,
                                    uint32_t timeout_ms)
{
    uint32_t start_time = HAL_GetTick();
    
    while (!ctx->response_received) {
        if (HAL_GetTick() - start_time > timeout_ms) {
            return BLE_STATUS_TIMEOUT;  // 超时 / Timeout
        }
        
        /* 让LL层处理事件 / Let LL layer process events */
        ble_ll_process_events(ctx->ll_ctx);
        
        /* 短延时避免忙等待 / Short delay to avoid busy waiting */
        HAL_Delay(1);  // 1ms延时 / 1ms delay
    }
    
    /* 检查是否收到错误响应 / Check if error response received */
    if (ctx->response_buffer[0] == ATT_ERROR_RSP) {
        uint8_t error_code = ctx->response_buffer[4];  // 错误码在第5个字节 / Error code at 5th byte
        return BLE_STATUS_PROTOCOL_ERROR;
    }
    
    /* 检查是否收到期望的响应 / Check if expected response received */
    if (ctx->response_buffer[0] != expected_opcode) {
        return BLE_STATUS_PROTOCOL_ERROR;  // 意外的响应 / Unexpected response
    }
    
    return BLE_STATUS_OK;
}

/**
 * @brief 构建写请求 / Build write request
 * @param msg ATT消息结构 / ATT message structure
 * @param handle 目标句柄 / Target handle
 * @param value 要写入的数据 / Data to write
 * @param len 数据长度 / Data length
 */
void gatt_build_write_request(att_msg_t* msg, uint16_t handle, 
                             uint8_t* value, uint16_t len)
{
    msg->opcode = ATT_WRITE_REQ;           // 设置写请求操作码 / Set write request opcode
    msg->params.write.handle = handle;     // 设置句柄 / Set handle
    memcpy(msg->params.write.value, value, len);  // 复制数据 / Copy data
}

/**
 * @brief 构建读请求 / Build read request
 * @param msg ATT消息结构 / ATT message structure
 * @param handle 要读取的句柄 / Handle to read from
 */
void gatt_build_read_request(att_msg_t* msg, uint16_t handle)
{
    msg->opcode = ATT_READ_REQ;        // 设置读请求操作码 / Set read request opcode
    msg->params.read.handle = handle;  // 设置句柄 / Set handle
}

/**
 * @brief 处理通知 / Process notification
 * @param ctx GATT客户端上下文 / GATT client context
 * @param msg 通知消息 / Notification message
 * 
 * @details 处理从手环接收到的通知或指示
 *          Processes notifications or indications received from bracelet
 */
void gatt_process_notification(gatt_client_context_t* ctx, att_msg_t* msg)
{
    /* 检查是否是来自手环的文本通知 / Check if text notification from bracelet */
    if (msg->params.notification.handle == ctx->handles.rx_char_handle) {
        /* 通知应用层 / Notify application layer */
        if (ctx->ll_ctx->on_data_received) {
            ctx->ll_ctx->on_data_received(ctx->ll_ctx,          // 传递LL上下文 / Pass LL context
                                        msg->params.notification.value,  // 通知数据 / Notification data
                                        ctx->response_length - 3);       // 数据长度(减去头部) / Data length (minus header)
        }
    }
}

/**
 * @brief 获取手环句柄配置 / Get bracelet handle configuration
 * @param type 手环类型 / Bracelet type
 * @return 句柄配置指针，如果类型无效则返回NULL / Handle configuration pointer, NULL if type invalid
 */
const gatt_bracelet_handles_t* gatt_get_bracelet_handles(bracelet_type_t type)
{
    if (type >= sizeof(bracelet_handles) / sizeof(bracelet_handles[0])) {
        return NULL;  // 类型超出范围 / Type out of range
    }
    
    return &bracelet_handles[type];  // 返回对应的句柄配置 / Return corresponding handle configuration
}

/**
 * @brief 小米手环认证（如果需要） / Xiaomi bracelet authentication (if needed)
 * @param ctx GATT客户端上下文 / GATT client context
 * @return 操作状态 / Operation status
 * 
 * @note 小米手环可能需要特殊的认证流程
 *       Xiaomi bracelets may require special authentication process
 */
ble_status_t gatt_authenticate_xiaomi(gatt_client_context_t* ctx)
{
    /* 小米手环可能需要特殊的认证流程 / Xiaomi bracelet may need special authentication process */
    /* 这里只是占位，实际认证协议需要逆向工程 / This is just placeholder, actual auth protocol needs reverse engineering */
    
    // TODO: 实现小米手环认证 / Implement Xiaomi bracelet authentication
    
    return BLE_STATUS_OK;
}