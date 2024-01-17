/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485.c - RS485 routines for STM32F4XX or STM32F10X
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2015-2024 Frank Meyer - frank(at)uclock.de
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
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Possible UARTs of STM32F10x:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F401 / STM32F411:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 6    | PC6  | PC7  || PA11 | PA12 ||      |      |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F407:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  | 4    | PA0  | PA1  || PC10 | PC11 ||      |      |
 *  | 5    | PC12 | PD2  ||      |      ||      |      |
 *  | 6    | PC6  | PC7  || PG14 | PG9  ||      |      |
 *  +--------------------------------------------------+
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#include "rs485.h"
#include "rs485-config.h"

#define _CONCAT(a,b)                    a##b
#define CONCAT(a,b)                     _CONCAT(a,b)

#define RS485_TXBUFLEN                  64                                          // 64 Bytes ringbuffer for RS485_UART
#define RS485_RXBUFLEN                  128                                         // 128 Bytes ringbuffer for RS485_UART

#if defined(STM32F4XX)

#define RS485_UART_GPIO_CLOCK_CMD       RCC_AHB1PeriphClockCmd
#define RS485_UART_GPIO                 RCC_AHB1Periph_GPIO

#if RS485_UART_NUMBER == 1

#if RS485_UART_ALTERNATE == 0                                                       // A9/A10
#  define RS485_UART_TX_PORT_LETTER     A
#  define RS485_UART_TX_PIN_NUMBER      9
#  define RS485_UART_RX_PORT_LETTER     A
#  define RS485_UART_RX_PIN_NUMBER      10
#elif RS485_UART_ALTERNATE == 1                                                     // B6/B7
#  define RS485_UART_TX_PORT_LETTER     B
#  define RS485_UART_TX_PIN_NUMBER      6
#  define RS485_UART_RX_PORT_LETTER     B
#  define RS485_UART_RX_PIN_NUMBER      7
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART1
#define RS485_UART_USART_CLOCK_CMD      RCC_APB2PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB2Periph_USART1
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART1
#define RS485_UART_IRQ_HANDLER          USART1_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART1_IRQn

#elif RS485_UART_NUMBER == 2

#if RS485_UART_ALTERNATE == 0                                                       // A2/A3
#  define RS485_UART_TX_PORT_LETTER     A
#  define RS485_UART_TX_PIN_NUMBER      2
#  define RS485_UART_RX_PORT_LETTER     A
#  define RS485_UART_RX_PIN_NUMBER      3
#elif RS485_UART_ALTERNATE == 1                                                     // D5/D6
#  define RS485_UART_TX_PORT_LETTER     D
#  define RS485_UART_TX_PIN_NUMBER      5
#  define RS485_UART_RX_PORT_LETTER     D
#  define RS485_UART_RX_PIN_NUMBER      6
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART2
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_USART2
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART2
#define RS485_UART_IRQ_HANDLER          USART2_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART2_IRQn

#elif RS485_UART_NUMBER == 3

#if RS485_UART_ALTERNATE == 0                                                       // B10/B11
#  define RS485_UART_TX_PORT_LETTER     B
#  define RS485_UART_TX_PIN_NUMBER      10
#  define RS485_UART_RX_PORT_LETTER     B
#  define RS485_UART_RX_PIN_NUMBER      11
#elif RS485_UART_ALTERNATE == 1                                                     // D8/D9
#  define RS485_UART_TX_PORT_LETTER     D
#  define RS485_UART_TX_PIN_NUMBER      8
#  define RS485_UART_RX_PORT_LETTER     D
#  define RS485_UART_RX_PIN_NUMBER      9
#elif RS485_UART_ALTERNATE == 2                                                     // C10/C11
#  define RS485_UART_TX_PORT_LETTER     C
#  define RS485_UART_TX_PIN_NUMBER      10
#  define RS485_UART_RX_PORT_LETTER     C
#  define RS485_UART_RX_PIN_NUMBER      11
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART3
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_USART3
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART3
#define RS485_UART_IRQ_HANDLER          USART3_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART3_IRQn

#elif RS485_UART_NUMBER == 4

