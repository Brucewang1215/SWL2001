# SX1280 BLE 4.2 驱动实现方案

## 一、扩展RAL层BLE支持

### 1. 修改 ral_defs.h - 添加BLE包类型和相关定义

```c
// 在 ral_pkt_type_e 枚举中添加
typedef enum ral_pkt_type_e
{
    RAL_PKT_TYPE_GFSK,
    RAL_PKT_TYPE_LORA,
    RAL_PKT_TYPE_FLRC,
    RAL_PKT_TYPE_BLE,    // 新增BLE包类型
} ral_pkt_type_t;

// 添加BLE调制参数结构
typedef struct ral_ble_mod_params_s
{
    uint32_t br_bw;           // BLE比特率和带宽
    uint8_t  mod_ind;         // BLE调制指数
    uint8_t  pulse_shape;     // BLE脉冲整形
} ral_ble_mod_params_t;

// 添加BLE包参数结构  
typedef struct ral_ble_pkt_params_s
{
    uint8_t con_state;        // BLE连接状态
    uint8_t crc_type;         // BLE CRC配置
    uint8_t pkt_type;         // BLE测试包类型
    uint8_t whitening;        // BLE白化配置
} ral_ble_pkt_params_t;

// 添加BLE包状态结构
typedef struct ral_ble_pkt_status_s
{
    int8_t rssi_sync;         // 同步字RSSI
    int8_t rssi_avg;          // 平均RSSI
    uint8_t status;           // 包状态
} ral_ble_pkt_status_t;
```

### 2. 修改 ral.h - 添加BLE相关API

```c
// 在 ral.h 中添加BLE专用API声明
ral_status_t ral_set_ble_mod_params(const void* context, const ral_ble_mod_params_t* params);
ral_status_t ral_set_ble_pkt_params(const void* context, const ral_ble_pkt_params_t* params);
ral_status_t ral_get_ble_pkt_status(const void* context, ral_ble_pkt_status_t* pkt_status);
ral_status_t ral_set_ble_sync_word(const void* context, const uint8_t* sync_word);
ral_status_t ral_set_ble_crc_seed(const void* context, uint32_t seed);
ral_status_t ral_set_ble_whitening_seed(const void* context, uint16_t seed);
```

### 3. 修改 ral_sx128x.c - 实现BLE支持

```c
// 在 ral_sx128x_set_pkt_type() 中添加BLE处理
ral_status_t ral_sx128x_set_pkt_type(const void* context, const ral_pkt_type_t pkt_type)
{
    switch (pkt_type)
    {
    case RAL_PKT_TYPE_GFSK:
        return sx128x_set_pkt_type(context, SX128X_PKT_TYPE_GFSK);
    case RAL_PKT_TYPE_LORA:
        return sx128x_set_pkt_type(context, SX128X_PKT_TYPE_LORA);
    case RAL_PKT_TYPE_FLRC:
        return sx128x_set_pkt_type(context, SX128X_PKT_TYPE_FLRC);
    case RAL_PKT_TYPE_BLE:  // 新增BLE处理
        return sx128x_set_pkt_type(context, SX128X_PKT_TYPE_BLE);
    default:
        return RAL_STATUS_UNKNOWN_VALUE;
    }
}

// 实现BLE调制参数设置
ral_status_t ral_sx128x_set_ble_mod_params(const void* context, const ral_ble_mod_params_t* params)
{
    sx128x_mod_params_ble_t sx128x_params;
    
    // 转换RAL参数到SX128x参数
    sx128x_params.br_bw = (sx128x_ble_br_bw_t)params->br_bw;
    sx128x_params.mod_ind = (sx128x_ble_mod_ind_t)params->mod_ind;
    sx128x_params.pulse_shape = (sx128x_ble_pulse_shape_t)params->pulse_shape;
    
    return sx128x_set_ble_mod_params(context, &sx128x_params);
}

// 实现BLE包参数设置
ral_status_t ral_sx128x_set_ble_pkt_params(const void* context, const ral_ble_pkt_params_t* params)
{
    sx128x_pkt_params_ble_t sx128x_params;
    
    // 转换RAL参数到SX128x参数
    sx128x_params.con_state = (sx128x_ble_con_states_t)params->con_state;
    sx128x_params.crc_type = (sx128x_ble_crc_type_t)params->crc_type;
    sx128x_params.pkt_type = (sx128x_ble_pkt_type_t)params->pkt_type;
    sx128x_params.dc_free = (sx128x_ble_dc_free_t)params->whitening;
    
    return sx128x_set_ble_pkt_params(context, &sx128x_params);
}
```

## 二、BLE 4.2 应用层实现

### 1. BLE初始化模块

