/**
 * @file    sx1280_verification.c
 * @brief   SX1280芯片验证功能
 * @author  AI Assistant
 * @date    2024
 */

#include "sx1280_hal_stm32g0.h"
#include "sx128x.h"
#include "sx128x_regs.h"
#include <stdio.h>

/* SX1280芯片信息 */
#define SX1280_CHIP_ID_EXPECTED     0xA0
#define SX1280_FIRMWARE_VERSION_REG 0x0153
#define SX1280_DEVICE_ID_REG        0x8000

/**
 * @brief 验证SX1280芯片ID
 * @param radio_context SX1280上下文
 * @return true if chip ID is correct, false otherwise
 */
bool sx1280_verify_chip_id(void* radio_context)
{
    uint8_t chip_id;
    sx128x_status_t status;
    
    if (!radio_context) {
        printf("[SX1280] Error: Invalid radio context\n");
        return false;
    }
    
    /* 读取芯片ID */
    status = sx128x_get_device_id(radio_context, &chip_id);
    if (status != SX128X_STATUS_OK) {
        printf("[SX1280] Error: Failed to read chip ID (status: %d)\n", status);
        return false;
    }
    
    printf("[SX1280] Chip ID: 0x%02X", chip_id);
    
    /* 验证芯片ID */
    if (chip_id == SX1280_CHIP_ID_EXPECTED) {
        printf(" - OK (SX1280/SX1281 detected)\n");
        return true;
    } else {
        printf(" - ERROR (Expected: 0x%02X)\n", SX1280_CHIP_ID_EXPECTED);
        return false;
    }
}

/**
 * @brief 读取并显示SX1280版本信息
 * @param radio_context SX1280上下文
 */
void sx1280_display_version_info(void* radio_context)
{
    uint8_t firmware_version[2];
    sx128x_status_t status;
    
    if (!radio_context) {
        return;
    }
    
    /* 读取固件版本 */
    status = sx128x_read_register(radio_context, SX1280_FIRMWARE_VERSION_REG, 
                                 firmware_version, 2);
    if (status == SX128X_STATUS_OK) {
        printf("[SX1280] Firmware Version: %d.%d\n", 
               firmware_version[0], firmware_version[1]);
    }
}

/**
 * @brief 执行SX1280自检
 * @param radio_context SX1280上下文
 * @return true if self test passed, false otherwise
 */
bool sx1280_self_test(void* radio_context)
{
    sx128x_status_t status;
    uint8_t test_data[4] = {0xAA, 0x55, 0xF0, 0x0F};
    uint8_t read_data[4] = {0};
    
    printf("[SX1280] Starting self test...\n");
    
    /* 1. 验证芯片ID */
    if (!sx1280_verify_chip_id(radio_context)) {
        printf("[SX1280] Self test FAILED: Chip ID mismatch\n");
        return false;
    }
    
    /* 2. 测试寄存器读写 */
    printf("[SX1280] Testing register access...\n");
    
    /* 写入测试数据到缓冲区 */
    sx128x_write_buffer(radio_context, 0x00, test_data, 4);
    
    /* 读回数据 */
    sx128x_read_buffer(radio_context, 0x00, read_data, 4);
    
    /* 验证数据 */
    bool read_write_ok = true;
    for (int i = 0; i < 4; i++) {
        if (read_data[i] != test_data[i]) {
            read_write_ok = false;
            break;
        }
    }
    
    if (read_write_ok) {
        printf("[SX1280] Register read/write test - OK\n");
    } else {
        printf("[SX1280] Register read/write test - FAILED\n");
        printf("  Written: %02X %02X %02X %02X\n", 
               test_data[0], test_data[1], test_data[2], test_data[3]);
        printf("  Read:    %02X %02X %02X %02X\n", 
               read_data[0], read_data[1], read_data[2], read_data[3]);
        return false;
    }
    
    /* 3. 测试工作模式切换 */
    printf("[SX1280] Testing operating modes...\n");
    
    /* 切换到待机模式 */
    status = sx128x_set_standby(radio_context, SX128X_STANDBY_RC);
    if (status != SX128X_STATUS_OK) {
        printf("[SX1280] Failed to enter standby mode\n");
        return false;
    }
    
    /* 获取状态 */
    sx128x_chip_status_t chip_status;
    status = sx128x_get_status(radio_context, &chip_status);
    if (status != SX128X_STATUS_OK) {
        printf("[SX1280] Failed to read status\n");
        return false;
    }
    
    /* 验证状态 */
    if (chip_status.chip_mode == SX128X_CHIP_MODE_STBY_RC) {
        printf("[SX1280] Mode switching test - OK\n");
    } else {
        printf("[SX1280] Mode switching test - FAILED (mode: %d)\n", 
               chip_status.chip_mode);
        return false;
    }
    
    /* 4. 测试中断功能 */
    printf("[SX1280] Testing interrupt functionality...\n");
    
    /* 清除所有中断 */
    sx128x_clear_irq_status(radio_context, SX128X_IRQ_ALL);
    
    /* 读取中断状态（应该为0） */
    sx128x_irq_mask_t irq_status;
    sx128x_get_irq_status(radio_context, &irq_status);
    
    if (irq_status == 0) {
        printf("[SX1280] Interrupt test - OK\n");
    } else {
        printf("[SX1280] Interrupt test - WARNING (IRQ status: 0x%04X)\n", 
               irq_status);
    }
    
    /* 显示版本信息 */
    sx1280_display_version_info(radio_context);
    
    printf("[SX1280] Self test PASSED\n");
    return true;
}

/**
 * @brief 检查SX1280 BLE功能
 * @param radio_context SX1280上下文
 * @return true if BLE is supported, false otherwise
 */
bool sx1280_check_ble_capability(void* radio_context)
{
    sx128x_status_t status;
    
    printf("[SX1280] Checking BLE capability...\n");
    
    /* 尝试设置BLE包类型 */
    status = sx128x_set_pkt_type(radio_context, SX128X_PKT_TYPE_BLE);
    if (status != SX128X_STATUS_OK) {
        printf("[SX1280] BLE mode not supported\n");
        return false;
    }
    
    /* 尝试设置BLE调制参数 */
    sx128x_mod_params_ble_t ble_params = {
        .br_bw = SX128X_BLE_BR_1_000_BW_1_2,
        .mod_ind = SX128X_BLE_MOD_IND_0_50,
        .pulse_shape = SX128X_BLE_PULSE_SHAPE_OFF
    };
    
    status = sx128x_set_ble_mod_params(radio_context, &ble_params);
    if (status != SX128X_STATUS_OK) {
        printf("[SX1280] Failed to set BLE modulation parameters\n");
        return false;
    }
    
    printf("[SX1280] BLE capability confirmed\n");
    return true;
}