#if RS485_UART_ALTERNATE == 0                                                       // A0/A1
#  define RS485_UART_TX_PORT_LETTER     A
#  define RS485_UART_TX_PIN_NUMBER      0
#  define RS485_UART_RX_PORT_LETTER     A
#  define RS485_UART_RX_PIN_NUMBER      1
#elif RS485_UART_ALTERNATE == 1                                                     // C10/C11
#  define RS485_UART_TX_PORT_LETTER     C
#  define RS485_UART_TX_PIN_NUMBER      10
#  define RS485_UART_RX_PORT_LETTER     C
#  define RS485_UART_RX_PIN_NUMBER      11
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART4
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_UART4
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART4
#define RS485_UART_IRQ_HANDLER          USART4_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART4_IRQn

#elif RS485_UART_NUMBER == 5

#if RS485_UART_ALTERNATE == 0                                                       // C12/D2
#  define RS485_UART_TX_PORT_LETTER     C
#  define RS485_UART_TX_PIN_NUMBER      12
#  define RS485_UART_RX_PORT_LETTER     D
#  define RS485_UART_RX_PIN_NUMBER      2
#elif RS485_UART_ALTERNATE == 1                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART5
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_UART5
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART5
#define RS485_UART_IRQ_HANDLER          USART5_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART5_IRQn

#elif RS485_UART_NUMBER == 6

#if RS485_UART_ALTERNATE == 0                                                       // C6/C7
#  define RS485_UART_TX_PORT_LETTER     C
#  define RS485_UART_TX_PIN_NUMBER      6
#  define RS485_UART_RX_PORT_LETTER     C
#  define RS485_UART_RX_PIN_NUMBER      7
#elif RS485_UART_ALTERNATE == 1
#  if defined(STM32F401)                                                            // A11/A12
#    define RS485_UART_TX_PORT_LETTER   A
#    define RS485_UART_TX_PIN_NUMBER    11
#    define RS485_UART_RX_PORT_LETTER   A
#    define RS485_UART_RX_PIN_NUMBER    12
#  elif defined(STM32F407)                                                          // G14/G9
#    define RS485_UART_TX_PORT_LETTER   G
#    define RS485_UART_TX_PIN_NUMBER    14
#    define RS485_UART_RX_PORT_LETTER   G
#    define RS485_UART_RX_PIN_NUMBER    9
#  else
#    error unknown STM32 for UART6 ALT1
#  endif
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART6
#define RS485_UART_USART_CLOCK_CMD      RCC_APB2PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB2Periph_USART6
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART6
#define RS485_UART_IRQ_HANDLER          USART6_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART6_IRQn

#else
#error wrong number for RS485_UART_NUMBER, choose 1-6
#endif

#elif defined(STM32F10X)

#define RS485_UART_GPIO_CLOCK_CMD       RCC_APB2PeriphClockCmd
#define RS485_UART_GPIO                 RCC_APB2Periph_GPIO

#if RS485_UART_NUMBER == 1

#if RS485_UART_ALTERNATE == 0                                                       // A9/A10
#  define RS485_UART_TX_PORT_LETTER     A
#  define RS485_UART_TX_PIN_NUMBER      9
#  define RS485_UART_RX_PORT_LETTER     A
#  define RS485_UART_RX_PIN_NUMBER      10
#elif RS485_UART_ALTERNATE == 1                                                     // B6/B7
#  define RS485_UART_TX_PORT_LETTER     B
#  define RS485_UART_TX_PIN_NUMBER      6
#  define RS485_UART_RX_PORT_LETTER     B
#  define RS485_UART_RX_PIN_NUMBER      7
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART1
#define RS485_UART_USART_CLOCK_CMD      RCC_APB2PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB2Periph_USART1
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART1
#define RS485_UART_IRQ_HANDLER          USART1_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART1_IRQn

#elif RS485_UART_NUMBER == 2

