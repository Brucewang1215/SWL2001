/**
 * @file    sx1280_hal_stm32g0.h
 * @brief   SX1280 HAL接口定义（STM32G0平台）
 * @author  AI Assistant
 * @date    2024
 */

#ifndef SX1280_HAL_STM32G0_H
#define SX1280_HAL_STM32G0_H

#include "sx128x_hal.h"
#include "stm32g0xx_hal.h"

/* GPIO定义（需要根据实际硬件连接修改） */
#define SX1280_NSS_GPIO_Port    GPIOA
#define SX1280_NSS_Pin          GPIO_PIN_4

#define SX1280_RESET_GPIO_Port  GPIOB
#define SX1280_RESET_Pin        GPIO_PIN_0

#define SX1280_BUSY_GPIO_Port   GPIOB
#define SX1280_BUSY_Pin         GPIO_PIN_1

#define SX1280_DIO1_GPIO_Port   GPIOA
#define SX1280_DIO1_Pin         GPIO_PIN_1

#define SX1280_DIO2_GPIO_Port   GPIOA
#define SX1280_DIO2_Pin         GPIO_PIN_2

#define SX1280_DIO3_GPIO_Port   GPIOA
#define SX1280_DIO3_Pin         GPIO_PIN_3

/* SPI句柄（需要在main.c中定义） */
extern SPI_HandleTypeDef hspi1;

/* 函数声明 */
void sx1280_hal_init(void);
void sx1280_hal_deinit(void);

/* SX128x HAL接口实现 */
sx128x_hal_status_t sx1280_hal_reset(const void* context);
sx128x_hal_status_t sx1280_hal_wakeup(const void* context);
sx128x_hal_status_t sx1280_hal_write(const void* context, const uint8_t* data, uint16_t data_length);
sx128x_hal_status_t sx1280_hal_read(const void* context, uint8_t* data, uint16_t data_length);
sx128x_hal_status_t sx1280_hal_wait_on_busy(const void* context);

/* 实用函数 */
void sx1280_hal_delay_ms(uint32_t ms);
void sx1280_hal_delay_us(uint32_t us);
uint32_t sx1280_hal_get_time_ms(void);

/* 中断管理 */
void sx1280_hal_dio1_irq_enable(void);
void sx1280_hal_dio1_irq_disable(void);
void sx1280_hal_dio1_irq_clear(void);

#endif /* SX1280_HAL_STM32G0_H */