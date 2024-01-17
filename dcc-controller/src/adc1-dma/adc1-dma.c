/*-------------------------------------------------------------------------------------------------------------------------------------------
 * adc1-dma.c - ADC DMA
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022-2024 Frank Meyer - frank(at)uclock.de
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include "adc1-dma.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC register addresses:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC_NUMBER                  ADC1
#define ADC_BASE_ADDR               ((uint32_t) 0x40012000)                         // ADC base address
#define ADC_ADC1_OFFSET             ((uint32_t) 0x00000000)                         // ADC1 offset to base address
#define ADC_REG_DR_OFFSET           ((uint32_t) 0x0000004C)                         // address oof register
#define ADC1_DR_ADDRESS             (ADC_BASE_ADDR | ADC_ADC1_OFFSET | ADC_REG_DR_OFFSET)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * maximum ADC frequency:   36 MHz
 * base frequency:          APB2 (APB2 = 84 MHz)
 * possible dividers:
 *      ADC_Prescaler_Div2  42 MHz
 *      ADC_Prescaler_Div4  21 MHz
 *      ADC_Prescaler_Div6  14 MHz
 *      ADC_Prescaler_Div8  10.5 MHz
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC1_DIV                    ADC_Prescaler_Div4                              // 21 MHz

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC1 GPIO clocks & ports
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC_GPIO_CLOCK_CMD          RCC_AHB1PeriphClockCmd
#define ADC_GPIO_CLOCK              RCC_AHB1Periph_GPIOA
#define ADC_PORT                    GPIOA
#define ADC_PINS                    (GPIO_Pin_7)                                    // add more here
#define ADC_FIRST_CHANNEL           ADC_Channel_7                                   // begin with channel 7 (here: the only one)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC1 clocks
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC_ADC_CLOCK_CMD           RCC_APB2PeriphClockCmd
#define ADC_ADC_CLOCK               RCC_APB2Periph_ADC1

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC1 DMA channels & streams
 *
 * possible streams for ADC1:
 *      DMA2_Stream0 and DMA_Channel_0
 *      DMA2_Stream4 and DMA_Channel_0
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC1_DMA_STREAM             DMA2_Stream0
#define ADC1_DMA_CHANNEL            DMA_Channel_0
#define ADC_DMA_CLOCK_CMD           RCC_AHB1PeriphClockCmd
#define ADC_DMA_CLOCK               RCC_AHB1Periph_DMA2

volatile uint32_t                   adc1_dma_buffer[NUMBER_OF_ADC1_CHANNELS];

static void
adc1_dma_init_gpio  (void)
{
    GPIO_InitTypeDef    gpio;

    ADC_GPIO_CLOCK_CMD (ADC_GPIO_CLOCK, ENABLE);

    GPIO_StructInit (&gpio);
    gpio.GPIO_Pin   = ADC_PINS;                                                 // configure pins as Analog input
    gpio.GPIO_Mode  = GPIO_Mode_AN;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;                                         // GPIO_PuPd_NOPULL
    GPIO_Init (ADC_PORT, &gpio);
}

static void
adc1_dma_init_adc (void)
{
    ADC_CommonInitTypeDef   adc_common;
    ADC_InitTypeDef         adc;
    uint8_t                 adc_channel;

    ADC_ADC_CLOCK_CMD (ADC_ADC_CLOCK, ENABLE);

    ADC_CommonStructInit (&adc_common);
    adc_common.ADC_Mode             = ADC_Mode_Independent;
    adc_common.ADC_Prescaler        = ADC1_DIV;
    adc_common.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled;
    adc_common.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit (&adc_common);

    ADC_StructInit (&adc);
    adc.ADC_Resolution              = ADC_Resolution_12b;
    adc.ADC_ScanConvMode            = ENABLE;
    adc.ADC_ContinuousConvMode      = ENABLE;
    adc.ADC_ExternalTrigConvEdge    = ADC_ExternalTrigConvEdge_None;
    adc.ADC_DataAlign               = ADC_DataAlign_Right;
    adc.ADC_NbrOfConversion         = NUMBER_OF_ADC1_CHANNELS;
    ADC_Init (ADC_NUMBER, &adc);

    for (adc_channel = 0; adc_channel < NUMBER_OF_ADC1_CHANNELS; adc_channel++)
    {
        ADC_RegularChannelConfig (ADC_NUMBER, ADC_FIRST_CHANNEL + adc_channel, adc_channel + 1, ADC_SampleTime_28Cycles);    // faster, but more DMA traffic: ADC_SampleTime_3Cycles
    }
}

static void
adc1_dma_init_dma (void)
{
    DMA_InitTypeDef     dma;

    ADC_DMA_CLOCK_CMD (ADC_DMA_CLOCK, ENABLE);

    DMA_Cmd (ADC1_DMA_STREAM, DISABLE);
    DMA_DeInit (ADC1_DMA_STREAM);

    DMA_StructInit (&dma);
    dma.DMA_Channel                 = ADC1_DMA_CHANNEL;
    dma.DMA_PeripheralBaseAddr      = (uint32_t) ADC1_DR_ADDRESS;
    dma.DMA_Memory0BaseAddr         = (uint32_t) adc1_dma_buffer;
    dma.DMA_DIR                     = DMA_DIR_PeripheralToMemory;
    dma.DMA_BufferSize              = NUMBER_OF_ADC1_CHANNELS;
    dma.DMA_PeripheralInc           = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc               = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize      = DMA_PeripheralDataSize_Word;
    dma.DMA_MemoryDataSize          = DMA_MemoryDataSize_Word;
    dma.DMA_Mode                    = DMA_Mode_Circular;
    dma.DMA_Priority                = DMA_Priority_High;
    dma.DMA_FIFOMode                = DMA_FIFOMode_Disable;
    dma.DMA_FIFOThreshold           = DMA_FIFOThreshold_HalfFull;
    dma.DMA_MemoryBurst             = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst         = DMA_PeripheralBurst_Single;
    DMA_Init (ADC1_DMA_STREAM, &dma);

    DMA_Cmd (ADC1_DMA_STREAM, ENABLE);
}

static void
adc1_dma_init_start (void)
{
    ADC_DMARequestAfterLastTransferCmd (ADC_NUMBER, ENABLE);
    ADC_DMACmd (ADC_NUMBER, ENABLE);
    ADC_Cmd (ADC_NUMBER, ENABLE);
    ADC_SoftwareStartConv (ADC_NUMBER);
}

void
adc1_dma_init (void)
{
    adc1_dma_init_gpio ();
    adc1_dma_init_dma ();
    adc1_dma_init_adc ();
    adc1_dma_init_start ();
}