#if RS485_UART_ALTERNATE == 0                                                       // A2/A3
#  define RS485_UART_TX_PORT_LETTER     A
#  define RS485_UART_TX_PIN_NUMBER      2
#  define RS485_UART_RX_PORT_LETTER     A
#  define RS485_UART_RX_PIN_NUMBER      3
#elif RS485_UART_ALTERNATE == 1                                                     // D5/D6
#  define RS485_UART_TX_PORT_LETTER     D
#  define RS485_UART_TX_PIN_NUMBER      5
#  define RS485_UART_RX_PORT_LETTER     D
#  define RS485_UART_RX_PIN_NUMBER      6
#elif RS485_UART_ALTERNATE == 2                                                     // not defined
#  error wrong RS485_UART_ALTERNATE value
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART2
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_USART2
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART2
#define RS485_UART_IRQ_HANDLER          USART2_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART2_IRQn

#elif RS485_UART_NUMBER == 3

#if RS485_UART_ALTERNATE == 0                                                       // B10/B11
#  define RS485_UART_TX_PORT_LETTER     B
#  define RS485_UART_TX_PIN_NUMBER      10
#  define RS485_UART_RX_PORT_LETTER     B
#  define RS485_UART_RX_PIN_NUMBER      11
#elif RS485_UART_ALTERNATE == 1                                                     // C10/C11
#  define RS485_UART_TX_PORT_LETTER     C
#  define RS485_UART_TX_PIN_NUMBER      10
#  define RS485_UART_RX_PORT_LETTER     C
#  define RS485_UART_RX_PIN_NUMBER      11
#elif RS485_UART_ALTERNATE == 2                                                     // D8/D9
#  define RS485_UART_TX_PORT_LETTER     D
#  define RS485_UART_TX_PIN_NUMBER      8
#  define RS485_UART_RX_PORT_LETTER     D
#  define RS485_UART_RX_PIN_NUMBER      9
#else
#  error wrong RS485_UART_ALTERNATE value
#endif

#define RS485_UART_NAME                 USART3
#define RS485_UART_USART_CLOCK_CMD      RCC_APB1PeriphClockCmd
#define RS485_UART_USART_CLOCK          RCC_APB1Periph_USART3
#define RS485_UART_GPIO_AF_UART         GPIO_AF_USART3
#define RS485_UART_IRQ_HANDLER          USART3_IRQHandler
#define RS485_UART_IRQ_CHANNEL          USART3_IRQn

#else
#error wrong number for RS485_UART_NUMBER, choose 1-3
#endif

#if RS485_UART_NUMBER == 3
#  if RS485_UART_ALTERNATE == 0
#    define RS485_UART_GPIO_REMAP       CONCAT(GPIO_Remap_USART, RS485_UART_NUMBER)
#  elif RS485_UART_ALTERNATE == 1
#    define RS485_UART_GPIO_REMAP       CONCAT(GPIO_PartialRemap_USART, RS485_UART_NUMBER)
#  elif RS485_UART_ALTERNATE == 2
#    define RS485_UART_GPIO_REMAP       CONCAT(GPIO_FullRemap_USART, RS485_UART_NUMBER)
#  endif
#else
#  define RS485_UART_GPIO_REMAP         CONCAT(GPIO_Remap_USART, RS485_UART_NUMBER)
#endif

#endif

#define RS485_UART_TX_PORT              CONCAT(GPIO, RS485_UART_TX_PORT_LETTER)
#define RS485_UART_TX_GPIO_CLOCK        CONCAT(RS485_UART_GPIO, RS485_UART_TX_PORT_LETTER)
#define RS485_UART_TX_PIN               CONCAT(GPIO_Pin_, RS485_UART_TX_PIN_NUMBER)
#define RS485_UART_TX_PINSOURCE         CONCAT(GPIO_PinSource,  RS485_UART_TX_PIN_NUMBER)
#define RS485_UART_RX_PORT              CONCAT(GPIO, RS485_UART_RX_PORT_LETTER)
#define RS485_UART_RX_GPIO_CLOCK        CONCAT(RS485_UART_GPIO, RS485_UART_RX_PORT_LETTER)
#define RS485_UART_RX_PIN               CONCAT(GPIO_Pin_, RS485_UART_RX_PIN_NUMBER)
#define RS485_UART_RX_PINSOURCE         CONCAT(GPIO_PinSource, RS485_UART_RX_PIN_NUMBER)