```c
// ble_sx1280.h
#ifndef BLE_SX1280_H
#define BLE_SX1280_H

#include "ral.h"

// BLE频道定义 (2402 + n * 2) MHz, n = 0..39
#define BLE_CHANNEL_0   2402000000
#define BLE_CHANNEL_37  2402000000  // 广播频道
#define BLE_CHANNEL_38  2426000000  // 广播频道
#define BLE_CHANNEL_39  2480000000  // 广播频道

// BLE参数
typedef struct ble_config_s
{
    uint32_t access_address;    // 接入地址
    uint32_t crc_init;         // CRC初始值
    uint8_t  channel;          // 频道号(0-39)
    uint8_t  data_rate;        // 数据率: 0=125kbps, 1=500kbps, 2=1Mbps, 3=2Mbps
} ble_config_t;

// BLE广播包结构
typedef struct ble_adv_pdu_s
{
    uint8_t header;            // PDU类型和长度
    uint8_t payload[37];       // 最大37字节负载
    uint8_t length;            // 实际负载长度
} ble_adv_pdu_t;

// API函数
ral_status_t ble_sx1280_init(const void* context, const ble_config_t* config);
ral_status_t ble_sx1280_start_advertising(const void* context, const ble_adv_pdu_t* adv_pdu);
ral_status_t ble_sx1280_start_scanning(const void* context, uint32_t scan_window_ms);
ral_status_t ble_sx1280_send_data(const void* context, const uint8_t* data, uint8_t length);
ral_status_t ble_sx1280_receive_data(const void* context, uint8_t* data, uint8_t* length);

#endif // BLE_SX1280_H
```

### 2. BLE实现文件

```c
// ble_sx1280.c
#include "ble_sx1280.h"
#include "ral_sx128x.h"

// BLE数据率映射
static const uint32_t ble_data_rate_map[] = {
    125000,   // 125 kbps
    500000,   // 500 kbps
    1000000,  // 1 Mbps
    2000000   // 2 Mbps
};

ral_status_t ble_sx1280_init(const void* context, const ble_config_t* config)
{
    ral_status_t status;
    
    // 1. 设置包类型为BLE
    status = ral_set_pkt_type(context, RAL_PKT_TYPE_BLE);
    if (status != RAL_STATUS_OK) return status;
    
    // 2. 设置BLE调制参数
    ral_ble_mod_params_t mod_params = {
        .br_bw = config->data_rate,
        .mod_ind = 1,  // BT=0.5
        .pulse_shape = 0  // No pulse shaping
    };
    status = ral_set_ble_mod_params(context, &mod_params);
    if (status != RAL_STATUS_OK) return status;
    
    // 3. 设置BLE包参数
    ral_ble_pkt_params_t pkt_params = {
        .con_state = 0,     // Advertiser
        .crc_type = 1,      // 3 bytes CRC
        .pkt_type = 0,      // PRBS9
        .whitening = 1      // Whitening enabled
    };
    status = ral_set_ble_pkt_params(context, &pkt_params);
    if (status != RAL_STATUS_OK) return status;
    
    // 4. 设置接入地址（同步字）
    uint8_t sync_word[4];
    sync_word[0] = (config->access_address >> 0) & 0xFF;
    sync_word[1] = (config->access_address >> 8) & 0xFF;
    sync_word[2] = (config->access_address >> 16) & 0xFF;
    sync_word[3] = (config->access_address >> 24) & 0xFF;
    status = ral_set_ble_sync_word(context, sync_word);
    if (status != RAL_STATUS_OK) return status;
    
    // 5. 设置CRC初始值
    status = ral_set_ble_crc_seed(context, config->crc_init);
    if (status != RAL_STATUS_OK) return status;
    
    // 6. 设置白化种子（基于频道）
    uint16_t whitening_seed = 0x40 | (config->channel & 0x3F);
    status = ral_set_ble_whitening_seed(context, whitening_seed);
    
    return status;
}

ral_status_t ble_sx1280_start_advertising(const void* context, const ble_adv_pdu_t* adv_pdu)
{
    ral_status_t status;
    uint32_t channels[] = {BLE_CHANNEL_37, BLE_CHANNEL_38, BLE_CHANNEL_39};
    
    // 在三个广播频道上轮流发送
    for (int i = 0; i < 3; i++)
    {
        // 设置频率
        status = ral_set_rf_freq(context, channels[i]);
        if (status != RAL_STATUS_OK) return status;
        
        // 发送广播包
        status = ral_set_pkt_payload(context, adv_pdu->payload, adv_pdu->length);
        if (status != RAL_STATUS_OK) return status;
        
        // 设置TX模式并发送
        ral_params_tx_t tx_params = {
            .output_pwr_in_dbm = 0,  // 0 dBm
            .rf_freq_in_hz = channels[i],
            .pkt_type = RAL_PKT_TYPE_BLE
        };
        status = ral_set_tx(context, &tx_params);
        if (status != RAL_STATUS_OK) return status;
        
        // 等待发送完成
        // 实际应用中应该使用中断或DIO检测
        hal_delay_ms(10);
    }
    
    return RAL_STATUS_OK;
}

ral_status_t ble_sx1280_start_scanning(const void* context, uint32_t scan_window_ms)
{
    ral_status_t status;
    
    // 设置接收参数
    ral_params_rx_t rx_params = {
        .rf_freq_in_hz = BLE_CHANNEL_37,  // 从37频道开始
        .timeout_in_ms = scan_window_ms,
        .pkt_type = RAL_PKT_TYPE_BLE,
        .continuous = false
    };
    
    // 开始接收
    status = ral_set_rx(context, &rx_params);
    
    return status;
}
```

