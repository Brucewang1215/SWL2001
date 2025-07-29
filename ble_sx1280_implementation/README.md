# BLE SX1280 Implementation for STM32G0

基于STM32G0和SX1280的BLE手环通信实现，支持向BLE手环发送文本消息。

## 项目概述

本项目实现了一个精简的BLE协议栈，专门用于STM32G0微控制器通过SX1280射频芯片与BLE手环进行通信。主要功能包括：

- BLE扫描和连接
- GATT客户端功能
- 向手环发送文本消息
- 支持多种手环类型

## 硬件要求

- **MCU**: STM32G071KB (或兼容型号)
  - 128KB Flash
  - 36KB RAM
  - 64MHz主频
  
- **射频芯片**: SX1280
  - 2.4GHz ISM频段
  - 支持BLE物理层
  
- **外设连接**:
  ```
  SX1280连接:
  - SPI1: SCK(PA5), MISO(PA6), MOSI(PA7), NSS(PA4)
  - RESET: PB0
  - BUSY: PB1
  - DIO1: PA1 (中断)
  
  调试接口:
  - UART2: TX(PA2), RX(PA3)
  
  用户接口:
  - LED: PA5
  - 按键: PC13
  ```

## 软件架构

```
应用层 (ble_app)
    ↓
GATT层 (ble_gatt) 
    ↓
Link Layer (ble_ll)
    ↓
SX1280驱动 (sx128x)
    ↓
HAL层 (stm32g0xx_hal)
```

### 主要模块

1. **应用层** (`Core/Src/ble_app.c`)
   - 状态机管理
   - 用户接口
   - 连接管理

2. **GATT层** (`Middleware/BLE_Stack/Src/ble_gatt.c`)
   - ATT协议实现
   - 手环服务发现
   - 数据读写

3. **Link Layer** (`Middleware/BLE_Stack/Src/ble_ll.c`)
   - 连接管理
   - 信道跳频
   - 数据包收发

4. **HAL适配** (`Drivers/BSP/Src/sx1280_hal_stm32g0.c`)
   - SPI通信
   - GPIO控制
   - 定时器管理

## 编译和烧录

### 环境准备

1. 安装ARM GCC工具链
2. 安装ST-Link工具
3. 克隆LoRa Basics Modem库到上级目录

### 编译

```bash
# 设置LBM路径
export LBM_PATH=../path/to/lora_basics_modem

# 编译
make

# 清理
make clean
```

### 烧录

```bash
make flash
```

## 使用说明

### 串口命令

通过UART2 (115200 8N1)发送命令：

- `scan` - 扫描BLE设备
- `connect` - 连接到预设手环
- `send <text>` - 发送文本消息
- `disconnect` - 断开连接
- `status` - 显示当前状态

### 按键操作

- 空闲状态：按键开始扫描并连接
- 已连接状态：按键发送预设消息

### LED指示

- 慢闪：空闲
- 快闪：扫描/连接中
- 常亮：已连接
- 双闪：错误

## 配置选项

### 修改目标手环地址

在 `main.c` 中修改：
```c
static const uint8_t TARGET_BRACELET_ADDR[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
```

### 支持的手环类型

在 `ble_gatt.c` 中定义：
- 小米手环 (`BRACELET_TYPE_XIAOMI`)
- Nordic UART服务 (`BRACELET_TYPE_NORDIC_UART`)
- 自定义手环 (`BRACELET_TYPE_CUSTOM`)

### 连接参数

在 `main.c` 中配置：
```c
ble_config.conn_interval_ms = 50;      // 连接间隔
ble_config.slave_latency = 0;          // 从设备延迟
ble_config.supervision_timeout_ms = 5000; // 超时时间
```

## 已知限制

1. **仅支持BLE物理层**，不包含完整BLE协议栈
2. **不支持加密**，手环必须允许未加密连接
3. **单连接**，同时只能连接一个设备
4. **固定参数**，不支持连接参数协商
5. **无BLE认证**，不能用于商业产品

## 故障排除

### 问题：SX1280初始化失败
- 检查SPI连接
- 验证RESET和BUSY引脚
- 确认电源供电正常

### 问题：无法连接手环
- 确认手环地址正确
- 手环需要处于广播状态
- 检查手环是否需要配对

### 问题：发送文本失败
- 确认GATT句柄配置正确
- 手环可能需要特殊认证
- 检查文本长度限制

## 扩展开发

### 添加新手环支持

1. 在 `ble_gatt.c` 中添加句柄配置：
```c
[BRACELET_TYPE_NEW] = {
    .service_handle = 0x0030,
    .tx_char_handle = 0x0032,
    .rx_char_handle = 0x0034,
    .cccd_handle    = 0x0035
}
```

2. 实现特定认证（如需要）

### 添加新功能

1. 接收通知：启用 `enable_notifications`
2. 读取数据：使用 `ble_gatt_read_data()`
3. 多连接：修改Link Layer支持多实例

## 性能指标

- 连接建立时间：< 1秒
- 数据发送延迟：< 100ms
- 功耗（连接状态）：~15mA
- 功耗（空闲状态）：~5mA

## 许可证

本项目基于LoRa Basics Modem的许可证。

## 联系方式

如有问题或建议，请提交Issue。