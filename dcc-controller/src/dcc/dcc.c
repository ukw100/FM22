/*------------------------------------------------------------------------------------------------------------------------
 * dcc.c - DCC encoder
 *------------------------------------------------------------------------------------------------------------------------
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
 *------------------------------------------------------------------------------------------------------------------------
 *  DCC protocol - see NEM 671:
 *
 *  Mindestabstand zwischen 2 DCC-Datenpaketen: tD >= 5msec
 *  Wiederholungszeit für DCC-Datenpakete:      tR <= 30msec
 *
 *  Short Address Loco Decoder (7 bit, 1-127, 0 forbidden):
 *      preamble             address
 *      1111111111111111  0  0AAA-AAAA  0
 *      1111111111111111  0  0000-0000  0                                   example: address = 0 (broadcast)
 *      1111111111111111  0  0000-0001  0                                   example: address = 1
 *      1111111111111111  0  0111-1111  0                                   example: address = 127
 *
 *  Long Address (14 bit):
 *
 *      11AA-AAAA  AAAA-AAAA
 *      1100-0000  0000-0000                                                example: address = 0 (broadcast)
 *      1100-0000  1000-0000                                                example: address = 1
 *      1111-1110  1111-1110                                                example: address = 16126
 *
 *  Basic Accessory Decoder (see RCN-213 2.1):
 *
 *      1 0 A  A  -  A  A  A  A     1  A    A   A  -  D  A  A  R            D=1: activate output, D=0: deactivate output, R=0: Weiche auf Abzweig, R=1 Weiche gerade
 *          A7 A6    A5 A4 A3 A2       /A10 /A9 /A8      A1 A0              A8, A9, A10 inverted!
 *
 *  Accessory Decoder Emergency Off:
 *      1011-1111 1000-0110
 *
 *  Operations 000.....: Decoder and Consist Control (see RCN-212)
 *
 *      preamble             databyte1      data-byte2    checksum
 *      1111111111111111  0  0000-0000  0   0000-0000  0  0000-0000  1      reset of all decoders, prepare programming mode
 *      1111111111111111  0  1111-1111  0   0000-0000  0  1111-1111  1      "no operation" for all decoders ("Leerlauf")
 *
 *      command
 *      0000-0000                                                           reset decoder
 *      0000-0001                                                           hard reset of decoder, CV 29, 31, 32 will be reset to factory settings, CV 19 = 0x00
 *      0000-101D                                                           set bit 5 of CV 29, D=1: use long address, D=0: use short address
 *      0000-1111                                                           get ACK from decoder
 *
 *
 *  Operations 001.....: Advanced Operation instruction
 *
 *      001C-CCCC  DDDD-DDDD
 *      0011-1111  DDDD-DDDD                                                CCCCC = 11111: 126 speed step control, speed follows, D7 is direction, D6-D0 is speed
 *      0011-1110  DDDD-DDDD                                                CCCCC = 11110: restricted speed step instruction
 *      0011-1101  DDDD-DDDD                                                CCCCC = 11101: analog function group
 *      0010-0000  DDDD-DDDD                                                CCCCC = 00000: binary state control instruction long form
 *
 *  Operations 010.....: 14 speed steps, backward
 *
 *      010C-SSSS                                                           C = light (14 steps) or lowest speed bit (28 steps), SSSS = speed
 *      0101-0000                                                           example: speed 0 backward, light on
 *      0101-0001                                                           example: speed 1 backward, light on (1 == emergency stop!)
 *      0101-0010                                                           example: speed 2 backward, light on
 *      0100-1110                                                           example: speed 14 backward, light off
 *
 *  Operations 011.....: 14 speed steps, forward
 *
 *      011C-SSSS                                                           C = light, SSSS = speed
 *      0111-0000                                                           example: speed 0 forward, light on
 *      0111-0001                                                           example: speed 1 forward, light on (1 == emergency stop!)
 *      0111-0010                                                           example: speed 2 forward, light on
 *      0110-1110                                                           example: speed 14 forward, light off
 *
 *  Operations 100.....: Loco functions F0 - F4
 *
 *      data byte
 *      100....1                                                            F1
 *      100...1.                                                            F2
 *      100..1..                                                            F3
 *      100.1...                                                            F4
 *      1001....                                                            F0
 *
 *  Operations 101.....: Loco functions F5 - F12
 *
 *      1011...1                                                            F5
 *      1011..1.                                                            F6
 *      1011.1..                                                            F7
 *      10111...                                                            F8
 *
 *      1010...             1                                               F9
 *      1010..1.                                                            F10
 *      1010.1..                                                            F11
 *      10101...                                                            F12
 *
 *  Operations 110.....: Expanded Loco functions and Binary State Control
 *
 *      data byte    expansion
 *      1101-1110    .......1                                               F13
 *      1101-1110    ......1.                                               F14
 *      1101-1110    .....1..                                               F15
 *      1101-1110    ....1...                                               F16
 *      1101-1110    ...1....                                               F17
 *      1101-1110    ..1.....                                               F18
 *      1101-1110    .1......                                               F19
 *      1101-1110    1.......                                               F20
 *
 *      1101-1111    .......1                                               F21
 *      1101-1111    ......1.                                               F22
 *      1101-1111    .....1..                                               F23
 *      1101-1111    ....1...                                               F24
 *      1101-1111    ...1....                                               F25
 *      1101-1111    ..1.....                                               F26
 *      1101-1111    .1......                                               F27
 *      1101-1111    1.......                                               F28
 *
 *  Operations 111.....:
 *      CV control
 *
 *  CV-Types (see RCN-225):
 *  M - Mandatory
 *  R - Recommended
 *  O - Optional
 *
 *  CV Nr.      CV Verwendung                                               Typ Bereich     Kommentar
 *    1         Basis-Adresse                                               M   1 … 127     Standardwert 3
 *    2         Minimale Geschwindigkeit                                    R
 *    3         Faktor Beschleunigung                                       R
 *    4         Faktor Bremsen                                              R
 *    5         Maximale Geschwindigkeit                                    O
 *    6         Mittlere Geschwindigkeit                                    O
 *    7         Decoder Versionsnummer                                      M               Nur Lesen; vom Hersteller vergeben
 *    8         Hersteller Identifikation                                   M   Ja          Nur Lesen; durch NMRA vorgegeben
 *    9         Periode Pulsweite                                           O
 *   10         Herstellerspezifisches CV                                   O
 *   11         Maximalzeit ohne Datenempfang                               R
 *   12         Zulässige Betriebsarten                                     O   Ja          Bitweise
 *   13         Funktionsstatus Alternative Betriebsart F1-F8               O   Ja          Bitweise
 *   14         Funktionsstatus Alternative Betriebsart F0 + F9-F12         O   Ja          Bitweise
 *   15 – 16    Decoder Sperre                                              O   Ja
 *   17 – 18    Erweiterte Adresse                                          O   Ja          Standardwert 1000
 *   19         Adresse für Mehrfachtraktion                                O   Ja
 *   20         Reserviert für lange Mehrfachtraktionsadresse               -               Reserviert NMRA
 *   21         Funktionssteuerung in einer Mehrfachtraktion F1-F8          O   Ja          Bitweise
 *   22         Funktionssteuerung in einer Mehrfachtraktion F0, F9-F12     O   Ja          Bitweise
 *   23         Justieren Beschleunigung                                    O   Ja
 *   24         Justieren Bremsen                                           O   Ja
 *   25         Tabelle Geschwindigkeit                                     O   Ja
 *   26         n/a                                                         -               Reserviert NMRA
 *   27         Konfiguration für automatisches Anhalten                    O   Ja          Bitweise
 *   28         Konfiguration der Zwei-Wege Kommunikation (RailCom)         O   Ja          Bitweise
 *   29         Decoder Konfiguration                                       M   Ja          Bitweise
 *   30         Fehlerinformation                                           O   Ja          Dynamischer Wert; 0 = kein Fehler
 *   31         Zeiger-Wert für erweiterten Bereich, höherwertiges Byte     O   Ja          Zeiger für CV 257 – 512
 *   32         Zeiger-Wert für erweiterten Bereich, niederwertiges Byte    O   Ja          Zeiger für CV 257 – 512
 *   33 – 46    Funktionszuordnung für F0-F12                               O   Ja
 *   47 – 64    Herstellerspezifische CVs                                   O
 *   65         Herstellerspezifisches CV                                   O
 *   66         Trimmen Vorwärts                                            O
 *   67 – 94    Tabelle Geschwindigkeit                                     O
 *   95         Trimmen Rückwärts                                           O
 *   96         Methode zur Funktionszuordnung                              O   Ja          Siehe auch [RCN227]
 *   97 – 104   Herstellerspezifische CVs   O
 *  105 – 106   Anwenderkennung 1 & 2                                       O               Anwenderspezifisch
 *  107 – 108   16 Bit Herstellerkennung                                    (M)             Nur bei CV 8 = 238, sonst herstellerspezifisch
 *  109 – 111   Herstellerspezifische CVs                                   O
 *  112 – 256   Herstellerspezifische CVs                                   O
 *  257 – 512   Erweiterter CV-Bereich                                      O               Adressiert durch CV 31 und 32
 *  513 – 879   n/a                                                         -               Reserviert für Zubehördecoder
 *  880 – 896   n/a                                                         -               Reserviert NMRA / RailCommunity
 *  897 – 1024  SUSI-CVs                                                    O   Ja          Reserviert für SUSI
 *
 *  Examples CV:
 *
 *  CV 1   Basis-Adresse
 *  --------------------
 *  Format: 0AAA-AAAA
 *
 *  CV 7   Hersteller Versionsnummer
 *  --------------------------------
 *  Version of decoder, see also: RCN-226
 *
 *  CV 8   Hersteller Identifikation
 *  --------------------------------
 *  If value is 0xEE, extended ID in CV 107-108
 *
 *  CV 17 – CV 18   Erweiterte Adresse
 *  ----------------------------------
 *  Used if bit 5 set in CV 29
 *  Format: CV17      CV18
 *          11AA-AAAA AAAA-AAAA
 *  Invalid:1100-0000 0000-0000
 *
 *  CV 28   Konfiguration der Zwei-Wege Kommunikation (RailCom)
 *  -----------------------------------------------------------
 *  Bit 0 = Freigabe Kanal 1 Adress-Broadcast
 *  Bit 1 = Freigabe Kanal 2 Daten und Acknowledge
 *  Bit 2 = Freigabe Kanal 1 automatisch abschalten
 *  Bit 3 = Reserviert für künftige Nutzung
 *  Bit 4 = Freigabe Programmieradresse 0003 (lange Adresse 3)
 *  Bit 5 = Reserviert für künftige Nutzung
 *  Bit 6 = Freigabe Hochstrom-RailCom
 *  Bit 7 = Freigabe RailComPlus
 *  Bit = 0:  gesperrt
 *  Bit = 1:  freigegeben
 *
 *  CV 29   Decoder Konfiguration
 *  -----------------------------
 *  Bit 0 = Richtung des Fahrzeugs: 0 = normal, 1 = inverse Fahrtrichtung
 *  Bit 1 = Übertragung der Lichtfunktion F0: 0 = F0 wird in Bit 4 des Basis-
 *          Geschwindigkeitsbefehl übertragen (14 Fahrstufenmodus),
 *          1 = F0 wird in Bit 4 des Funktionsbefehls F0-F4 übertragen.
 *          Das Bit 4 im Basis-Geschwindigkeitsbefehl wird als 5. Fahrstufenbit
 *          genutzt (28 Fahrstufenmodus).
 *  Bit 2 = Freigabe Analogbetrieb: 0 = Nur Digitalbetrieb, 1 = auch analoger Betrieb möglich.
 *  Bit 3 = Freigabe Zwei-Wege Kommunikation (RailCom): 0 = gesperrt, 1 = freigegeben
 *  Bit 4 = Tabelle Geschwindigkeit:
 *          0 = Geschwindigkeit wird aus den CVs 2, 5 + 6 berechnet.
 *          1 = Geschwindigkeit wird über Tabelle aus CV 67 – 94 berechnet.
 *  Bit 5 = Lokadresse:
 *          0 = Basis-Adresse aus CV 1
 *          1 = Erweiterte Adresse aus CV 17 und CV 18.
 *
 *  CV 30   Fehlerinformation
 *  -------------------------
 *  0 = kein Fehler
 *
 *
 *  Examples for commands:
 *      addressbyte = loco_address & 0x7F;
 *      databyte    = 0b01000000 | (speed & 0x0F) | (direction << 5);
 *
 *  See also:
 *      https://www.railcommunity.org/index.php?option=com_content&view=article&id=49&Itemid=61
 *      RCN-210, RCN-211, RCN-212 and other
 *      NEM-670, NEM-671, NEM-672
 *------------------------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "board-led.h"
#include "listener.h"
#include "dcc.h"
#include "booster.h"
#include "adc1-dma.h"
#include "delay.h"
#include "s88.h"
#include "io.h"
#include "rc-detector.h"

#define SHORTCUT_DEFAULT                1000                                // default shortcut value

#define PREAMBLE_LEN_RAILCOM            16                                  // length of PREAMBLE if railcom mode
#define PREAMBLE_LEN_NO_RAILCOM         12                                  // length of PREAMBLE if no railcom mode, 12 should be enough
#define PREAMBLE_LEN_PROGRAMMING        20                                  // length of PREAMBLE if programming mode

#define DELAY_RCN_211_5                 5                                   // 5 msec pause between packets if same address

static volatile uint_fast8_t            dcc_mode                = RAILCOM_MODE;
static volatile uint_fast8_t            preamble_len            = PREAMBLE_LEN_RAILCOM;
static volatile uint_fast8_t            do_switch_booster_on    = 0;

uint_fast16_t                           dcc_shortcut_value      = SHORTCUT_DEFAULT;
volatile uint32_t                       millis                  = 0;                // volatile: used in ISR
volatile uint_fast8_t                   debuglevel              = 0;                // volatile: used in ISR

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * timer definitions:
 *
 * Period = ((TIM_PRESCALER + 1) * (TIM_PERIOD + 1)) / TIM_CLK
 *
 * STM32F407VE with period of 58 usec:
 *      TIM_PERIOD      =   4 - 1   =   3
 *      TIM_PRESCALER   = 609 - 1   = 608
 *      TIM_CLK         = 84000000
 *      period          = (4 * 609) / 84000000 = 0.000058 = 29 usec
 *
 * STM32F401CC with period of 25 usec:
 *      TIM_PERIOD      =   4 - 1   =   3
 *      TIM_PRESCALER   = 609 - 1   = 608
 *      TIM_CLK         = 84000000
 *      period          = (4 * 609) / 84000000 = 0.000058 = 29 usec
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */

