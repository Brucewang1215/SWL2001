/**
 * @file    main.h
 * @brief   主程序头文件
 * @author  AI Assistant
 * @date    2024
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* GPIO定义 */
/* LED引脚（根据实际板子修改） */
#define LED_Pin                 GPIO_PIN_5
#define LED_GPIO_Port           GPIOA

/* 用户按键（根据实际板子修改） */
#define USER_BTN_Pin            GPIO_PIN_13
#define USER_BTN_GPIO_Port      GPIOC

/* SPI引脚（SPI1） */
#define SPI1_SCK_Pin            GPIO_PIN_5
#define SPI1_SCK_GPIO_Port      GPIOA
#define SPI1_MISO_Pin           GPIO_PIN_6
#define SPI1_MISO_GPIO_Port     GPIOA
#define SPI1_MOSI_Pin           GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port     GPIOA

/* UART引脚（USART2） */
#define USART2_TX_Pin           GPIO_PIN_2
#define USART2_TX_GPIO_Port     GPIOA
#define USART2_RX_Pin           GPIO_PIN_3
#define USART2_RX_GPIO_Port     GPIOA

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */