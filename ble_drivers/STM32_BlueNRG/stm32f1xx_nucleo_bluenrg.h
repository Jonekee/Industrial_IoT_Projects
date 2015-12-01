/**
  ******************************************************************************
  *
  *                        DIRECT CLONE FROM stm32f4xx_nucleo_bluenrg.h
  *
  * @file    stm32f1xx_nucleo_bluenrg.h
  * @author  CL
  * @version V1.0.0
  * @date    04-July-2014
  * @brief   This file contains definitions for SPI communication on
  *          STM32F4XX-Nucleo Kit from STMicroelectronics for BLE BlueNRG
  *          Expansion Board (reference X-NUCLEO-IDB04A1).
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1XX_NUCLEO_BLUENRG_H
#define __STM32F1XX_NUCLEO_BLUENRG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"                         // WVD Change for F1
#include "stm32f1xx_nucleo.h"                      // WVD Change for F1


/**
* @brief SPI communication details between Nucleo F4 and BlueNRG
*        Expansion Board.
*/
// SPI Instance
#define BNRG_SPI_INSTANCE                SPI1
#define BNRG_SPI_CLK_ENABLE()            __SPI1_CLK_ENABLE()

// SPI Configuration
#define BNRG_SPI_MODE                    SPI_MODE_MASTER
#define BNRG_SPI_DIRECTION               SPI_DIRECTION_2LINES
#define BNRG_SPI_DATASIZE                SPI_DATASIZE_8BIT
#define BNRG_SPI_CLKPOLARITY             SPI_POLARITY_LOW
#define BNRG_SPI_CLKPHASE                SPI_PHASE_1EDGE
#define BNRG_SPI_NSS                     SPI_NSS_SOFT
#define BNRG_SPI_FIRSTBIT                SPI_FIRSTBIT_MSB
#define BNRG_SPI_TIMODE                  SPI_TIMODE_DISABLED
#define BNRG_SPI_CRCPOLYNOMIAL           7
#define BNRG_SPI_BAUDRATEPRESCALER       SPI_BAUDRATEPRESCALER_4
#define BNRG_SPI_CRCCALCULATION          SPI_CRCCALCULATION_DISABLED

// SPI Reset Pin: PA.8
#define BNRG_SPI_RESET_PIN               GPIO_PIN_8
#define BNRG_SPI_RESET_MODE              GPIO_MODE_OUTPUT_PP
#define BNRG_SPI_RESET_PULL              GPIO_PULLUP  /*[RG:]fixme!!! GPIO_PULLUP (original GPIO_NOPULL)*/
#define BNRG_SPI_RESET_SPEED             GPIO_SPEED_LOW
#define BNRG_SPI_RESET_ALTERNATE         0
#define BNRG_SPI_RESET_PORT              GPIOA
#define BNRG_SPI_RESET_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

// SCLK: PB.3
#define BNRG_SPI_SCLK_PIN                GPIO_PIN_3
#define BNRG_SPI_SCLK_MODE               GPIO_MODE_AF_PP
#define BNRG_SPI_SCLK_PULL               GPIO_PULLDOWN //[RG:] fixme!!! GPIO_PULLDOWN (original GPIO_PULLUP)
#define BNRG_SPI_SCLK_SPEED              GPIO_SPEED_HIGH
#define BNRG_SPI_SCLK_ALTERNATE          GPIO_AF5_SPI1
#define BNRG_SPI_SCLK_PORT               GPIOB
#define BNRG_SPI_SCLK_CLK_ENABLE()       __GPIOB_CLK_ENABLE()

// MISO (Master Input Slave Output): PA.6
#define BNRG_SPI_MISO_PIN                GPIO_PIN_6
#define BNRG_SPI_MISO_MODE               GPIO_MODE_AF_PP
#define BNRG_SPI_MISO_PULL               GPIO_NOPULL //[RG:] fixme!!! GPIO_NOPULL (original GPIO_PULLDOWN)
#define BNRG_SPI_MISO_SPEED              GPIO_SPEED_HIGH
#define BNRG_SPI_MISO_ALTERNATE          GPIO_AF5_SPI1
#define BNRG_SPI_MISO_PORT               GPIOA
#define BNRG_SPI_MISO_CLK_ENABLE()       __GPIOA_CLK_ENABLE()


// MOSI (Master Output Slave Input): PA.7
#define BNRG_SPI_MOSI_PIN                GPIO_PIN_7
#define BNRG_SPI_MOSI_MODE               GPIO_MODE_AF_PP
/*[RG:] sometimes at the end of a SPI receive phase MOSI is left high (even if the NCS rises)*/
#define BNRG_SPI_MOSI_PULL               GPIO_NOPULL //[RG:] fixme!!! GPIO_PULLDOWN/GPIO_NOPULL (original GPIO_PULLUP)
#define BNRG_SPI_MOSI_SPEED              GPIO_SPEED_HIGH
#define BNRG_SPI_MOSI_ALTERNATE          GPIO_AF5_SPI1
#define BNRG_SPI_MOSI_PORT               GPIOA
#define BNRG_SPI_MOSI_CLK_ENABLE()       __GPIOA_CLK_ENABLE()

// NSS/CSN/CS: PA.1
#define BNRG_SPI_CS_PIN                  GPIO_PIN_1
#define BNRG_SPI_CS_MODE                 GPIO_MODE_OUTPUT_PP
#define BNRG_SPI_CS_PULL                 GPIO_PULLUP /*[RG:]fixme!!! GPIO_PULLUP (original GPIO_NOPULL)*/
#define BNRG_SPI_CS_SPEED                GPIO_SPEED_HIGH
#define BNRG_SPI_CS_ALTERNATE            0
#define BNRG_SPI_CS_PORT                 GPIOA
#define BNRG_SPI_CS_CLK_ENABLE()         __GPIOA_CLK_ENABLE()

// IRQ: PA.0
#define BNRG_SPI_IRQ_PIN                 GPIO_PIN_0
#define BNRG_SPI_IRQ_MODE                GPIO_MODE_IT_RISING
#define BNRG_SPI_IRQ_PULL                GPIO_NOPULL
#define BNRG_SPI_IRQ_SPEED               GPIO_SPEED_HIGH
#define BNRG_SPI_IRQ_ALTERNATE           0
#define BNRG_SPI_IRQ_PORT                GPIOA
#define BNRG_SPI_IRQ_CLK_ENABLE()        __GPIOA_CLK_ENABLE()

// EXTI External Interrupt for SPI
// NOTE: if you change the IRQ pin remember to implement a corresponding handler
// function like EXTI0_IRQHandler() in the user project
#define BNRG_SPI_EXTI_IRQn            EXTI0_IRQn      // def for F1_03 and F4_01
#define BNRG_SPI_EXTI_IRQHandler      EXTI0_IRQHandler   // def for F1_03, F4_01
//#define BNRG_SPI_EXTI_IRQn              EXTI0_1_IRQn      // def for F0_72 WVD Change for F0
//#define BNRG_SPI_EXTI_IRQHandler        EXTI0_IRQHandler  // def for F0_72
#define BNRG_SPI_EXTI_PIN               BNRG_SPI_IRQ_PIN
#define BNRG_SPI_EXTI_PORT              BNRG_SPI_IRQ_PORT
#define RTC_WAKEUP_IRQHandler           RTC_WKUP_IRQHandler

void    Enable_SPI_IRQ(void);
void    Disable_SPI_IRQ(void);
void    Clear_SPI_IRQ(void);
void    Clear_SPI_EXTI_Flag(void);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1XX_NUCLEO_BLUENRG_H */

    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
