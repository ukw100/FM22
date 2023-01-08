/*------------------------------------------------------------------------------------------------------------------------
 * booster.c - handle booster
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022 Frank Meyer - frank(at)uclock.de
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
 *------------------------------------------------------------------------------------------------------------------------
 * L6201 booster
 *
 * L6201 pins:
 *    +-----------------+---------------------------+
 *    | L6201 pins      | Remarks                   |
 *    +-----------------+---------------------------+
 *    | B+              | Battery + (not used)      |
 *    | GND             | GND                       |
 *    | EN              | Enable                    |
 *    | RPWM            | Right PWM                 |
 *    | LPWM            | Left PWM                  |
 *    | CT              | Current Sense             |
 *    | VT              | Voltage Sense (not used)  |
 *    +-------------------------+-------------------+
 *
 *    +-----------------+---------------------------+
 *    | Device          | STM32F401CC Black Pill    |
 *    +-----------------+---------------------------+
 *    | Booster         |                           |
 *    | Booster L_PWM   | GPIO          PA4         |
 *    | Booster R_PWM   | GPIO          PA5         |
 *    | Booster ENABLE  | GPIO          PA6         |
 *    | Booster CT      | GPIO          PA7         |
 *    | Booster LED     | GPIO          PC13        |
 *    +-----------------+---------------------------+
 *------------------------------------------------------------------------------------------------------------------------
 */
#include "board-led.h"
#include "rc-detector.h"
#include "booster.h"
#include "io.h"

#define BOOSTER_PERIPH_CLOCK_CMD    RCC_AHB1PeriphClockCmd

#if defined STM32F407VE

#define BOOSTER_PERIPH              RCC_AHB1Periph_GPIOC
#define BOOSTER_PORT                GPIOC

#define BOOSTER_L_PIN               GPIO_Pin_0
#define BOOSTER_R_PIN               GPIO_Pin_1
#define BOOSTER_E_PIN               GPIO_Pin_2

#elif defined STM32F401CC

#define BOOSTER_PERIPH              RCC_AHB1Periph_GPIOA
#define BOOSTER_PORT                GPIOA

#define BOOSTER_L_PIN               GPIO_Pin_4
#define BOOSTER_R_PIN               GPIO_Pin_5
#define BOOSTER_E_PIN               GPIO_Pin_6

#else
#error unknown STM32
#endif

#define BOOSTER_L_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_L_PIN)
#define BOOSTER_L_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_L_PIN)
#define BOOSTER_R_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_R_PIN)
#define BOOSTER_R_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_R_PIN)
#define BOOSTER_E_LOW               GPIO_RESET_BIT(BOOSTER_PORT, BOOSTER_E_PIN)
#define BOOSTER_E_HIGH              GPIO_SET_BIT(BOOSTER_PORT, BOOSTER_E_PIN)

volatile uint_fast8_t               booster_is_on = 0;      // volatile: changed in ISR

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC routines:
 *
 * maximum ADC frequency: 36MHz
 * base frequency:        84MHz (APB2)
 * possible prescaler:    ADC_Prescaler_Div2 - frequency:  42 MHz
 *                        ADC_Prescaler_Div4 - frequency:  21 MHz
 *                        ADC_Prescaler_Div6 - frequency:  14 MHz
 *                        ADC_Prescaler_Div8 - frequency: 10.5 MHz
 *
 * Channel           ADC1    ADC2    ADC3
 * APB                2       2       2
 * ADC Channel 0     PA0     PA0     PA0
 * ADC Channel 1     PA1     PA1     PA1
 * ADC Channel 2     PA2     PA2     PA2
 * ADC Channel 3     PA3     PA3     PA3
 * ADC Channel 4     PA4     PA4     PF6
 * ADC Channel 5     PA5     PA5     PF7
 * ADC Channel 6     PA6     PA6     PF8
 * ADC Channel 7     PA7     PA7     PF9
 * ADC Channel 8     PB0     PB0     PF10
 * ADC Channel 9     PB1     PB1     PF3
 * ADC Channel 10    PC0     PC0     PC0
 * ADC Channel 11    PC1     PC1     PC1
 * ADC Channel 12    PC2     PC2     PC2
 * ADC Channel 13    PC3     PC3     PC3
 * ADC Channel 14    PC4     PC4     PF4
 * ADC Channel 15    PC5     PC5     PF5
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADC_PRESCALER                   ADC_Prescaler_Div4

#if defined STM32F407VE
#define ADC_NUMBER                      ADC1
#define ADC_PORT                        GPIOC
#define ADC_PIN                         GPIO_Pin_4
#define ADC_ADC_CLOCK_CMD               RCC_APB2PeriphClockCmd
#define ADC_ADC_CLOCK                   RCC_APB2Periph_ADC1
#define ADC_GPIO_CLOCK_CMD              RCC_AHB1PeriphClockCmd
#define ADC_GPIO_CLOCK                  RCC_AHB1Periph_GPIOC
#define ADC_CHANNEL                     ADC_Channel_14

