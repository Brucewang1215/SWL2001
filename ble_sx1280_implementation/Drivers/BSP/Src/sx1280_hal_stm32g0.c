/**
 * @file    sx1280_hal_stm32g0.c
 * @brief   SX1280 HAL接口实现（STM32G0平台）
 * @author  AI Assistant
 * @date    2024
 */

#include "sx1280_hal_stm32g0.h"
#include <string.h>

/* 私有变量 */
static volatile bool sx1280_busy_flag = false;
static volatile bool sx1280_dio1_flag = false;

/**
 * @brief 初始化SX1280 HAL
 */
void sx1280_hal_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* GPIO时钟使能 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* 配置NSS引脚（输出） */
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = SX1280_NSS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SX1280_NSS_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置RESET引脚（输出） */
    HAL_GPIO_WritePin(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = SX1280_RESET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SX1280_RESET_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置BUSY引脚（输入） */
    GPIO_InitStruct.Pin = SX1280_BUSY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SX1280_BUSY_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置DIO1引脚（中断输入） */
    GPIO_InitStruct.Pin = SX1280_DIO1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(SX1280_DIO1_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置DIO2和DIO3引脚（输入，可选） */
    GPIO_InitStruct.Pin = SX1280_DIO2_Pin | SX1280_DIO3_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SX1280_DIO2_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置EXTI中断优先级 */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
}

/**
 * @brief 反初始化SX1280 HAL
 */
void sx1280_hal_deinit(void)
{
    /* 禁用中断 */
    HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
    
    /* 复位GPIO */
    HAL_GPIO_DeInit(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin);
    HAL_GPIO_DeInit(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin);
    HAL_GPIO_DeInit(SX1280_BUSY_GPIO_Port, SX1280_BUSY_Pin);
    HAL_GPIO_DeInit(SX1280_DIO1_GPIO_Port, SX1280_DIO1_Pin);
    HAL_GPIO_DeInit(SX1280_DIO2_GPIO_Port, SX1280_DIO2_Pin | SX1280_DIO3_Pin);
}

/**
 * @brief 复位SX1280
 */
sx128x_hal_status_t sx1280_hal_reset(const void* context)
{
    /* 拉低RESET引脚 */
    HAL_GPIO_WritePin(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin, GPIO_PIN_RESET);
    sx1280_hal_delay_ms(20);
    
    /* 拉高RESET引脚 */
    HAL_GPIO_WritePin(SX1280_RESET_GPIO_Port, SX1280_RESET_Pin, GPIO_PIN_SET);
    sx1280_hal_delay_ms(10);
    
    /* 等待芯片就绪 */
    sx1280_hal_wait_on_busy(context);
    
    return SX128X_HAL_STATUS_OK;
}

/**
 * @brief 唤醒SX1280
 */
sx128x_hal_status_t sx1280_hal_wakeup(const void* context)
{
    /* 发送任意SPI数据唤醒芯片 */
    uint8_t dummy = 0x00;
    
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &dummy, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_SET);
    
    /* 等待唤醒 */
    sx1280_hal_delay_ms(10);
    sx1280_hal_wait_on_busy(context);
    
    return SX128X_HAL_STATUS_OK;
}

/**
 * @brief SPI写操作
 */
sx128x_hal_status_t sx1280_hal_write(const void* context, 
                                     const uint8_t* data, 
                                     uint16_t data_length)
{
    HAL_StatusTypeDef status;
    
    /* 等待BUSY信号 */
    sx1280_hal_wait_on_busy(context);
    
    /* 拉低NSS */
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_RESET);
    
    /* 发送数据 */
    status = HAL_SPI_Transmit(&hspi1, (uint8_t*)data, data_length, HAL_MAX_DELAY);
    
    /* 拉高NSS */
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_SET);
    
    if (status != HAL_OK) {
        return SX128X_HAL_STATUS_ERROR;
    }
    
    return SX128X_HAL_STATUS_OK;
}

/**
 * @brief SPI读操作
 */
sx128x_hal_status_t sx1280_hal_read(const void* context, 
                                    uint8_t* data, 
                                    uint16_t data_length)
{
    HAL_StatusTypeDef status;
    
    /* 等待BUSY信号 */
    sx1280_hal_wait_on_busy(context);
    
    /* 拉低NSS */
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_RESET);
    
    /* 接收数据 */
    status = HAL_SPI_Receive(&hspi1, data, data_length, HAL_MAX_DELAY);
    
    /* 拉高NSS */
    HAL_GPIO_WritePin(SX1280_NSS_GPIO_Port, SX1280_NSS_Pin, GPIO_PIN_SET);
    
    if (status != HAL_OK) {
        return SX128X_HAL_STATUS_ERROR;
    }
    
    return SX128X_HAL_STATUS_OK;
}

/**
 * @brief 等待BUSY信号
 */
sx128x_hal_status_t sx1280_hal_wait_on_busy(const void* context)
{
    uint32_t timeout = 10000;  // 10ms超时
    
    while (HAL_GPIO_ReadPin(SX1280_BUSY_GPIO_Port, SX1280_BUSY_Pin) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            return SX128X_HAL_STATUS_ERROR;
        }
        sx1280_hal_delay_us(1);
    }
    
    return SX128X_HAL_STATUS_OK;
}

/**
 * @brief 毫秒延时
 */
void sx1280_hal_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief 微秒延时
 */
void sx1280_hal_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    
    while ((DWT->CYCCNT - start) < cycles);
}

/**
 * @brief 获取系统时间（毫秒）
 */
uint32_t sx1280_hal_get_time_ms(void)
{
    return HAL_GetTick();
}

/**
 * @brief 使能DIO1中断
 */
void sx1280_hal_dio1_irq_enable(void)
{
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
}

/**
 * @brief 禁用DIO1中断
 */
void sx1280_hal_dio1_irq_disable(void)
{
    HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
}

/**
 * @brief 清除DIO1中断标志
 */
void sx1280_hal_dio1_irq_clear(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(SX1280_DIO1_Pin);
    sx1280_dio1_flag = false;
}

/**
 * @brief GPIO中断回调
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == SX1280_DIO1_Pin) {
        sx1280_dio1_flag = true;
        /* 通知上层 */
        extern void sx1280_dio1_irq_handler(void);
        sx1280_dio1_irq_handler();
    }
}

/**
 * @brief EXTI中断处理函数
 */
void EXTI0_1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(SX1280_DIO1_Pin);
}