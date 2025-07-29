/**
 * @file    main.c
 * @brief   BLE手环通信主程序（基于STM32G0 + SX1280）
 * @author  AI Assistant
 * @date    2024
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "ble_app.h"
#include "sx1280_hal_stm32g0.h"
#include "sx128x.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim2;
LPTIM_HandleTypeDef hlptim1;
IWDG_HandleTypeDef hiwdg;

/* BLE应用上下文 */
static app_context_t g_ble_app;
static sx128x_t g_sx1280;

/* 已知手环地址（需要根据实际设备修改） */
static const uint8_t TARGET_BRACELET_ADDR[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

/* 预定义消息 */
static const char* messages[] = {
    "Hello Bracelet!",
    "Heart Rate: 72",
    "Steps: 5000",
    "Call from John",
    "Low Battery!"
};
static uint8_t msg_index = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_IWDG_Init(void);
static void Error_Handler(void);

/* 应用回调函数 */
static void on_ble_connected(void);
static void on_ble_disconnected(uint8_t reason);
static void on_text_sent(void);
static void on_text_received(const char* text);
static void on_ble_error(ble_status_t error);

/* UART命令处理 */
static void process_uart_command(char* cmd);

/* SX1280中断处理 */
void sx1280_dio1_irq_handler(void);

/**
 * @brief  主函数
 */
int main(void)
{
    /* MCU配置 --------------------------------------------------------*/
    
    /* 复位所有外设，初始化Flash接口和Systick */
    HAL_Init();
    
    /* 配置系统时钟 */
    SystemClock_Config();
    
    /* 初始化所有配置的外设 */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    MX_LPTIM1_Init();
    MX_IWDG_Init();
    
    /* 使能DWT用于微秒计时 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    /* 打印启动信息 */
    printf("\n\n");
    printf("=====================================\n");
    printf("   BLE Bracelet Communicator v1.0   \n");
    printf("   STM32G0 + SX1280 Implementation  \n");
    printf("=====================================\n\n");
    
    /* 初始化SX1280 HAL */
    printf("Initializing SX1280...\n");
    sx1280_hal_init();
    
    /* 初始化SX1280驱动 */
    sx128x_hal_t sx1280_hal = {
        .reset = sx1280_hal_reset,
        .wakeup = sx1280_hal_wakeup,
        .write = sx1280_hal_write,
        .read = sx1280_hal_read,
        .wait_on_busy = sx1280_hal_wait_on_busy,
    };
    
    if (sx128x_init(&g_sx1280, &sx1280_hal) != SX128X_STATUS_OK) {
        printf("ERROR: SX1280 initialization failed!\n");
        Error_Handler();
    }
    
    /* 验证芯片ID */
    // TODO: 读取并验证芯片ID
    
    printf("SX1280 initialized successfully\n");
    
    /* 配置SX1280为BLE模式 */
    sx128x_set_standby(&g_sx1280, SX128X_STANDBY_RC);
    sx128x_set_pkt_type(&g_sx1280, SX128X_PKT_TYPE_BLE);
    sx128x_set_rf_freq(&g_sx1280, 2402000000);  // 2.402 GHz (Channel 37)
    
    /* 设置BLE调制参数 */
    sx128x_mod_params_ble_t ble_mod_params = {
        .br_bw = SX128X_BLE_BR_1_000_BW_1_2,     // 1 Mbps
        .mod_ind = SX128X_BLE_MOD_IND_0_50,      // Modulation index 0.5
        .pulse_shape = SX128X_BLE_PULSE_SHAPE_OFF
    };
    sx128x_set_ble_mod_params(&g_sx1280, &ble_mod_params);
    
    /* 设置输出功率 */
    sx128x_set_tx_params(&g_sx1280, 0, SX128X_RAMP_TIME_10_US);  // 0 dBm
    
    /* 初始化BLE应用 */
    printf("\nInitializing BLE stack...\n");
    
    app_config_t ble_config;
    ble_app_get_default_config(&ble_config);
    
    /* 设置目标手环 */
    memcpy(ble_config.target_addr, TARGET_BRACELET_ADDR, 6);
    ble_config.bracelet_type = BRACELET_TYPE_NORDIC_UART;  // 假设使用Nordic UART
    
    /* 设置连接参数 */
    ble_config.conn_interval_ms = 50;
    ble_config.slave_latency = 0;
    ble_config.supervision_timeout_ms = 5000;
    ble_config.disconnect_after_send = true;
    
    /* 设置回调 */
    g_ble_app.on_connected = on_ble_connected;
    g_ble_app.on_disconnected = on_ble_disconnected;
    g_ble_app.on_text_sent = on_text_sent;
    g_ble_app.on_text_received = on_text_received;
    g_ble_app.on_error = on_ble_error;
    
    /* 设置无线电上下文 */
    g_ble_app.ble_conn.radio_context = &g_sx1280;
    
    if (ble_app_init(&g_ble_app, &ble_config) != BLE_STATUS_OK) {
        printf("ERROR: BLE app initialization failed!\n");
        Error_Handler();
    }
    
    printf("BLE stack initialized\n");
    printf("\nCommands:\n");
    printf("  scan         - Start scanning for devices\n");
    printf("  connect      - Connect to target device\n");
    printf("  send <text>  - Send text message\n");
    printf("  disconnect   - Disconnect from device\n");
    printf("  status       - Show current status\n");
    printf("\nPress USER button to send preset message\n\n");
    
    /* 启动看门狗 */
    HAL_IWDG_Refresh(&hiwdg);
    
    /* UART接收缓冲 */
    char uart_buffer[128];
    uint8_t uart_index = 0;
    
    /* 主循环 */
    while (1)
    {
        /* 处理UART命令 */
        if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
            char c = (char)(huart2.Instance->RDR & 0xFF);
            
            if (c == '\n' || c == '\r') {
                if (uart_index > 0) {
                    uart_buffer[uart_index] = '\0';
                    process_uart_command(uart_buffer);
                    uart_index = 0;
                }
            } else if (uart_index < sizeof(uart_buffer) - 1) {
                uart_buffer[uart_index++] = c;
                /* 回显字符 */
                HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10);
            }
        }
        
        /* 处理用户按键 */
        if (HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin) == GPIO_PIN_RESET) {
            /* 按键按下 */
            if (ble_app_get_state(&g_ble_app) == APP_STATE_IDLE) {
                /* 开始扫描并连接 */
                printf("\nButton pressed - Starting scan...\n");
                ble_app_start_scan(&g_ble_app);
            } else if (ble_app_get_state(&g_ble_app) == APP_STATE_CONNECTED) {
                /* 发送预设消息 */
                printf("\nSending: %s\n", messages[msg_index]);
                ble_app_send_text(&g_ble_app, messages[msg_index]);
                msg_index = (msg_index + 1) % (sizeof(messages) / sizeof(messages[0]));
            }
            
            /* 等待按键释放 */
            while (HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin) == GPIO_PIN_RESET) {
                HAL_Delay(10);
            }
            HAL_Delay(50);  // 消抖
        }
        
        /* 处理BLE事件 */
        ble_app_process(&g_ble_app);
        
        /* LED状态指示 */
        static uint32_t led_timer = 0;
        if (HAL_GetTick() - led_timer > 500) {
            led_timer = HAL_GetTick();
            
            switch (ble_app_get_state(&g_ble_app)) {
                case APP_STATE_IDLE:
                    /* 慢闪 */
                    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
                    break;
                    
                case APP_STATE_SCANNING:
                case APP_STATE_CONNECTING:
                    /* 快闪 */
                    if ((led_timer / 100) % 2) {
                        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
                    }
                    break;
                    
                case APP_STATE_CONNECTED:
                    /* 常亮 */
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
                    break;
                    
                case APP_STATE_ERROR:
                    /* 双闪 */
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
                    HAL_Delay(100);
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                    HAL_Delay(100);
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
                    HAL_Delay(100);
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                    break;
                    
                default:
                    break;
            }
        }
        
        /* 喂狗 */
        HAL_IWDG_Refresh(&hiwdg);
        
        /* 低功耗模式（如果空闲） */
        if (ble_app_get_state(&g_ble_app) == APP_STATE_IDLE ||
            ble_app_get_state(&g_ble_app) == APP_STATE_CONNECTED) {
            __WFI();  // 等待中断
        }
    }
}

