/**
 * @file    system_stm32g0xx.c
 * @brief   CMSIS Cortex-M0+ Device Peripheral Access Layer System Source File
 * @author  MCD Application Team
 */

#include "stm32g0xx.h"

/* This variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate()
    2) by calling HAL API function HAL_RCC_GetHCLKFreq()
    3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
*/
uint32_t SystemCoreClock = 16000000U;

const uint32_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint32_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};

/**
 * @brief  Setup the microcontroller system.
 * @retval None
 */
void SystemInit(void)
{
    /* Configure the Vector Table location add offset address ------------------*/
#if defined(USER_VECT_TAB_ADDRESS)
    SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#endif /* USER_VECT_TAB_ADDRESS */
}

/**
 * @brief  Update SystemCoreClock variable according to Clock Register Values.
 * @retval None
 */
void SystemCoreClockUpdate(void)
{
    uint32_t tmp;
    uint32_t pllvco;
    uint32_t pllr;
    uint32_t pllsource;
    uint32_t pllm;
    uint32_t hsidiv;
    
    /* Get SYSCLK source -------------------------------------------------------*/
    switch (RCC->CFGR & RCC_CFGR_SWS)
    {
        case 0x00U:  /* HSI used as system clock */
            hsidiv = (1UL << ((READ_BIT(RCC->CR, RCC_CR_HSIDIV)) >> RCC_CR_HSIDIV_Pos));
            SystemCoreClock = (HSI_VALUE / hsidiv);
            break;
            
        case 0x08U:  /* HSE used as system clock */
            SystemCoreClock = HSE_VALUE;
            break;
            
        case 0x10U:  /* PLL used as system clock */
            /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
               SYSCLK = PLL_VCO / PLLR */
            pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
            pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1U;
            
            if(pllsource == 0x02UL) /* HSI used as PLL clock source */
            {
                pllvco = (HSI_VALUE / pllm);
            }
            else /* HSE used as PLL clock source */
            {
                pllvco = (HSE_VALUE / pllm);
            }
            
            pllvco = pllvco * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
            pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1U);
            SystemCoreClock = pllvco / pllr;
            break;
            
        case 0x18U:  /* LSI used as system clock */
            SystemCoreClock = LSI_VALUE;
            break;
            
        case 0x20U:  /* LSE used as system clock */
            SystemCoreClock = LSE_VALUE;
            break;
            
        default:
            SystemCoreClock = HSI_VALUE;
            break;
    }
    
    /* Compute HCLK clock frequency --------------------------------------------*/
    /* Get HCLK prescaler */
    tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4U)];
    /* HCLK clock frequency */
    SystemCoreClock >>= tmp;
}