#if defined (STM32F407VE)                                                   // STM32F407VE Black Board @168MHz
#define TIM_CLK                 84000000L                                   // APB1 clock: 84 MHz
#define TIM_PERIOD              3
#define TIM_PRESCALER           608
#elif defined (STM32F401CC)                                                 // STM32F401CC Black Pill @84MHz
#define TIM_CLK                 84000000L                                   // APB1 clock: 84 MHz
#define TIM_PERIOD              3
#define TIM_PRESCALER           608
#else
#error STM32 unknown
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize timer2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
timer2_init (void)
{
    TIM_TimeBaseInitTypeDef     tim;
    NVIC_InitTypeDef            nvic;

    TIM_TimeBaseStructInit (&tim);
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2, ENABLE);

    tim.TIM_ClockDivision   = TIM_CKD_DIV1;
    tim.TIM_CounterMode     = TIM_CounterMode_Up;
    tim.TIM_Period          = TIM_PERIOD;
    tim.TIM_Prescaler       = TIM_PRESCALER;
    TIM_TimeBaseInit (TIM2, &tim);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    nvic.NVIC_IRQChannel                    = TIM2_IRQn;
    nvic.NVIC_IRQChannelCmd                 = ENABLE;
    nvic.NVIC_IRQChannelPreemptionPriority  = 0x0F;
    nvic.NVIC_IRQChannelSubPriority         = 0x0F;
    NVIC_Init (&nvic);

    TIM_Cmd(TIM2, ENABLE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Timer3 IRQ handler - called every 29 usec
 *
 * Parameter        Name    Min     Max
 * --------------------------------------
 * Cutout Start     TCS      26µs    32µs
 * Cutout End       TCE     454µs   488µs
 * Start Kanal 1    TTS1     80µs
 * End Kanal 1      TTC1            177µs
 * Start Kanal 2    TTS2    193µs
 * End Kanal 2      TTC2            454µs
 *
 * Die Uebertragung eines Byte auf dem RC2-Channel dauert bei 250000 Bd 40 usec.
 * Daher kann die Konstante CUTOUT_END_CHANNEL_1 durchaus 232-177 = 55 usec
 * hoeher ausfallen, ohne Gefahr zu laufen, dass beim Lesen des RC1 nach 203 usec
 * bereits ein Byte aus RC2 gelesen wird.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define CUTOUT_LAST_HALF_BIT    0                               // t = -29 usec: last half bit!
#define CUTOUT_POSITIVE_EDGE    1                               // t =   0 usec: positive edge
#define CUTOUT_START            2                               // t =  29 usec: cutout start (TCS)
#define CUTOUT_START_CHANNEL_1  4                               // t =  87 usec: start channel 1 (TTS1)
#define CUTOUT_FLUSH_CHANNEL_1  5                               // t = 116 usec: flush channel 1
#define CUTOUT_END_CHANNEL_1    9                               // t = 232 usec: end channel 1 (TTC1)
#define CUTOUT_END_CHANNEL_2    17                              // t = 464 usec: end channel 2 (TTC2)
#define CUTOUT_END              18                              // t = 493 usec: cutout end (TCE)

#define DCC_BUFLEN              16                              // 9 should be enough, but...

#define STATE_READY             0
#define STATE_PREAMBLE          1
#define STATE_DATA              2
#define STATE_CUTOUT            3

static volatile uint_fast8_t    dcc_buflen = 0;
static volatile uint8_t         dcc_buf[DCC_BUFLEN];
static uint_fast16_t            dcc_loco_idx = 0xFFFF;

extern void TIM2_IRQHandler (void);                             // keep compiler happy

void
TIM2_IRQHandler (void)
{
    static uint8_t              buf[DCC_BUFLEN];
    static uint32_t             millis_cnt;
    static uint_fast8_t         state           = STATE_READY;
    static uint_fast8_t         buflen          = 0;
    static uint_fast8_t         flags           = 0;
    static uint_fast8_t         byteidx         = 0;
    static uint_fast8_t         bitidx          = 0;
    static uint_fast8_t         positive        = 1;
    static uint_fast8_t         longpulse       = 0;
    static uint_fast8_t         divider         = 0;
    static uint_fast16_t        loco_idx        = 0;
    uint_fast8_t                bitval;

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    millis_cnt++;

    if (millis_cnt == 35)                                       // 35 * 29 = 1015 usec
    {
        millis++;
        millis_cnt = 0;
    }

    if (state == STATE_CUTOUT)
    {
        switch (bitidx)
        {
            case CUTOUT_LAST_HALF_BIT:                          // 0 - 1 = -1: t = -29 usec: last half bit!
            {
                break;
            }

            case CUTOUT_POSITIVE_EDGE:                          // 1 - 1 = 0: t = 0 usec
            {
                if (booster_is_on)
                {
                    booster_positive ();                        // last edge before cutout, ESU decoders need it
                }
                positive = 0;
                break;
            }

            case CUTOUT_START:                                  // 2 - 1 = 1: t = 29 usec
            {
                if (booster_is_on)
                {
                    booster_cutout ();                          // cutout
                }
                break;
            }

            case CUTOUT_START_CHANNEL_1:                        // 4 - 1 = 3: t = 87 usec
            {
                if (booster_is_on)
                {
                    rc_detector_enable ();                      // enable rc detector
                }
                break;
            }

            case CUTOUT_FLUSH_CHANNEL_1:                        // 5 - 1 = 4: t = 116 usec
            {
                if (booster_is_on)
                {
                    rc_detector_uart_input_flush ();            // flush input of RC detector
                }
                break;
            }

            case CUTOUT_END_CHANNEL_1:                          // 9 - 1 = 8: t = 232 usec
            {
                if (booster_is_on)
                {
                    rc_detector_read_rc1 ();
                }
                break;
            }

            case CUTOUT_END_CHANNEL_2:                          // 17 - 1 = 16: t = 464 usec: end rc channel 2
            {
                if (booster_is_on)
                {
                    rc_detector_disable ();                     // disable rc detector
                    rc_detector_read_rc2 (loco_idx);
                }
                break;
            }

            case CUTOUT_END:                                    // 18 - 1 = 17: t = 493 usec
            {
                state = STATE_READY;                            // set new state and fall through below
                rc_detector_uart_input_flush ();                // flush input of RC detector
                divider = 0;
                break;
            }
        }
    }

    if (state == STATE_CUTOUT)
    {
        bitidx++;
    }
    else                                                        // fall through with state == STATE_READY
    {
        if (divider)
        {
            divider = 0;
        }
        else
        {
            divider = 1;

            if (longpulse)
            {
                longpulse = 0;
            }
            else
            {
                switch (state)
                {
                    case STATE_READY:
                    {
                        if (do_switch_booster_on)                                   // flag set?
                        {                                                           // switch booster on when we are beginning a new frame
                            booster_on ();
                            do_switch_booster_on = 0;
                        }

                        if (dcc_buflen)                                             // data in buffer?
                        {
                            buflen          = dcc_buflen & 0x0F;                    // copy buflen
                            memcpy (buf, (uint8_t *) dcc_buf, buflen);              // copy buffer
                            flags           = dcc_buflen & 0xF0;
                            loco_idx        = dcc_loco_idx;                         // loco index
                            dcc_buflen      = 0;                                    // reset buflen
#if defined STM32F407VE
                            board_led3_off ();                                      // indicate busy on LED3: led off
#endif
                            listener_set_continue ();                               // input buffer empty
                        }
                        else
                        {                                                           // no data
#if defined STM32F407VE
                            board_led3_on ();                                       // indicate pgm or idle frame on LED3: led on
#endif
                            listener_set_continue ();                               // input buffer empty

                            if (dcc_mode == PROGRAMMING_MODE)                       // generate reset frame
                            {
                                buf[0]      = 0x00;                                 // 0000-0000
                                buf[1]      = 0x00;                                 // 0000-0000
                                buf[2]      = 0x00;                                 // xor value
                                buflen      = 3;
                            }
                            else                                                    // generate idle frame
                            {
                                buf[0]      = 0xFF;
                                buf[1]      = 0x00;
                                buf[2]      = 0xFF;
                                buflen      = 3;
                            }

                            flags           = 0;                                    // reset flags
                            loco_idx        = 0xFFFF;                               // reset loco index
                        }

                        state           = STATE_PREAMBLE;                           // start with preamble now!
                        bitidx          = 0;                                        // reset bit index
                        positive        = 1;                                        // start with positive value in preamble
                    }

                    // fall through
                    case STATE_PREAMBLE:
                    {
                        if (bitidx < preamble_len)                                  // send 11...?
                        {
                            if (positive)
                            {
                                booster_positive ();
                                positive = 0;
                            }
                            else
                            {
                                booster_negative ();
                                positive = 1;
                                bitidx++;
                            }
                        }
                        else
                        {                                                           // last preamble bit sent, generate 0
                            if (positive)                                           // 1st positive
                            {
                                booster_positive ();
                                positive = 0;
                            }
                            else
                            {                                                       // 2nd negative
                                booster_negative ();
                                state       = STATE_DATA;                           // set state to STATE_DATA
                                byteidx     = 0;                                    // reset byte index
                                bitidx      = 0;                                    // reset bit index
                                positive    = 1;                                    // start with positive value in data
                            }
                            longpulse = 1;
                        }
                        break;
                    }

                    case STATE_DATA:
                    {
                        if (bitidx < 8)                                             // send 8 bits
                        {
                            bitval = buf[byteidx] & 1 << (7 - bitidx);              // get bit value first, not later

                            if (positive)                                           // 1st positive value
                            {
                                booster_positive ();
                                positive = 0;
                            }
                            else
                            {                                                       // 2nd negative value
                                booster_negative ();
                                positive = 1;
                                bitidx++;                                           // increment bit index
                            }

                            if (! bitval)                                           // bit value is 0
                            {
                                longpulse = 1;                                      // generate long pulse
                            }
                        }
                        else                                                        // all bits send, send delimiter 0 or end bit 1?
                        {
                            if (byteidx != buflen - 1)                              // last byte?
                            {                                                       // no, send bit "0"
                                longpulse = 1;                                      // by generating longpulse
                            }

                            if (positive)                                           // 1st positive value?
                            {
                                booster_positive ();
                                positive = 0;
                            }
                            else
                            {                                                       // 2nd negative value
                                booster_negative ();
                                positive = 1;                                       // start next with positive value
                                bitidx = 0;                                         // reset bit index
                                byteidx++;                                          // increment byte index
                            }

                            if (byteidx == buflen)                                  // last byte?
                            {                                                       // yes, set state to STATE_CUTOUT
                                if (dcc_mode == RAILCOM_MODE && flags == 0)
                                {
                                    preamble_len = PREAMBLE_LEN_RAILCOM;
                                    state = STATE_CUTOUT;
                                }
                                else
                                {
                                    preamble_len = PREAMBLE_LEN_NO_RAILCOM;
                                    state = STATE_READY;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (booster_is_on && state != STATE_CUTOUT)
            {
                s88_read ();
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * send_packet () - send DCC packet
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
send_packet (uint_fast16_t loco_idx, uint_fast16_t addr, const uint8_t * buf, uint_fast8_t len)
{
    uint_fast8_t    idx;
    uint_fast8_t    val;
    uint_fast8_t    xor;
    uint_fast8_t    n = 0;

    if (loco_idx < RC_DETECTOR_MAX_LOCOS && rc_loco_addrs[loco_idx] != addr)
    {
        rc_loco_addrs[loco_idx] = addr;

        if (n_rc_loco_addrs < loco_idx + 1)
        {
            n_rc_loco_addrs = loco_idx + 1;
        }
    }

    while (dcc_buflen != 0)
    {
        ;
    }

    if (addr != 0xFFFF)
    {
        if (addr < 128)
        {
            val = addr & 0x7F;                          // 0AAA-AAAA
            xor = val;
            dcc_buf[n++] = val;
        }
        else
        {
            val = 0xC0 | (addr >> 8);                   // 11AA-AAAA
            xor = val;
            dcc_buf[n++] = val;

            val = addr & 0xFF;                          // AAAA-AAAA
            xor ^= val;
            dcc_buf[n++] = val;
        }
    }
    else
    {
        xor = 0x00;
    }

    for (idx = 0; idx < len; idx++)
    {
        val = *buf++;
        xor ^= val;
        dcc_buf[n++] = val;
    }

    dcc_buf[n++]    = xor;
    dcc_loco_idx    = loco_idx;
    dcc_buflen      = n;
    listener_set_stop ();
}

static void
idle_packet (void)
{
    while (dcc_buflen != 0)
    {
        ;
    }

    dcc_buf[0]      = 0xFF;
    dcc_buf[1]      = 0x00;
    dcc_buf[2]      = 0xFF;
    dcc_loco_idx    = 0xFFFF;
    dcc_buflen      = 3;
}

// Zentralen-Eigenschaftenkennung: RCN-211 5.3
// 1100-0011 1111-1111 DDDD-DDDD DDDD-DDDD      Befehle für Fahrzeugdecoder
// 1100-0011 1111-1110 DDDD-DDDD DDDD-DDDD      Befehle für Zubehördecoder und Broadcastbefehle
// 1100-0011 1111-1101 DDDD-DDDD DDDD-DDDD      RailCom

// DDDD-DDDD lower byte
#define CENTRAL_PROP_RAILCOM_217    0x01        // RailCom RCN-217
#define CENTRAL_PROP_RAILCOM_218    0x02        // DCC-A RCN-218
#define CENTRAL_PROP_RAILCOM_NOP    0x04        // NOP fuer Schaltdecoder
#define CENTRAL_PROP_RAILCOM_POM    0x08        // POM (read)
#define CENTRAL_PROP_RAILCOM_XPOM   0x10        // XPOM (read)
#define CENTRAL_PROP_RAILCOM_RESL1  0x20        // reserved
#define CENTRAL_PROP_RAILCOM_RESL2  0x40        // reserved
#define CENTRAL_PROP_RAILCOM_RESL3  0x80        // reserved

// DDDD-DDDD upper byte
#define CENTRAL_PROP_RAILCOM_DYN1   0x01        // Behälterstände (ID 7 DYN, Sub-Index 8 bis 19)
#define CENTRAL_PROP_RAILCOM_DYN2   0x02        // Betriebsparameter (ID 7 DYN, Sub-Indices 0, 1, 7, 20, 22 und 26)
#define CENTRAL_PROP_RAILCOM_TEST   0x04        // Testbetrieb: Spannung am Gleis (ID 7 DYN, Sub-Index 46)
#define CENTRAL_PROP_RAILCOM_RESH1  0x08        // reserved
#define CENTRAL_PROP_RAILCOM_RESH2  0x10        // reserved
#define CENTRAL_PROP_RAILCOM_RESH3  0x20        // reserved
#define CENTRAL_PROP_RAILCOM_RESH4  0x40        // reserved
#define CENTRAL_PROP_RAILCOM_PLUS   0x80        // Unterstützt RailComPlus

#if 0
static void
central_properties (void)
{
    dcc_buf[0]      = 0xC3;
    dcc_buf[1]      = 0xFD;
    dcc_buf[2]      = 0x00;
    dcc_buf[3]      = CENTRAL_PROP_RAILCOM_XPOM | CENTRAL_PROP_RAILCOM_POM | CENTRAL_PROP_RAILCOM_217;
    dcc_loco_idx    = 0xFFFF;
    dcc_buflen      = 4;
}
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_get_mode () - get DCC mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_get_mode (void)
{
    uint_fast8_t mode = dcc_mode;
    return mode;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_set_mode () - set DCC mode
 *
 * Parameters:
 *      RAILCOM_MODE        - set RailCom mode
 *      NO_RAILCOM_MODE     - set RailCom mode
 *      PROGRAMMING_MODE    - set Programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_set_mode (uint_fast8_t newmode)
{
    switch (newmode)
    {
        case NO_RAILCOM_MODE:               preamble_len = PREAMBLE_LEN_NO_RAILCOM;     break;
        case PROGRAMMING_MODE:              preamble_len = PREAMBLE_LEN_PROGRAMMING;    break;
        default: newmode = RAILCOM_MODE;    preamble_len = PREAMBLE_LEN_RAILCOM;        break;
    }
    dcc_mode = newmode;
}

#define PGM_RESET_PACKETS       25                                      // number of reset packets, should be min. 25
#define PGM_REPEAT_PACKETS       5                                      // number of repeat packets, should be 5
#define PGM_WRITE_DURATION      100                                     // 100msec
#define PGM_READ_DURATION       80                                      // 80msec - normally 50msec should be enough
#define PGM_MIN_CYCLE_CNT       40                                      // 40 cycles (== 4 msec) needed to detect a positive answer

#define PGM_METHOD_ADC          1                                       // method: ADC (CT pin of booster)
#define PGM_METHOD_RCL          2                                       // method: RCL (RCL message)
#define PGM_METHOD_ACK          3                                       // method: ACK (pin ACK_PGM pin of ACK detector), default

#define PGM_METHOD              PGM_METHOD_ACK

#if PGM_METHOD == PGM_METHOD_ACK

#define PGM_PERIPH_CLOCK_CMD        RCC_AHB1PeriphClockCmd
#define PGM_PERIPH                  RCC_AHB1Periph_GPIOB
#define PGM_PORT                    GPIOB
#define PGM_ACK_PIN                 GPIO_Pin_10     // PB10

static uint_fast8_t
dcc_pgm_cmd (uint8_t * buf, uint_fast8_t buflen, uint_fast8_t is_write_command)
{
    uint_fast8_t    ack = 0;
    uint32_t        next_millis;
    uint32_t        ack_cnt = 0;
    uint32_t        no_ack_cnt = 0;
    uint_fast8_t    r;

    for (r = 0; r < PGM_REPEAT_PACKETS; r++)
    {
        send_packet (0xFFFF, 0xFFFF, buf, buflen);
    }

    if (is_write_command)
    {
        next_millis = millis + PGM_WRITE_DURATION;          // 100msec
    }
    else
    {
        next_millis = millis + PGM_READ_DURATION;           // 80msec
    }

    while (millis < next_millis)
    {
        uint8_t * rcl_msg = listener_read_rcl ();

        if (rcl_msg && rcl_msg[0] == MSG_RCL_PGM_ACK)       // FM22 RC-Detector sends ACK via RS485
        {
            ack_cnt = PGM_MIN_CYCLE_CNT;
            break;
        }

        if (GPIO_GET_BIT(PGM_PORT, PGM_ACK_PIN) == 0)
        {
            ack_cnt++;
            no_ack_cnt = 0;
        }
        else
        {
            no_ack_cnt++;

            if (ack_cnt >= PGM_MIN_CYCLE_CNT && no_ack_cnt >= 5)    // got ack, then nevermore
            {                                                       // we can break here
                break;
            }
        }
        delay_usec (100);
    }

    char b[128];
    sprintf (b, "ack_cnt: %lu, no_ack_cnt: %lu, rest: %lu", ack_cnt, no_ack_cnt, next_millis - millis);
    listener_send_debug_msg (b);

    if (ack_cnt >= PGM_MIN_CYCLE_CNT)                               // minimum is 4 msec
    {
        ack = 1;
    }

    return ack;
}

#elif PGM_METHOD == PGM_METHOD_ADC

#define SHOW_VALUES             0                                   // 1: show values, only for debug

static uint_fast8_t
dcc_pgm_cmd (uint8_t * buf, uint_fast8_t buflen, uint_fast8_t is_write_command)
{
    uint16_t        dcc_pgm_min_lower_value = 9999;
    uint16_t        dcc_pgm_max_lower_value = 0;
    uint16_t        dcc_pgm_min_upper_value = 9999;
    uint16_t        dcc_pgm_max_upper_value = 0;
    uint16_t        dcc_pgm_min_cnt         = 9999;
    uint16_t        dcc_pgm_max_cnt         = 0;
    uint16_t        dcc_pgm_limit;
    uint16_t        value;
    uint_fast16_t   m;
    uint_fast8_t    r;
    uint_fast8_t    ack = 0;
    uint32_t        sum = 0;
    uint_fast16_t   duration;
    uint_fast8_t    cntl = 0;
    uint_fast8_t    cnth = 0;

    for (m = 0; m < 8; m++)
    {
        value = adc1_dma_buffer[0];
        delay_usec (100);
        sum += value;
    }

    value = sum / 8;

    dcc_pgm_limit = 2 * value;

#if SHOW_VALUES == 1
    static uint16_t values[1000];

    for (m = 0; m < 1000; m++)
    {
        values[m] = 0;
    }
#endif

    for (r = 0; r < PGM_REPEAT_PACKETS; r++)
    {
        send_packet (0xFFFF, 0xFFFF, buf, buflen);
    }

    if (is_write_command)
    {
        duration = 10 * PGM_WRITE_DURATION;                                 // 100msec
    }
    else
    {
        duration = 10 * PGM_READ_DURATION;                                  // 80msec
    }

    for (m = 0; m < duration; m++)
    {
        value = adc1_dma_buffer[0];

#if SHOW_VALUES == 1
if (dcc_buflen == 0) { dcc_reset (); }

        if (m < 1000)
        {
            values[m] = value;
        }
#endif

        if (value > dcc_pgm_limit)
        {
            if (value < dcc_pgm_min_upper_value)
            {
                dcc_pgm_min_upper_value = value;
            }

            if (value > dcc_pgm_max_upper_value)
            {
                dcc_pgm_max_upper_value = value;
            }
        }
        else
        {
            if (value < dcc_pgm_min_lower_value)
            {
                dcc_pgm_min_lower_value = value;
            }

            if (value > dcc_pgm_max_lower_value)
            {
                dcc_pgm_max_lower_value = value;
            }
        }

        if (value > dcc_pgm_limit)
        {
            cnth++;
        }
        else
        {
            cntl++;
        }

        delay_usec (100);
    }

    if (cnth > dcc_pgm_max_cnt)
    {
        dcc_pgm_max_cnt = cnth;
    }

    if (cnth >= PGM_MIN_CYCLE_CNT && cnth < dcc_pgm_min_cnt)        // >= 4 msec
    {
        dcc_pgm_min_cnt = cnth;
    }

    char b[128];
    sprintf (b, "lim=%u cntl=%u cnth=%u minp=%u maxp=%u maxu=%u maxl=%u", dcc_pgm_limit, cntl, cnth, dcc_pgm_min_cnt, dcc_pgm_max_cnt, dcc_pgm_max_upper_value, dcc_pgm_max_lower_value);
    listener_send_debug_msg (b);

#if SHOW_VALUES == 1
    for (m = 0; m < 1000; m++)
    {
        sprintf (b, "%u", values[m]);
        listener_send_debug_msg (b);
    }
#endif

    if (cnth >= PGM_MIN_CYCLE_CNT)                                  // >= 4 msec
    {
        ack = 1;
    }
    else
    {
        ack = 0;
    }

    return ack;
}

#elif PGM_METHOD == PGM_METHOD_RCL

static uint_fast8_t
dcc_pgm_cmd (uint8_t * buf, uint_fast8_t buflen, uint_fast8_t is_write_command)
{
    uint_fast8_t    ack = 0;
    uint32_t        next_millis;
    uint_fast8_t    r;

    for (r = 0; r < PGM_REPEAT_PACKETS; r++)
    {
        send_packet (0xFFFF, 0xFFFF, buf, buflen);
    }

    if (is_write_command)
    {
        next_millis = millis + PGM_WRITE_DURATION;          // 100msec
    }
    else
    {
        next_millis = millis + PGM_READ_DURATION;           // 80msec
    }

    while (millis < next_millis)
    {
        uint8_t * rcl_msg = listener_read_rcl ();

        if (rcl_msg && rcl_msg[0] == MSG_RCL_PGM_ACK)       // important: read all messages, may be more than one!
        {
            ack = 1;                                        // don't break here!
            // char b[128];
            // sprintf (b, "ack: %d", ack);
            // listener_send_debug_msg (b);
        }
    }

    return ack;
}

#endif

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pgm_read_cv () - read CV in programming mode
 *
 * Step 1: compare 8 bits of CV:
 *
 *      0111-10VV VVVV-VVVV 111K-DBBB - see also RCN 214 2
 *      K = 0   bit check (used here)
 *      K = 1   bit write
 *      D =     Data bit
 *      BBB =   Bit position
 *
 * Step 2: check complete byte of CV:
 *
 *      0111-KKVV VVVV-VVVV DDDD-DDDD - see also RCN 214 2
 *      KK = 00 reserved
 *      KK = 01 byte check (used here)
 *      KK = 10 bit manipulation
 *      KK = 11 byte write
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_pgm_read_cv (uint_fast8_t * cv_valuep, uint_fast16_t cv)
{
    uint8_t         buf[3];
    uint_fast8_t    idx;
    uint_fast8_t    cv_value = 0;
    uint_fast8_t    oldmode = dcc_get_mode ();
    uint_fast8_t    rtc = 0;

    dcc_set_mode (PROGRAMMING_MODE);

    for (idx = 0; idx < PGM_RESET_PACKETS; idx++)
    {
        dcc_reset ();
    }

    cv--;

    for (idx = 0; idx < 8; idx++)
    {
        buf[0] = 0x78 | (cv >> 8);
        buf[1] = cv & 0xFF;
        buf[2] = 0xE8 | idx;                                        // check if bit is 1

        if (dcc_pgm_cmd (buf, 3, 0))
        {
            cv_value |= (1 << idx);                                 // if check successful, set bit
        }
    }

    // printf ("cv = %d, value = %d (0x%02X)\r\n", cv + 1, cv_value, cv_value);
    char b[128];
    sprintf (b, "cv = %d, value = %d (0x%02X)", cv + 1, cv_value, cv_value);
    listener_send_debug_msg (b);

    // check complete byte:

    buf[0] = 0x74 | (cv >> 8);
    buf[1] = cv & 0xFF;
    buf[2] = cv_value;                                              // check complete value

    if (dcc_pgm_cmd (buf, 3, 0))
    {
        listener_send_debug_msg ("check successful\r\n");
        *cv_valuep = cv_value;
        rtc = 1;
    }
    else
    {
        listener_send_debug_msg ("check failed\r\n");
        *cv_valuep = 0;
        rtc = 0;
    }

    delay_msec (50);
    dcc_set_mode (oldmode);

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pgm_write_cv () - write CV in programming mode
 *
 *      0111-KKVV VVVV-VVVV DDDD-DDDD - see also RCN 214 2
 *      KK = 00 reserved
 *      KK = 01 byte check
 *      KK = 10 bit manipulation
 *      KK = 11 byte write (used here)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_pgm_write_cv (uint_fast16_t cv, uint_fast8_t cv_value)
{
    uint8_t         buf[3];
    uint_fast8_t    idx;
    uint_fast8_t    oldmode = dcc_get_mode ();
    uint_fast8_t    rtc = 0;

    dcc_set_mode (PROGRAMMING_MODE);

    for (idx = 0; idx < PGM_RESET_PACKETS; idx++)
    {
        dcc_reset ();
    }

    cv--;

    // write complete byte:

    buf[0] = 0x7C | (cv >> 8);
    buf[1] = cv & 0xFF;
    buf[2] = cv_value;                                              // write complete value

    if (dcc_pgm_cmd (buf, 3, 1))
    {
        // check complete byte:

        buf[0] = 0x74 | (cv >> 8);
        buf[1] = cv & 0xFF;
        buf[2] = cv_value;                                              // check complete value

        if (dcc_pgm_cmd (buf, 3, 0))
        {
            rtc = 1;
        }
        else
        {
            rtc = 0;
        }
    }
    else
    {
        rtc = 0;
    }

    delay_msec (50);
    dcc_set_mode (oldmode);

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pgm_write_cv_bit () - write bit into CV in programming mode
 *
 *  Step 1: write bit
 *      0111-10VV VVVV-VVVV 111K-DBBB - see also RCN 214 2
 *      K = 0   bit check
 *      K = 1   bit write (used here)
 *      D =     Data bit
 *      BBB =   Bit position
 *
 *  Step 1: check bit
 *      0111-10VV VVVV-VVVV 111K-DBBB - see also RCN 214 2
 *      K = 0   bit check (used here)
 *      K = 1   bit write
 *      D =     Data bit
 *      BBB =   Bit position
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_pgm_write_cv_bit (uint_fast16_t cv, uint_fast8_t bitpos, uint_fast8_t bitvalue)
{
    uint8_t         buf[3];
    uint_fast8_t    idx;
    uint_fast8_t    oldmode = dcc_get_mode ();
    uint_fast8_t    rtc = 0;

    dcc_set_mode (PROGRAMMING_MODE);

    for (idx = 0; idx < PGM_RESET_PACKETS; idx++)
    {
        dcc_reset ();
    }

    cv--;

    // write bit:

    buf[0] = 0x78 | (cv >> 8);
    buf[1] = cv & 0xFF;
    buf[2] = 0xF0 | (bitvalue << 3) | bitpos;

    if (dcc_pgm_cmd (buf, 3, 1))
    {
        // check bit:

        buf[0] = 0x78 | (cv >> 8);
        buf[1] = cv & 0xFF;
        buf[2] = 0xE0 | (bitvalue << 3) | bitpos;

        if (dcc_pgm_cmd (buf, 3, 0))
        {
            rtc = 1;
        }
        else
        {
            rtc = 0;
        }
    }
    else
    {
        rtc = 0;
    }

    delay_msec (50);
    dcc_set_mode (oldmode);

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pgm_write_address () - write loco address in programming mode
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_pgm_write_address (uint_fast16_t addr)
{
    uint_fast8_t    rtc1;
    uint_fast8_t    rtc2;
    uint_fast8_t    rtc3;
    uint_fast8_t    rtc  = 0;

    if (addr > 127)
    {
        uint_fast8_t    addr_high   = (addr >> 8) | 0xC0;
        uint_fast8_t    addr_low    = addr & 0xFF;

        rtc1 = dcc_pgm_write_cv (17, addr_high);        // don't check return value here!
        rtc2 = dcc_pgm_write_cv (18, addr_low);
        rtc3 = dcc_pgm_write_cv_bit (29, 5, 1);

        if (rtc1 && rtc2 && rtc3)
        {
            rtc = 1;
        }
    }
    else
    {
        rtc1 = dcc_pgm_write_cv (1, addr);              // don't check return value here!
        rtc2 = dcc_pgm_write_cv_bit (29, 5, 0);

        if (rtc1 && rtc2)
        {
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_loco_28 () - send loco command, 28 speed steps
 *
 * Group:
 *  01xx-xxxx   Basic speed and direction command (RCN-212 2.1)
 * Operation:
 *  01RG-GGGG   Speed and direction (RCN-212 2.2.1)
 *
 *  short addr: 0 <= addr  <= 127
 *  long  addr: 0 <= addr  <= 10239
 *  speed:      0 or 1: stop
 *  speed:      2 or 3: estop (emergency stop)
 *  speed:      4 .. 31: 28 speed steps
 *  direction:  DCC_DIRECTION_BWD or DCC_DIRECTION_FWD
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_loco_28 (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed)
{
    uint8_t buf[1];

    buf[0] = 0x40 | (direction << 5) | ((speed & 0x01) << 4) | ((speed & 0x1F) >> 1);   // 0 1 D S0 S4 S3 S2 S1
    send_packet (loco_idx, addr, buf, 1);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_loco_function () - send loco functions
 *
 * Group:
 *  10xx-xxxx  function groups (RCN-212 2.1)
 *
 * Operations:
 *  100x-xxxx               function control F0-F4
 *  1010-xxxx               function control F9-F12
 *  1011-xxxx               function control F5-F8
 *
 * Group:
 *  110x-xxxx  extended function groups (RCN-212 2.1)
 *
 * Extended operations:
 *  1101-1110  xxxx-xxxx    function control F13-F20
 *  1101-1111  xxxx-xxxx    function control F21-F28
 *
 * Not implemented:
 *  1101-1000  xxxx-xxxx    function control F29-F36
 *  1101-1001  xxxx-xxxx    function control F37-F44
 *  1101-1010  xxxx-xxxx    function control F45-F52
 *  1101-1011  xxxx-xxxx    function control F53-F60
 *  1101-1100  xxxx-xxxx    function control F61-F68
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_loco_function (uint_fast16_t loco_idx, uint_fast16_t addr, uint32_t fmask, uint_fast8_t range)
{
    uint8_t buf[2];

    switch (range)
    {
        case DCC_F00_F04_RANGE:
            buf[0] = 0x80 | ((fmask & 0x01) << 4) | ((fmask >> 1) & 0x0F);              // 100F-FFFF
            send_packet (loco_idx, addr, buf, 1);
            break;
        case DCC_F05_F08_RANGE:
            buf[0] = 0xB0 | ((fmask >> 5) & 0x0F);                                      // 1011-FFFF
            send_packet (loco_idx, addr, buf, 1);
            break;
        case DCC_F09_F12_RANGE:
            buf[0] = 0xA0 | ((fmask >> 9) & 0x0F);                                      // 1010-FFFF
            send_packet (loco_idx, addr, buf, 1);
            break;
        case DCC_F13_F20_RANGE:
            buf[0] = 0xDE;                                                              // 1101-1110
            buf[1] = (fmask >> 13) & 0xFF;                                              // FFFF-FFFF
            send_packet (loco_idx, addr, buf, 2);
            break;
        case DCC_F21_F28_RANGE:
            buf[0] = 0xDF;                                                              // 1101-1111
            buf[1] = (fmask >> 21) & 0xFF;                                              // FFFF-FFFF
            send_packet (loco_idx, addr, buf, 2);
            break;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_loco () - send loco speed, direction and function bits (F0-F31)
 *
 * Group:
 *  001x-xxxx   Extended operations (RCN-212 2.1)
 *
 * Operations:
 *  0011-1111   128 speed steps, no function information
 *  0011-1100   speed, direction and functions
 *
 *  short addr: 0 <= addr  <= 127
 *  long  addr: 0 <= addr  <= 10239
 *  speed:      0: stop
 *  speed:      1: estop (emergency stop)
 *  speed:      2 .. 127: 126 speed steps
 *  direction:  DCC_DIRECTION_BWD or DCC_DIRECTION_FWD
 *  fmask:      F00-F31
 *  len:        number of function bytes to send, len = 0 .. 4
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_loco (uint_fast16_t loco_idx, uint_fast16_t addr, uint_fast8_t direction, uint_fast8_t speed, uint32_t fmask, uint_fast8_t n)
{
    uint8_t buf[6];
    uint_fast8_t idx;

    if (n == 0)                                                                         // no function information
    {
        buf[0] = 0x3F;                                                                  // 0011-1111
        buf[1] = (direction << 7) | (speed & 0x7F);                                     // DSSS-SSSS
        send_packet (loco_idx, addr, buf, 2);
    }
    else                                                                                // n function bytes
    {
        if (n > 4)
        {
            n = 4;
        }

        buf[0] = 0x3C;                                                                  // 0011-1100
        buf[1] = (direction << 7) | (speed & 0x7F);                                     // DSSS-SSSS

        for (idx = 0; idx < n; idx++)
        {
            buf[idx + 2] = fmask & 0xFF;                                                // FFFFFFFF
            fmask >>= 8;
        }

        send_packet (loco_idx, addr, buf, n + 2);
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_reset () - send DCC reset packet - reset all locos and stop (broadcast)
 *
 *  Group:
 *  0000-xxxx               Decoder Control (RCN-212 2.1)
 *
 *  Operation:
 *  0000-0000   0000-0000   Reset
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_reset (void)
{
    while (dcc_buflen != 0)
    {
        ;
    }

    dcc_buf[0]      = 0x00;                             // 0000-0000
    dcc_buf[1]      = 0x00;                             // 0000-0000
    dcc_buf[2]      = 0x00;                             // xor value
    dcc_loco_idx    = 0xFFFF;                           // reset dcc_loco_idx
    dcc_buflen      = 3;                                // at least set dcc_buflen
    listener_set_stop ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_stop () - send DCC stop packet - stop all locos (broadcast)
 *
 * Group:
 *  01xx-xxxx   Basic speed and direction command (RCN-212 2.1)
 * Operation:
 *  01RG-GGGG   Speed and direction (RCN-212 2.2.1)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_stop (void)
{
    uint_fast16_t addr       = 0;
    uint_fast8_t speed       = 0;
    uint_fast8_t direction   = DCC_DIRECTION_FWD;

    dcc_loco_28 (0xFFFF, addr, direction, speed);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_estop () - send DCC emergency stop packet - stop all locos (broadcast)
 *
 * Group:
 *  001x-xxxx   Extended operations (RCN-212 2.1)
 *
 * Operation:
 *  0011-1111   128 speed steps, no function information
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_estop (void)
{
    uint_fast16_t addr       = 0;
    uint_fast8_t speed       = 1;
    uint_fast8_t direction   = DCC_DIRECTION_FWD;

    dcc_loco (0xFFFF, addr, direction, speed, 0, 0);
    // dcc_loco_28 (addr, direction, speed);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_reset_decoder () - send DCC reset packet - reset loco and stop
 *
 * Group:
 *  0000-xxxx               Decoder Control (RCN-212 2.1)
 *
 * Operation:
 *  0000-0000               Reset
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_reset_decoder (uint_fast16_t addr)
{
    uint8_t     buf[1] = { 0x00 };                                              // 0000-0000
    send_packet (0xFFFF, addr, buf, 1);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_hard_reset_decoder () - send DCC reset packet - hard reset of loco
 *
 * Group:
 *  0000-xxxx               Decoder Control (RCN-212 2.1)
 *
 * Operation:               Hard reset of decoder
 *  0000-0001               CV 29, 31, 32 will be reset to factory settings, CV 19 = 0x00
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_hard_reset_decoder (uint_fast16_t addr)
{
    uint8_t     buf[1] = { 0x01 };                                              // 0000-0001
    send_packet (0xFFFF, addr, buf, 1);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_use_long_address () - set bit 5 of CV 29
 *
 * Group:
 *  0000-xxxx               Decoder Control (RCN-212 2.1)
 *
 * Operation:               Set long address mode (RCN-212 2.5.4)
 *  0000-101D               Set bit 5 of CV 29, D=1: use long address, D=0: use short address
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
dcc_pom_use_long_address (uint_fast16_t addr, uint_fast8_t do_use_long_address)
{
    uint8_t     buf[1];

    buf[0] = 0x0A;                                                              // 0000-1010

    if (do_use_long_address)
    {
        buf[0] |= 0x01;                                                         // 0000-1011
    }

    send_packet (0xFFFF, addr, buf, 1);
    send_packet (0xFFFF, addr, buf, 1);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_set_long_address () - set long address (14 bit) in CV 17/18 and set bit 5 of CV 29
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Short CV access (RCN-214 3.x)
 *  1111-KKKK   DDDD-DDDD   DDDD-DDDD
 *
 * Operation: Write CV17 and CV18, set bit 5 in CV29 (RCN-214 3.2)
 *  1111-0100   DDDD-DDDD   DDDD-DDDD
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
dcc_pom_set_long_address (uint_fast16_t addr, uint16_t new_addr)
{
    uint8_t     buf[3];

    buf[0] = 0xF4;
    buf[1] = 0xC0 | (new_addr >> 8);
    buf[2] = new_addr & 0xFF;

    send_packet (0xFFFF, addr, buf, 3);                 // send packet twice
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5
    send_packet (0xFFFF, addr, buf, 3);
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_write_address () - write loco address in POM mode
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_pom_write_address (uint_fast16_t oldaddr, uint_fast16_t newaddr)
{
    if (newaddr > 127)
    {
        dcc_pom_set_long_address (oldaddr, newaddr);
    }
    else
    {
        if (oldaddr <= 127)                             // old: short addr, new: short addr
        {
            dcc_pom_write_cv (oldaddr, 1, newaddr);     // we are safe, no need to reset bit 5 in CV29
        }
        else                                            // old: long, new: short
        {
            dcc_pom_write_cv (oldaddr, 1, newaddr);     // many decoders reset also bit 5 in CV29 if writing to CV1!
            delay_msec (100);
            dcc_pom_use_long_address (oldaddr, 0);      // don't know here if loco still uses old long or new short addr!
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_read_cv () - read value of CV
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Long CV access to bytes (RCN-214 2.2)
 *  1110-KKVV   VVVV-VVVV   DDDD-DDDD
 *
 * Operation: read byte (POM)
 *  1110-01VV   VVVV-VVVV   0000-0000
 *
 * Answer: RailCom channel2 with ID 0000
 *
 * The associated response datagram (ID0) does not have to be sent in the cutout immediatly after the
 * command packet but must be sent within 0.5 seconds of the command packet. A Command Station
 * must therefore ensure that the decoder is addressed again and no other programming command is
 * sent to the same address (repeated the command is permitted).
 * When the read operation is completed, the decoder sends the result to the associated read command.
 * If the decoder does not return the data within 0.5s then the read is considered failed.
 *
 * See also:
 * https://www.nmra.org/sites/default/files/s-9.3.2_2012_12_10.pdf (old)
 * https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.3.2_bi-directional_communication.pdf (new)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_pom_read_cv (uint_fast8_t * valuep, uint_fast16_t addr, uint16_t cv)
{
    uint8_t         buf[3];
    uint_fast16_t   idx;
    uint_fast16_t   cnt = 0;
    uint32_t        stop_millis;
    uint8_t *       rcl_msg;
    uint_fast8_t    rtc = 0;

    if (booster_is_on)
    {
        cv--;                                               // begin with 0

        // delay_msec (DELAY_RCN_211_5);                    // RCN 211.5
        rc2_cv_value_valid = 0;                             // flush old CV values

        for (idx = 0; idx < 4; idx++)                       // most decoders need a repetition of read command (e.g. Viessmann: 4)
        {
            buf[0] = 0xE4 | cv >> 8;                        // 1110-01VV
            buf[1] = cv & 0xFF;                             // VVVV-VVVV
            buf[2] = 0x00;                                  // 0000-00000

            send_packet (0xFFFF, addr, buf, 3);
            delay_msec (DELAY_RCN_211_5);                   // RCN 211.5

            if (rc2_cv_value_valid)
            {
                *valuep = rc2_cv_value;
                rc2_cv_value_valid = 0;                     // flush CV value!
                rtc = 1;
                break;
            }
        }

        stop_millis = millis + 500;

        while (millis < stop_millis)                        // Lenz decoder sends value 5 times, read all answers
        {                                                   // Tams sends answers very late, count until 25
            dcc_get_ack(addr);
            delay_msec (DELAY_RCN_211_5);                   // RCN 211.5

            if (rc2_cv_value_valid)
            {
                *valuep = rc2_cv_value;
                rc2_cv_value_valid = 0;                     // flush CV value!
                rtc = 1;
            }
            else if ((rcl_msg = listener_read_rcl ()) != (uint8_t *) 0 && rcl_msg[0] == MSG_RCL_POM_CV)
            {
                *valuep = rcl_msg[1];
                rtc = 1;
            }
            else
            {
                if (rtc == 1)                              // got answers previously, break here if 5x no answer
                {
                    cnt++;
                }

                if (cnt >= 5)
                {
                    break;
                }
            }
            idx++;
        }
    }
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_write_cv () - write value of CV
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Long CV access to bytes (RCN-214 2.2)
 *  1110-KKVV   VVVV-VVVV   DDDD-DDDD
 *
 * Operation: write byte (POM)
 *  1110-11VV   VVVV-VVVV   DDDD-DDDD
 *
 * When writing, the decoder should respond as follows: With ACK, as long as the new value is not
 * written with the new value, if the new value is written successfully, first with ACK and then NACK
 * if the CV is not supported or another one Value if the written value can not be accepted. Instead of
 * an ACK, another non-command message can be used.
 * If the decoder does not return a value within 0.5s then the write is considered failed. For read only
 * CV‘s the shall decoder return the current value of the CV.
 *
 * See also:
 * https://www.nmra.org/sites/default/files/s-9.3.2_2012_12_10.pdf (old)
 * https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.3.2_bi-directional_communication.pdf (new)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_pom_write_cv (uint_fast16_t addr, uint16_t cv, uint_fast8_t value)
{
    uint8_t         buf[3];
    uint_fast8_t    idx;

    cv--;                                               // begin with 0

    buf[0] = 0xEC | cv >> 8;                            // 1110-11VV
    buf[1] = cv & 0xFF;                                 // VVVV-VVVV
    buf[2] = value;                                     // DDDD-DDDD

    send_packet (0xFFFF, addr, buf, 3);                 // send 2 identical packets
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5
    send_packet (0xFFFF, addr, buf, 3);                 // repeat last packet
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5

    for (idx = 10; idx < 20; idx++)                     // then wait some time
    {
        dcc_get_ack(addr);
        delay_msec (DELAY_RCN_211_5);                   // RCN 211.5
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_pom_write_cv_bit () - write bit of CV
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Long CV access to bytes (RCN-214 2.2)
 *  1110-KKVV   VVVV-VVVV   DDDD-DDDD
 *
 * Operation: write bit (POM)
 *  1110-10VV   VVVV-VVVV   1111-DBBB
 *
 * The answers are the same as with "Write byte".
 *
 * See also:
 * https://www.nmra.org/sites/default/files/s-9.3.2_2012_12_10.pdf (old)
 * https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.3.2_bi-directional_communication.pdf (new)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_pom_write_cv_bit (uint_fast16_t addr, uint16_t cv, uint_fast8_t bitpos, uint_fast8_t value)
{
    uint8_t     buf[3];

    cv--;                                               // begin with 0

    buf[0] = 0xE8 | cv >> 8;                            // 1110-10VV
    buf[1] = cv & 0xFF;                                 // VVVV-VVVV
    buf[2] = 0xF0 | (value << 3) | bitpos;              // 1111-DBBB (value: 0 or 1, bitpos: 0-7)

    send_packet (0xFFFF, addr, buf, 3);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_xpom_read_cv () - read up to n * 4 values of CVs
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Long CV access to bytes (RCN-214 2.2)
 *  1110-KKSS   VVVV-VVVV   VVVV-VVVV   VVVV-VVVV
 *
 * Operation: read byte per XPOM (RCN-217 6.7.1)
 *  1110-01SS   VVVV-VVVV   VVVV-VVVV   VVVV-VVVV
 *
 * 1st VVVV-VVVV similar to value of CV 31
 * 2nd VVVV-VVVV similar to value of CV 32
 * 3rd VVVV-VVVV 8-bit value CV
 *
 * parameters:
 *   n may be 1..4 for 4, 8, 12, or 16 cv values
 *   cv must be a multiple of 4 beginning with 0
 *------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
dcc_xpom_read_cv (uint8_t * valuep, uint_fast8_t n, uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv)
{
    static uint_fast8_t seq;
    uint8_t             buf[4];
    uint_fast16_t       cnt;
    uint32_t            stop_millis;
    uint8_t *           p;
    uint_fast8_t        rtc = 0;

    if (booster_is_on)
    {
        uint_fast8_t    got[4] = { 0, 0, 0, 0 };

        // central_properties ();                           // doesn't matter

        for (seq = 0; seq < n; seq++)
        {
            rc2_cv_values_valid[seq] = 0;                  // flush old CV values

            buf[0] = 0xE4 | seq;                           // 1110-01ss (Sequence 0..3)
            buf[1] = cv31;
            buf[2] = cv32;
            buf[3] = cv + 4 * seq;

            send_packet (0xFFFF, addr, buf, 4);
            idle_packet ();
//            dcc_get_ack(addr);                              // idle_packet() does not work, use dcc_get_ack()
        }

        stop_millis = millis + 30;                          // wait time: 30 msec

        while (millis < stop_millis)
        {
            dcc_get_ack(addr);

            cnt = 0;

            for (seq = 0; seq < n; seq++)
            {
                if (got[seq])
                {
                    cnt++;
                }
                else if (rc2_cv_values_valid[seq])
                {
                    p = valuep + 4 * seq;
                    *p++ = rc2_cv_values[seq][0];
                    *p++ = rc2_cv_values[seq][1];
                    *p++ = rc2_cv_values[seq][2];
                    *p   = rc2_cv_values[seq][3];
                    got[seq] = 1;
                    cnt++;
                    // char b[128];
                    // sprintf (b, "x: got %d %ld", seq, stop_millis - millis);
                    // listener_send_debug_msg (b);
                }
            }

            if (cnt == n)
            {
                rtc = 1;
                break;
            }
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_xpom_write_cv () - write up to four values of CVs
 *
 * Group: CV access (RCN-212 2.1)
 *  111x-xxxx
 *
 * Sub-Group: Long CV access to bytes (RCN-214 4.x)
 *  1110-KKSS   VVVV-VVVV   VVVV-VVVV   VVVV-VVVV
 *
 * Operation: write bytes per XPOM (RCN-217 6.7.2)
 *  1110-11SS   VVVV-VVVV   VVVV-VVVV   VVVV-VVVV   {DDDD-DDDD {DDDD-DDDD {DDDD-DDDD {DDDD-DDDD}}}}
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_xpom_write_cv (uint_fast16_t addr, uint_fast8_t cv31, uint_fast8_t cv32, uint_fast8_t cv, uint8_t * ptr, uint_fast8_t len)
{
    uint8_t         buf[8];
    uint_fast8_t    i;

    buf[0] = 0xEC | 0x00;                               // 1110-1100 (Sequence 0)
    buf[1] = cv31;
    buf[2] = cv32;
    buf[3] = cv;

    for (i = 0; i < len; i++)
    {
        buf[i + 4] = ptr[i];
    }

    len += 4;

    send_packet (0xFFFF, addr, buf, len);
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5
    send_packet (0xFFFF, addr, buf, len);
    delay_msec (DELAY_RCN_211_5);                       // RCN 211.5
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_track_search () - search new decoder
 *
 * Group: Extended function control (RCN-212 2.1)
 *  110x-xxxx
 *
 * Sub-Group: Extended function control (RCN-212 2.3.5)
 *  1101-1101   DLLL-LLLL
 *
 * Operation: XF2 off (RCN-217 5.2.2)
 *  1101-1101   0000-0010
 *
 * This is a special application to identify a decoder on the layout. For this purpose, the decoder
 * is temporarily separated from the track power. After the decoder receives power again, it responds
 * for a maximum of 30 seconds to the command "XF2 off" to broadcast address 0 with its address
 * in Channel 2. During this time, the user must trigger this command via the control panel.
 *
 * The decoder should not respond to every command "XF2 off" to the broadcast address 0, so that
 * collisions in channel 2 may be reduced. These occur when another vehicle unintentionally had no
 * track power for a short time and therefore might respond. Over time since the decoder has tension
 * again, the desired vehicle should be identifiable.
 * Decoders should not respond to the command „XF2 off“ on broadcast address 0 unless the decoder
 * is powering up, this is to avoid collisions.
 *
 * See also:
 * https://www.nmra.org/sites/default/files/s-9.3.2_2012_12_10.pdf (old) 5.2.2
 * https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.3.2_bi-directional_communication.pdf (new) 5.2.2
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_track_search (void)
{
    uint8_t         buf[2];
    uint_fast16_t   addr = 0;                           // broadcast address

    buf[0] = 0xDD;                                      // 1101-1101 send XF2 off
    buf[1] = 0x02;                                      // 0000-0010 Bit 2 set

    send_packet (0xFFFF, addr, buf, 2);
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_get_ack () - get ACK from decoder
 *
 *  Group:
 *  0000-xxxx               Decoder Control (RCN-212 2.1)
 *
 *  Operation:
 *  0000-1111               get ACK from decoder (RCN-212 2.5.5)
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_get_ack (uint_fast16_t addr)
{
    uint8_t     buf[1] = { 0x0F };                      // 0000-1111
    send_packet (0xFFFF, addr, buf, 1);
}

typedef struct
{
    uint32_t    millis;
    uint16_t    addr;
    uint8_t     nswitch;
} DCC_SWITCH_RESET;

static DCC_SWITCH_RESET     dcc_switch_reset;

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_reset_last_active_switch () - reset last active switch
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_reset_last_active_switch (void)
{
    if (dcc_switch_reset.millis != 0 && millis >= dcc_switch_reset.millis)
    {
        dcc_base_switch_reset (dcc_switch_reset.addr, dcc_switch_reset.nswitch);
        dcc_switch_reset.millis = 0;
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_base_switch_set () - activate rail switch (base accessory decoder)
 *
 *  Group:
 *  10xx-xxxx               Accessory Decoder Control (RCN-213 2)
 *
 *  Operation:
 *  10AA-AAAA 1AAA-DAAR     Base Accessory Decoder Control (RCN-213 2.1)
 *
 *  Format
 *   1   0  A   A   -  A    A   A   A       1  A    A    A    -  D  A   A    R
 *   –   –  A7  A6  -  A5   A4  A3  A2      –  /A10 /A9  /A8  -  –  A1  A0   –
 *
 * here: D = 1
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_base_switch_set (uint_fast16_t addr, uint_fast8_t nswitch)
{
    uint_fast16_t swaddr;
    uint_fast16_t swstate;

    if (nswitch == DCC_SWITCH_STATE_BRANCH2)    // BRANCH-2 for 3way switch
    {                                           // use following address
        swaddr = addr + 4;                      // 1st address = xx00-0001 x111-x00x
        swstate = DCC_SWITCH_STATE_BRANCH;
    }
    else
    {
        swaddr = addr + 3;                      // 1st address = xx00-0001 x111-x00x
        swstate = nswitch;
    }

    if (dcc_switch_reset.millis)                // should never be, but be sure...
    {
        dcc_base_switch_reset (dcc_switch_reset.addr, dcc_switch_reset.nswitch);
        dcc_switch_reset.millis = 0;
    }

    while (dcc_buflen != 0)
    {
        ;
    }

    dcc_buf[0]      = 0x80 | ((swaddr >> 2) & 0x3F);                                                        // 10AA-AAAA
    dcc_buf[1]      = 0x80 | ((~swaddr >> 4) & 0x70) | 0x08 | ((swaddr & 0x03) << 1) | (swstate & 0x01);    // 1AAA-1AAR
    dcc_buf[2]      = dcc_buf[0] ^ dcc_buf[1];                                                              // xor value
    dcc_loco_idx    = 0xFFFF;                                                                               // reset dcc_loco_idx
    dcc_buflen      = 0x80 | 0x03;                                                                          // at least set dcc_buflen
    listener_set_stop ();

    dcc_switch_reset.millis     = millis + 200;
    dcc_switch_reset.addr       = addr;
    dcc_switch_reset.nswitch    = nswitch;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_base_switch_reset () - deactivate rail switch (base accessory decoder)
 *
 *  Group:
 *  10xx-xxxx               Accessory Decoder Control (RCN-213 2)
 *
 *  Operation:
 *  10AA-AAAA 1AAA-DAAR     Base Accessory Decoder Control (RCN-213 2.1)
 *
 *  Format
 *   1   0  A   A   -  A    A   A   A       1  A    A    A    -  D  A   A    R
 *   –   –  A7  A6  -  A5   A4  A3  A2      –  /A10 /A9  /A8  -  –  A1  A0   –
 *
 * here: D = 0
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_base_switch_reset (uint_fast16_t addr, uint_fast8_t nswitch)
{
    uint_fast16_t swaddr;
    uint_fast16_t swstate;

    if (nswitch == DCC_SWITCH_STATE_BRANCH2)    // BRANCH-2 for 3way switch
    {                                           // use following address
        swaddr = addr + 4;                      // 1st address = xx00-0001 x111-x00x
        swstate = DCC_SWITCH_STATE_BRANCH;
    }
    else
    {
        swaddr = addr + 3;                      // 1st address = xx00-0001 x111-x00x
        swstate = nswitch;
    }

    while (dcc_buflen != 0)
    {
        ;
    }

    dcc_buf[0]      = 0x80 | ((swaddr >> 2) & 0x3F);                                                // 10AA-AAAA
    dcc_buf[1]      = 0x80 | ((~swaddr >> 4) & 0x70) | ((swaddr & 0x03) << 1) | (swstate & 0x01);   // 1AAA-0AAR
    dcc_buf[2]      = dcc_buf[0] ^ dcc_buf[1];                                                      // xor value
    dcc_loco_idx    = 0xFFFF;                                                                       // reset dcc_loco_idx
    dcc_buflen      = 3;                                                                            // at least set dcc_buflen
    listener_set_stop ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_ext_accessory_set () - send 8 bit value to extended accessory decoder
 *
 *  Group:
 *  10xx-xxxx               Accessory Decoder Control (RCN-213 2)
 *
 *  Operation:
 *  10AA-AAAA 0AAA-0AA1 DDDD-DDDD   Extended Accessory Decoder Control (RCN-213 2.2)
 *
 *  Format
 *   1   0  A   A   -  A   A   A   A       0  A    A    A    -  0  A   A    1
 *   –   –  A7  A6  -  A5  A4  A3  A2      –  /A10 /A9  /A8  -  –  A1  A0   –
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_ext_accessory_set (uint_fast16_t addr, uint_fast8_t value)
{
    addr += 3;                                                                                      // 1st address = xx00-0001 x111-x00x

    while (dcc_buflen != 0)
    {
        ;
    }

    dcc_buf[0]      = 0x80 | ((addr >> 2) & 0x3F);                                                  // 10AA-AAAA
    dcc_buf[1]      = ((~addr >> 4) & 0x70) | ((addr & 0x03) << 1) | 0x01;                          // 0AAA-0AA1
    dcc_buf[2]      = value;
    dcc_buf[3]      = dcc_buf[0] ^ dcc_buf[1] ^ dcc_buf[2];                                         // xor value
    dcc_loco_idx    = 0xFFFF;                                                                       // reset dcc_loco_idx
    dcc_buflen      = 0x80 | 0x04;                                                                  // at least set dcc_buflen

    listener_set_stop ();
}


/*------------------------------------------------------------------------------------------------------------------------
 * dcc_booster_on () - switch booster on
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_booster_on (void)
{
    uint_fast8_t    seq;

    for (seq = 0; seq < 4; seq++)
    {
        rc2_cv_values_valid[seq] = 1;                                                       // block xpom rc2 detector
    }

    do_switch_booster_on = 1;                                                               // switch booster on in ISR
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_booster_off () - switch booster off
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_booster_off (void)
{
    booster_off ();
}

void
dcc_set_shortcut_value (uint_fast16_t value)
{
    dcc_shortcut_value = value;
}

/*------------------------------------------------------------------------------------------------------------------------
 * dcc_init () - init
 *------------------------------------------------------------------------------------------------------------------------
 */
void
dcc_init (void)
{
    booster_init ();
    timer2_init ();

#if PGM_METHOD == PGM_METHOD_ACK
    PGM_PERIPH_CLOCK_CMD (PGM_PERIPH, ENABLE);                                              // enable clock for PGM Port

    GPIO_InitTypeDef gpio;
    GPIO_StructInit (&gpio);
    GPIO_SET_MODE_IN_UP(gpio, PGM_ACK_PIN, GPIO_Speed_2MHz);
    GPIO_Init(PGM_PORT, &gpio);
#endif
}
