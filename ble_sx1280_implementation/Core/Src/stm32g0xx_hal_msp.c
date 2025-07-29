/**
 * @file    stm32g0xx_hal_msp.c
 * @brief   HAL MSP模块
 * @author  AI Assistant
 * @date    2024
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/**
 * @brief 初始化全局MSP
 */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

/**
 * @brief SPI MSP初始化
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if(hspi->Instance == SPI1)
    {
        /* 使能时钟 */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        /* SPI1 GPIO配置
         * PA5 ------> SPI1_SCK
         * PA6 ------> SPI1_MISO
         * PA7 ------> SPI1_MOSI
         */
        GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/**
 * @brief SPI MSP反初始化
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    if(hspi->Instance == SPI1)
    {
        /* 禁用时钟 */
        __HAL_RCC_SPI1_CLK_DISABLE();
        
        /* 反初始化GPIO */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    }
}

/**
 * @brief UART MSP初始化
 */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if(huart->Instance == USART2)
    {
        /* 使能时钟 */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        /* USART2 GPIO配置
         * PA2 ------> USART2_TX
         * PA3 ------> USART2_RX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/**
 * @brief UART MSP反初始化
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if(huart->Instance == USART2)
    {
        /* 禁用时钟 */
        __HAL_RCC_USART2_CLK_DISABLE();
        
        /* 反初始化GPIO */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
    }
}

/**
 * @brief TIM MSP初始化
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance == TIM2)
    {
        /* 使能时钟 */
        __HAL_RCC_TIM2_CLK_ENABLE();
        
        /* 配置中断 */
        HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
}

/**
 * @brief TIM MSP反初始化
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance == TIM2)
    {
        /* 禁用时钟 */
        __HAL_RCC_TIM2_CLK_DISABLE();
        
        /* 禁用中断 */
        HAL_NVIC_DisableIRQ(TIM2_IRQn);
    }
}

/**
 * @brief LPTIM MSP初始化
 */
void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef* hlptim)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    if(hlptim->Instance == LPTIM1)
    {
        /* 配置时钟源 */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
        PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
        
        /* 使能时钟 */
        __HAL_RCC_LPTIM1_CLK_ENABLE();
        
        /* 配置中断 */
        HAL_NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_LPTIM1_IRQn);
    }
}

/**
 * @brief LPTIM MSP反初始化
 */
void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef* hlptim)
{
    if(hlptim->Instance == LPTIM1)
    {
        /* 禁用时钟 */
        __HAL_RCC_LPTIM1_CLK_DISABLE();
        
        /* 禁用中断 */
        HAL_NVIC_DisableIRQ(TIM6_DAC_LPTIM1_IRQn);
    }
}