#define RS485_UART_TX_PORT              CONCAT(GPIO, RS485_UART_TX_PORT_LETTER)
#define RS485_UART_TX_GPIO_CLOCK        CONCAT(RS485_UART_GPIO, RS485_UART_TX_PORT_LETTER)
#define RS485_UART_TX_PIN               CONCAT(GPIO_Pin_, RS485_UART_TX_PIN_NUMBER)
#define RS485_UART_TX_PINSOURCE         CONCAT(GPIO_PinSource,  RS485_UART_TX_PIN_NUMBER)
#define RS485_UART_RX_PORT              CONCAT(GPIO, RS485_UART_RX_PORT_LETTER)
#define RS485_UART_RX_GPIO_CLOCK        CONCAT(RS485_UART_GPIO, RS485_UART_RX_PORT_LETTER)
#define RS485_UART_RX_PIN               CONCAT(GPIO_Pin_, RS485_UART_RX_PIN_NUMBER)
#define RS485_UART_RX_PINSOURCE         CONCAT(GPIO_PinSource, RS485_UART_RX_PIN_NUMBER)

#if defined(STM32F4XX)
#define RS485_DIR_PERIPH_CLOCK_CMD      RCC_AHB1PeriphClockCmd
#define RS485_DIR_PERIPH                CONCAT(RCC_AHB1Periph_GPIO, RS485_DIR_PORT_LETTER)
#elif defined(STM32F10X)
#define RS485_DIR_PERIPH_CLOCK_CMD      RCC_APB2PeriphClockCmd
#define RS485_DIR_PERIPH                CONCAT(RCC_APB2Periph_GPIO, RS485_DIR_PORT_LETTER)
#endif

#define RS485_DIR_PORT                  CONCAT(GPIO, RS485_DIR_PORT_LETTER)
#define RS485_DIR_PIN                   CONCAT(GPIO_Pin_, RS485_DIR_PIN_NUMBER)

#define RS485_DIR_RECEIVE               RESET
#define RS485_DIR_SEND                  SET

static volatile uint8_t                 rs485_txbuf[RS485_TXBUFLEN];     // tx ringbuffer
static volatile uint_fast16_t           rs485_txsize = 0;                // tx size
static volatile uint8_t                 rs485_rxbuf[RS485_RXBUFLEN];     // rx ringbuffer
static volatile uint_fast16_t           rs485_rxsize = 0;                // rx size

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_init ()
 *
 * Parameters:
 *  baudrate        baudrate
 *  wordlength      USART_WordLength_8b or USART_WordLength_9b;
 *  parity          USART_Parity_No, USART_Parity_Even, or USART_Parity_Odd
 *  stopbits        USART_StopBits_1, USART_StopBits_0_5, USART_StopBits_2, or USART_StopBits_1_5
 *
 *  Examples:
 *  8N1             wordlength = USART_WordLength_8b, parity = USART_Parity_No, stopbits = USART_StopBits_1
 *  8E1             wordlength = USART_WordLength_9b (!), parity = USART_Parity_Even, stopbits = USART_StopBits_1
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rs485_init (uint32_t baudrate, uint16_t wordlength, uint16_t parity, uint16_t stopbits)
{
    // Direction init:
    GPIO_InitTypeDef    gpio_direction;
    GPIO_InitTypeDef    gpio;
    USART_InitTypeDef   uart;
    NVIC_InitTypeDef    nvic;

    GPIO_StructInit (&gpio_direction);
    RS485_DIR_PERIPH_CLOCK_CMD (RS485_DIR_PERIPH, ENABLE);     // enable clock for RS485 direction Port

    gpio_direction.GPIO_Pin   = RS485_DIR_PIN;
    gpio_direction.GPIO_Speed = GPIO_Speed_2MHz;

#if defined (STM32F10X)
    gpio_direction.GPIO_Mode  = GPIO_Mode_Out_PP;
#elif defined (STM32F4XX)
    gpio_direction.GPIO_Mode  = GPIO_Mode_OUT;
    gpio_direction.GPIO_OType = GPIO_OType_PP;
    gpio_direction.GPIO_PuPd  = GPIO_PuPd_NOPULL;
#endif

    GPIO_Init(RS485_DIR_PORT, &gpio_direction);
    GPIO_WriteBit(RS485_DIR_PORT, RS485_DIR_PIN, RS485_DIR_RECEIVE);

    // RS485_UART init:

    GPIO_StructInit (&gpio);
    USART_StructInit (&uart);

    RS485_UART_GPIO_CLOCK_CMD (RS485_UART_TX_GPIO_CLOCK, ENABLE);
    RS485_UART_GPIO_CLOCK_CMD (RS485_UART_RX_GPIO_CLOCK, ENABLE);

    RS485_UART_USART_CLOCK_CMD (RS485_UART_USART_CLOCK, ENABLE);

    // connect RS485_UART functions with IO-Pins
#if defined (STM32F4XX)
    GPIO_PinAFConfig (RS485_UART_TX_PORT, RS485_UART_TX_PINSOURCE, RS485_UART_GPIO_AF_UART);      // TX
    GPIO_PinAFConfig (RS485_UART_RX_PORT, RS485_UART_RX_PINSOURCE, RS485_UART_GPIO_AF_UART);      // RX

    // RS485_UART as alternate function with PushPull
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;                                             // fm: perhaps better: GPIO_PuPd_NOPULL

    gpio.GPIO_Pin = RS485_UART_TX_PIN;
    GPIO_Init(RS485_UART_TX_PORT, &gpio);

    gpio.GPIO_Pin = RS485_UART_RX_PIN;
    GPIO_Init(RS485_UART_RX_PORT, &gpio);

#elif defined (STM32F10X)

#if RS485_UART_ALTERNATE == 0
    // GPIO_PinRemapConfig(RS485_UART_GPIO_REMAP, DISABLE);                     // fm: that's the default?!?
#else
    GPIO_PinRemapConfig(RS485_UART_GPIO_REMAP, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
#endif

    /* TX Pin */
    gpio.GPIO_Pin   = RS485_UART_TX_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_UART_TX_PORT, &gpio);

    /* RX Pin */
    gpio.GPIO_Pin   = RS485_UART_RX_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_UART_RX_PORT, &gpio);