### 3. 使用示例

```c
// main_ble_example.c
#include "ble_sx1280.h"
#include "smtc_modem_hal.h"

// BLE广播数据示例
static void create_ble_beacon(ble_adv_pdu_t* pdu)
{
    // PDU Header: ADV_NONCONN_IND
    pdu->header = 0x02;
    
    // 广播地址（6字节）
    uint8_t adv_addr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    memcpy(&pdu->payload[0], adv_addr, 6);
    
    // 广播数据
    uint8_t idx = 6;
    
    // Flags
    pdu->payload[idx++] = 0x02;  // 长度
    pdu->payload[idx++] = 0x01;  // 类型: Flags
    pdu->payload[idx++] = 0x06;  // LE General Discoverable + BR/EDR Not Supported
    
    // 设备名称
    const char* device_name = "SX1280-BLE";
    pdu->payload[idx++] = strlen(device_name) + 1;  // 长度
    pdu->payload[idx++] = 0x09;  // 类型: Complete Local Name
    memcpy(&pdu->payload[idx], device_name, strlen(device_name));
    idx += strlen(device_name);
    
    pdu->length = idx;
}

int main(void)
{
    // 初始化HAL
    smtc_modem_hal_init();
    
    // 初始化SX1280
    void* sx1280_context = sx128x_init();  // 假设已实现
    
    // BLE配置
    ble_config_t ble_config = {
        .access_address = 0x8E89BED6,  // BLE广播接入地址
        .crc_init = 0x555555,          // BLE CRC初始值
        .channel = 37,                 // 广播频道37
        .data_rate = 2                 // 1 Mbps
    };
    
    // 初始化BLE
    if (ble_sx1280_init(sx1280_context, &ble_config) != RAL_STATUS_OK)
    {
        printf("BLE initialization failed\n");
        return -1;
    }
    
    // 创建广播包
    ble_adv_pdu_t adv_pdu;
    create_ble_beacon(&adv_pdu);
    
    // 开始广播
    while (1)
    {
        ble_sx1280_start_advertising(sx1280_context, &adv_pdu);
        hal_delay_ms(100);  // 100ms广播间隔
    }
    
    return 0;
}
```

## 三、编译配置

### 1. Makefile修改

```makefile
# 在应用的Makefile中添加
RADIO = sx128x
USE_BLE = yes

# BLE源文件
ifeq ($(USE_BLE),yes)
    C_SOURCES += ble_sx1280.c
    C_INCLUDES += -I$(BLE_INC_DIR)
    CFLAGS += -DUSE_BLE_SUPPORT
endif
```

### 2. 配置文件

```c
// modem_config.h
#define USE_BLE_SUPPORT
#define BLE_MAX_PAYLOAD_SIZE 37
#define BLE_NUM_CHANNELS 40
```

## 四、测试和验证

### 1. 基本功能测试
- 广播包发送测试
- 扫描接收测试
- 不同数据率测试
- 频道切换测试

### 2. 兼容性测试
- 使用标准BLE设备（手机、BLE模块）验证
- 使用BLE协议分析仪验证包格式
- 测试与其他BLE 4.2设备的互操作性

### 3. 性能测试
- 发送功率和接收灵敏度测试
- 数据吞吐量测试
- 功耗测试

## 五、注意事项

1. **物理层限制**: SX1280只实现BLE物理层，不包含完整的BLE协议栈
2. **时序要求**: BLE有严格的时序要求，需要精确的定时器支持
3. **频率精度**: 需要高精度的频率源（TCXO）
4. **认证**: 商用产品需要通过BLE认证
5. **功耗优化**: 实现低功耗扫描和广播模式

## 六、扩展功能

未来可以添加的功能：
1. 连接模式支持
2. 数据信道跳频
3. 加密支持
4. 更高层的BLE协议栈集成
5. Mesh网络支持