/**
 * @brief 系统时钟配置
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* 配置电源电压检测等级 */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /* 配置HSI作为系统时钟源 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 8;  // 16MHz * 8 = 128MHz
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  // 128MHz / 2 = 64MHz
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    /* 配置时钟总线 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief SPI1初始化
 */
static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  // 8MHz SPI
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief USART2初始化
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief TIM2初始化（微秒定时器）
 */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 63;  // 64MHz / 64 = 1MHz (1μs分辨率)
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFF;  // 32位计数器
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief LPTIM1初始化（低功耗定时器）
 */
static void MX_LPTIM1_Init(void)
{
    hlptim1.Instance = LPTIM1;
    hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
    hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
    hlptim1.Init.RepetitionCounter = 0;
    
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief IWDG初始化
 */
static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Window = 4095;
    hiwdg.Init.Reload = 4095;  // ~4秒超时
    
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO初始化
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* GPIO端口时钟使能 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* 配置LED引脚 */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);
    
    /* 配置用户按键 */
    GPIO_InitStruct.Pin = USER_BTN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(USER_BTN_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief 处理UART命令
 */
static void process_uart_command(char* cmd)
{
    printf("\n");
    
    if (strcmp(cmd, "scan") == 0) {
        if (ble_app_get_state(&g_ble_app) == APP_STATE_IDLE) {
            printf("Starting scan...\n");
            ble_app_start_scan(&g_ble_app);
        } else {
            printf("Error: Not in idle state\n");
        }
    }
    else if (strcmp(cmd, "connect") == 0) {
        if (ble_app_get_state(&g_ble_app) == APP_STATE_IDLE) {
            printf("Connecting to target device...\n");
            ble_app_connect(&g_ble_app, (uint8_t*)TARGET_BRACELET_ADDR);
        } else {
            printf("Error: Not in idle state\n");
        }
    }
    else if (strncmp(cmd, "send ", 5) == 0) {
        if (ble_app_get_state(&g_ble_app) == APP_STATE_CONNECTED) {
            printf("Sending: %s\n", &cmd[5]);
            ble_app_send_text(&g_ble_app, &cmd[5]);
        } else {
            printf("Error: Not connected\n");
        }
    }
    else if (strcmp(cmd, "disconnect") == 0) {
        if (ble_app_get_state(&g_ble_app) == APP_STATE_CONNECTED) {
            printf("Disconnecting...\n");
            ble_app_disconnect(&g_ble_app);
        } else {
            printf("Error: Not connected\n");
        }
    }
    else if (strcmp(cmd, "status") == 0) {
        printf("State: %s\n", ble_app_state_to_string(ble_app_get_state(&g_ble_app)));
        
        uint32_t sent, received, connect_time;
        ble_app_get_stats(&g_ble_app, &sent, &received, &connect_time);
        printf("Stats: Sent=%lu, Received=%lu, ConnectTime=%lums\n",
               sent, received, connect_time);
    }
    else {
        printf("Unknown command: %s\n", cmd);
    }
    
    printf("> ");
}

/**
 * @brief BLE连接成功回调
 */
static void on_ble_connected(void)
{
    printf("\n[CALLBACK] Connected to bracelet!\n> ");
}

/**
 * @brief BLE断开连接回调
 */
static void on_ble_disconnected(uint8_t reason)
{
    printf("\n[CALLBACK] Disconnected (reason: 0x%02X)\n> ", reason);
}

/**
 * @brief 文本发送成功回调
 */
static void on_text_sent(void)
{
    printf("\n[CALLBACK] Text sent successfully\n> ");
}

/**
 * @brief 收到文本回调
 */
static void on_text_received(const char* text)
{
    printf("\n[CALLBACK] Received: %s\n> ", text);
}

/**
 * @brief BLE错误回调
 */
static void on_ble_error(ble_status_t error)
{
    printf("\n[CALLBACK] BLE Error: %d\n> ", error);
}

/**
 * @brief SX1280 DIO1中断处理
 */
void sx1280_dio1_irq_handler(void)
{
    /* 在中断中设置标志，主循环中处理 */
    ble_ll_radio_irq_handler(&g_ble_app.ble_conn);
}

/**
 * @brief 错误处理函数
 */
static void Error_Handler(void)
{
    __disable_irq();
    printf("\n!!! FATAL ERROR !!!\n");
    
    while (1) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}

/**
 * @brief printf重定向
 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

#ifdef USE_FULL_ASSERT
/**
 * @brief 断言失败处理
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Assertion failed: %s, line %lu\n", file, line);
    Error_Handler();
}
#endif /* USE_FULL_ASSERT */