#endif

    USART_OverSampling8Cmd(RS485_UART_NAME, ENABLE);

    // 8 bit no parity, no RTS/CTS
    uart.USART_BaudRate             = baudrate;
    uart.USART_WordLength           = wordlength;
    uart.USART_StopBits             = stopbits;
    uart.USART_Parity               = parity;
    uart.USART_HardwareFlowControl  = USART_HardwareFlowControl_None;
    uart.USART_Mode                 = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(RS485_UART_NAME, &uart);

    // RS485_UART enable
    USART_Cmd(RS485_UART_NAME, ENABLE);

    // RX-Interrupt enable
    USART_ITConfig(RS485_UART_NAME, USART_IT_RXNE, ENABLE);

    // enable RS485_UART Interrupt-Vector
    nvic.NVIC_IRQChannel                    = RS485_UART_IRQ_CHANNEL;
    nvic.NVIC_IRQChannelPreemptionPriority  = 0;
    nvic.NVIC_IRQChannelSubPriority         = 0;
    nvic.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init (&nvic);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_putc()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rs485_putc (uint_fast8_t ch)
{
    static uint_fast16_t    rs485_txstop  = 0;                      // tail

    while (rs485_txsize >= RS485_TXBUFLEN)                          // buffer full?
    {                                                               // yes
        ;                                                           // wait
    }

    rs485_txbuf[rs485_txstop++] = ch;                               // store character

    if (rs485_txstop >= RS485_TXBUFLEN)                             // at end of ringbuffer?
    {                                                               // yes
        rs485_txstop = 0;                                           // reset to beginning
    }

    __disable_irq();
    rs485_txsize++;                                                 // increment used size
    __enable_irq();

    USART_ITConfig(RS485_UART_NAME, USART_IT_TXE, ENABLE);          // enable TXE interrupt
}