#elif defined STM32F401CC

#define ADC_NUMBER                      ADC1
#define ADC_PORT                        GPIOA
#define ADC_PIN                         GPIO_Pin_7
#define ADC_ADC_CLOCK_CMD               RCC_APB2PeriphClockCmd
#define ADC_ADC_CLOCK                   RCC_APB2Periph_ADC1
#define ADC_GPIO_CLOCK_CMD              RCC_AHB1PeriphClockCmd
#define ADC_GPIO_CLOCK                  RCC_AHB1Periph_GPIOA
#define ADC_CHANNEL                     ADC_Channel_7

#endif

static uint_fast8_t                     adc_conversion_started = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * start conversion
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
booster_adc_start_single_conversion (void)
{
    if (! adc_conversion_started)
    {
        ADC_SoftwareStartConv (ADC_NUMBER);
        adc_conversion_started = 1;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * poll adc value
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
booster_adc_poll_conversion_value (uint16_t * value_p)
{
    uint_fast8_t    rtc = 0;

    if (adc_conversion_started && ADC_GetFlagStatus (ADC_NUMBER, ADC_FLAG_EOC) != RESET)       // conversion started and ready?
    {
        *value_p = ADC_GetConversionValue (ADC_NUMBER);
        adc_conversion_started = 0;
        rtc = 1;
    }
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize ADC (single conversion mode)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
booster_adc_init (void)
{
    static uint_fast8_t     already_called;

    if (!already_called)
    {
        GPIO_InitTypeDef        gpio;
        ADC_InitTypeDef         adc;

        GPIO_StructInit (&gpio);
        ADC_StructInit (&adc);
        ADC_GPIO_CLOCK_CMD (ADC_GPIO_CLOCK, ENABLE);
        gpio.GPIO_Pin   = ADC_PIN;                                                  // configure pin as Analog input
        gpio.GPIO_Mode  = GPIO_Mode_AN;
        gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init (ADC_PORT, &gpio);

        ADC_ADC_CLOCK_CMD (ADC_ADC_CLOCK, ENABLE);

        ADC_CommonInitTypeDef adc_common;

        ADC_CommonStructInit (&adc_common);

        // configure ADC:
        adc_common.ADC_Mode                 = ADC_Mode_Independent;
        adc_common.ADC_Prescaler            = ADC_PRESCALER;
        adc_common.ADC_DMAAccessMode        = ADC_DMAAccessMode_Disabled;
        adc_common.ADC_TwoSamplingDelay     = ADC_TwoSamplingDelay_5Cycles;
        ADC_CommonInit(&adc_common);

        adc.ADC_Resolution             = ADC_Resolution_12b;
        adc.ADC_ScanConvMode           = DISABLE;
        adc.ADC_ContinuousConvMode     = DISABLE;
        adc.ADC_ExternalTrigConvEdge   = ADC_ExternalTrigConvEdge_None;
        adc.ADC_DataAlign              = ADC_DataAlign_Right;
        adc.ADC_NbrOfConversion        = 1;
        ADC_Init(ADC_NUMBER, &adc);
        ADC_RegularChannelConfig (ADC_NUMBER, ADC_CHANNEL, 1, ADC_SampleTime_3Cycles);     // configure conversion channel
        ADC_Cmd(ADC_NUMBER, ENABLE);
    }

    already_called = 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_negative () - set voltage to -12V
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_negative (void)
{
    BOOSTER_L_HIGH;
    BOOSTER_R_LOW;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_positive () - set voltage to +12V
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_positive (void)
{
    BOOSTER_L_LOW;
    BOOSTER_R_HIGH;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_on () - enable both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_on (void)
{
    BOOSTER_E_HIGH;
    booster_is_on = 1;

#if defined STM32F407VE
    board_led2_on ();
#elif defined STM32F401CC
    board_led_on ();
#else
#error unknown STM32
#endif

}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_off () - disable both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_off (void)
{
    BOOSTER_E_LOW;
    booster_is_on = 0;

#if defined STM32F407VE
    board_led2_off ();
#elif defined STM32F401CC
    board_led_off ();
#else
#error unknown STM32
#endif

    rc_detector_reset_rc2_millis ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_cutout () - cutout both L6201
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_cutout (void)
{
    BOOSTER_L_LOW;
    BOOSTER_R_LOW;
}

/*------------------------------------------------------------------------------------------------------------------------
 * booster_init () - init
 *------------------------------------------------------------------------------------------------------------------------
 */
void
booster_init (void)
{
    GPIO_InitTypeDef gpio;

    booster_adc_init ();

    GPIO_StructInit (&gpio);
    BOOSTER_PERIPH_CLOCK_CMD (BOOSTER_PERIPH, ENABLE);
    GPIO_SET_MODE_OUT_PP(gpio, BOOSTER_L_PIN | BOOSTER_R_PIN | BOOSTER_E_PIN, GPIO_Speed_2MHz);
    GPIO_Init(BOOSTER_PORT, &gpio);
    booster_off ();
}