void
rs485_puts (char * s)
{
    uint8_t ch;

    while ((ch = (uint8_t) *s) != '\0')
    {
        rs485_putc (ch);
        s++;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_getc()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
rs485_getc (void)
{
    static uint_fast16_t    rs485_rxstart = 0;                  // head
    uint_fast8_t            ch;

    while (rs485_rxsize == 0)                                   // rx buffer empty?
    {                                                           // yes, wait
        ;
    }

    ch = rs485_rxbuf[rs485_rxstart++];                          // get character from ringbuffer

    if (rs485_rxstart == RS485_RXBUFLEN)                        // at end of rx buffer?
    {                                                           // yes
        rs485_rxstart = 0;                                      // reset to beginning
    }

    __disable_irq();
    rs485_rxsize--;                                             // decrement size
    __enable_irq();

    return ch;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_poll()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
rs485_poll (uint_fast8_t * chp)
{
    static uint_fast16_t    rs485_rxstart = 0;                  // head
    uint_fast8_t            ch;

    if (rs485_rxsize == 0)                                      // rx buffer empty?
    {                                                           // yes, return 0
        return 0;
    }

    ch = rs485_rxbuf[rs485_rxstart++];                          // get character from ringbuffer

    if (rs485_rxstart == RS485_RXBUFLEN)                        // at end of rx buffer?
    {                                                           // yes
        rs485_rxstart = 0;                                      // reset to beginning
    }

    __disable_irq();
    rs485_rxsize--;                                             // decrement size
    __enable_irq();

    *chp = ch;
    return 1;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_flush()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
rs485_flush ()
{
    while (rs485_txsize > 0)                                    // tx buffer empty?
    {
        ;                                                       // no, wait
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_read()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
rs485_read (char * p, uint_fast16_t size)
{
    uint_fast16_t i;

    for (i = 0; i < size; i++)
    {
        *p++ = rs485_getc ();
    }
    return size;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * rs485_write()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
rs485_write (char * p, uint_fast16_t size)
{
    uint_fast16_t i;

    for (i = 0; i < size; i++)
    {
        rs485_putc (*p++);
    }

    return size;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * RS485_UART_IRQ_HANDLER
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void RS485_UART_IRQ_HANDLER (void)
{
    static uint_fast16_t    rs485_rxstop  = 0;                                  // tail
    uint16_t                value;
    uint_fast8_t            ch;

    if (USART_GetITStatus (RS485_UART_NAME, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit (RS485_UART_NAME, USART_IT_RXNE);
        value = USART_ReceiveData (RS485_UART_NAME);

        ch = value & 0xFF;

        if (rs485_rxsize < RS485_RXBUFLEN)                                      // buffer full?
        {                                                                       // no
            rs485_rxbuf[rs485_rxstop++] = ch;                                   // store character

            if (rs485_rxstop >= RS485_RXBUFLEN)                                 // at end of ringbuffer?
            {                                                                   // yes
                rs485_rxstop = 0;                                               // reset to beginning
            }

            rs485_rxsize++;                                                     // increment used size
        }
    }

    if (USART_GetITStatus (RS485_UART_NAME, USART_IT_TXE) != RESET)
    {
        static uint_fast16_t    rs485_txstart = 0;                              // head

        USART_ClearITPendingBit (RS485_UART_NAME, USART_IT_TXE);
        GPIO_WriteBit(RS485_DIR_PORT, RS485_DIR_PIN, RS485_DIR_SEND);           // set direction to SEND

        if (rs485_txsize > 0)                                                   // tx buffer empty?
        {                                                                       // no
            ch = rs485_txbuf[rs485_txstart++];                                  // get character to send, increment offset

            if (rs485_txstart == RS485_TXBUFLEN)                                // at end of tx buffer?
            {                                                                   // yes
                rs485_txstart = 0;                                              // reset to beginning
            }

            rs485_txsize--;                                                     // decrement size

            USART_SendData(RS485_UART_NAME, ch);
        }
        else
        {                                                                       // tx buffer empty
            USART_ITConfig(RS485_UART_NAME, USART_IT_TXE, DISABLE);             // disable TXE interrupt
            USART_ITConfig(RS485_UART_NAME, USART_IT_TC, ENABLE);               // enable transfer complete interrupt
        }
    }
    else if (USART_GetITStatus (RS485_UART_NAME, USART_IT_TC) != RESET)         // transfer complete interrupt:
    {
        USART_ClearITPendingBit (RS485_UART_NAME, USART_IT_TC);
        USART_ITConfig(RS485_UART_NAME, USART_IT_TC, DISABLE);                  // disable transfer complete interrupt
        GPIO_WriteBit(RS485_DIR_PORT, RS485_DIR_PIN, RS485_DIR_RECEIVE);        // set direction to RECEIVE
    }
}
