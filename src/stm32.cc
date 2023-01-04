/*------------------------------------------------------------------------------------------------------------------------
 * stm32.cc - STM32 flasher functions
 *------------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2022-2023 Frank Meyer - frank(at)uclock.de
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
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "base.h"
#include "http.h"
#include "gpio.h"
#include "millis.h"
#include "serial.h"
#include "stm32.h"

#define hex2toi(pp)                         htoi(pp, 2)

#define STM32_BEGIN                         0x7F
#define STM32_ACK                           0x79
#define STM32_NACK                          0x1F

#define STM32_CMD_GET                       0x00    // gets the version and the allowed commands supported by the current version of the bootloader
#define STM32_CMD_GET_VERSION               0x01    // gets the bootloader version and the Read Protection status of the Flash memory
#define STM32_CMD_GET_ID                    0x02    // gets the chip ID
#define STM32_CMD_READ_MEMORY               0x11    // reads up to 256 bytes of memory starting from an address specified by the application
#define STM32_CMD_GO                        0x21    // jumps to user application code located in the internal Flash memory or in SRAM
#define STM32_CMD_WRITE_MEMORY              0x31    // writes up to 256 bytes to the RAM or Flash memory starting from an address specified by the application
#define STM32_CMD_ERASE                     0x43    // erases from one to all the Flash memory pages
#define STM32_CMD_EXT_ERASE                 0x44    // erases from one to all the Flash memory pages using two byte addressing mode
#define STM32_CMD_WRITE_PROTECT             0x63    // enables the write protection for some sectors
#define STM32_CMD_WRITE_UNPROTECT           0x73    // disables the write protection for all Flash memory sectors
#define STM32_CMD_READOUT_PROTECT           0x82    // enables the read protection
#define STM32_CMD_READOUT_UNPROTECT         0x92    // disables the read protection

#define STM32_INFO_BOOTLOADER_VERSION_IDX   0       // Bootloader version (0 < Version < 255), example: 0x10 = Version 1.0
#define STM32_INFO_GET_CMD_IDX              1       // 0x00 - Get command
#define STM32_INFO_GET_VERSION_CMD_IDX      2       // 0x01 - Get Version and Read Protection Status
#define STM32_INFO_GET_ID_CMD_IDX           3       // 0x02 - Get ID
#define STM32_INFO_READ_MEMORY_CMD_IDX      4       // 0x11 - Read Memory command
#define STM32_INFO_GO_CMD_IDX               5       // 0x21 - Go command
#define STM32_INFO_WRITE_MEMORY_CMD_IDX     6       // 0x31 - Write Memory command
#define STM32_INFO_ERASE_CMD_IDX            7       // 0x43 or 0x44 - Erase command or Extended Erase command
#define STM32_INFO_WRITE_PROTECT_CMD_IDX    8       // 0x63 - Write Protect command
#define STM32_INFO_WRITE_UNPROTECT_CMD_IDX  9       // 0x73 - Write Unprotect command
#define STM32_INFO_READOUT_PROTECT          10      // 0x82 - Readout Protect command
#define STM32_INFO_READOUT_UNPROTECT        11      // 0x92 - Readout Unprotect command
#define STM32_INFO_SIZE                     12      // number of bytes in INFO array

#define STM32_VERSION_BOOTLOADER_VERSION    0       // Bootloader version (0 < Version < 255), example: 0x10 = Version 1.0
#define STM32_VERSION_OPTION_BYTE1          1       // option byte 1
#define STM32_VERSION_OPTION_BYTE2          2       // option byte 2
#define STM32_VERSION_SIZE                  3       // number of byted in VERSION array

#define STM32_ID_BYTE1                      0       // production id byte 1
#define STM32_ID_BYTE2                      1       // production id byte 2
#define STM32_ID_SIZE                       2       // number of bytes in ID array

#define STM32_BUFLEN                        256

static uint8_t                              bootloader_info[STM32_INFO_SIZE];

#if 0
static uint8_t                              bootloader_version[STM32_VERSION_SIZE];             // yet not used
static uint8_t                              bootloader_id[STM32_ID_SIZE];                       // yet not used
#endif

static uint8_t                              stm32_buf[STM32_BUFLEN + 1];                        // one more byte for checksum

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * poll a character from serial
 * Returns -1 on errror
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
stm32_serial_poll (unsigned long timeout, bool log_error)
{
    unsigned long       curtime = Millis::elapsed ();
    uint_fast8_t        ch;

    while (! Serial::poll (&ch))
    {
        if (Millis::elapsed () - curtime >= timeout)
        {
            if (log_error)
            {
                HTTP::send ("Timeout<BR>\r\n");
            }
            return -1;
        }
    }

    return (int) ch;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wait for ACK
 * Returns -1 on error
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
wait_for_ack (long timeout, bool show_errors)
{
    int ch;

    if ((ch = stm32_serial_poll(timeout, false)) < 0)
    {
        if (show_errors)
        {
            HTTP::send ("timeout, no ACK<BR>\r\n");
        }

        return -1;
    }
    else if (ch != STM32_ACK)
    {
        if (show_errors)
        {
            char buf[32];
            HTTP::send ("no ACK: ");
            sprintf (buf, "(0x%02x)<BR>\r\n", ch);
            HTTP::send (buf);
        }

        return -1;
    }
    return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * write a bootloader command to STM32
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
stm32_write_cmd (uint8_t cmd)
{
    uint_fast8_t    ch;

    while (Serial::poll (&ch))
    {
    }

    stm32_buf[0] = cmd;
    stm32_buf[1] = ~cmd;
    Serial::send (stm32_buf, 2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: GET
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_get (uint8_t * buf, int maxlen)
{
    int i;
    int ch;
    int n_bytes;

    stm32_write_cmd (STM32_CMD_GET);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command GET failed<BR>\r\n");
        return -1;
    }

    n_bytes = stm32_serial_poll (1000, true);

    if (n_bytes < 0)
    {
        return -1;
    }

    n_bytes++;

    for (i = 0; i < n_bytes; i++)
    {
        ch = stm32_serial_poll (1000, true);

        if (ch < 0)
        {
            return -1;
        }

        if (i < maxlen)
        {
            buf[i] = ch;
        }
    }

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command GET failed<BR>\r\n");
        return -1;
    }
    return n_bytes;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: GET VERSION
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if 0 // yet not used
static int
stm32_get_version (uint8_t * buf, int maxlen)
{
    int i;
    int ch;
    int n_bytes;

    stm32_write_cmd (bootloader_info[STM32_INFO_GET_VERSION_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command GET VERSION failed<BR>\r\n");
        return -1;
    }

    n_bytes = 3;

    for (i = 0; i < n_bytes; i++)
    {
        ch = stm32_serial_poll (1000, true);

        if (ch < 0)
        {
            return -1;
        }

        if (i < maxlen)
        {
            buf[i] = ch;
        }
    }

    if (wait_for_ack (1000, true) < 0)
    {
        return -1;
    }

    return n_bytes;
}
#endif // 0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: GET ID
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if 0 // yet not used
static int
stm32_get_id (uint8_t * buf, int maxlen)
{
    int i;
    int ch;
    int n_bytes;

    stm32_write_cmd (bootloader_info[STM32_INFO_GET_ID_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command GET ID failed<BR>\r\n");
        return -1;
    }

    n_bytes = stm32_serial_poll (1000, true);

    if (n_bytes < 0)
    {
        return -1;
    }

    n_bytes++;

    for (i = 0; i < n_bytes; i++)
    {
        ch = stm32_serial_poll (1000, true);

        if (ch < 0)
        {
            return -1;
        }

        if (i < maxlen)
        {
            buf[i] = ch;
        }
    }

    if (wait_for_ack (1000, true) < 0)
    {
        return -1;
    }
    return n_bytes;
}
#endif // 0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: READ MEMORY
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_read_memory (uint32_t address, unsigned int len)
{
    unsigned int  i;
    int           ch;

    stm32_write_cmd (bootloader_info[STM32_INFO_READ_MEMORY_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command READ MEMORY failed<BR>\r\n");
        return -1;
    }

    stm32_buf[0] = (address >> 24) & 0xFF;
    stm32_buf[1] = (address >> 16) & 0xFF;
    stm32_buf[2] = (address >>  8) & 0xFF;
    stm32_buf[3] = address & 0xFF;
    stm32_buf[4] = stm32_buf[0] ^ stm32_buf[1] ^ stm32_buf[2] ^ stm32_buf[3];

    Serial::send (stm32_buf, 5);

    if (wait_for_ack (1000, true) < 0)
    {
        char logbuf[64];
        sprintf (logbuf, "%02x %02x %02x %02x (%02x)", stm32_buf[0], stm32_buf[1], stm32_buf[2], stm32_buf[3], stm32_buf[4]);
        HTTP::send ("READ MEMORY: address ");
        HTTP::send (logbuf);
        HTTP::send (" failed<BR>\r\n");
        return -1;
    }

    stm32_buf[0] = len - 1;
    stm32_buf[1] = ~stm32_buf[0];
    Serial::send (stm32_buf, 2);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("READ MEMORY: length failed<BR>\r\n");
        return -1;
    }

    for (i = 0; i < len; i++)
    {
        ch = stm32_serial_poll (1000, true);

        if (ch < 0)
        {
            return -1;
        }

        if (i < STM32_BUFLEN)
        {
            stm32_buf[i] = ch;
        }
    }

    return len;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: GO
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if 0 // yet not used
static int
stm32_go (uint32_t address)
{
    stm32_write_cmd (bootloader_info[STM32_INFO_GO_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command GO failed<BR>\r\n");
        return -1;
    }

    stm32_buf[0] = (address >> 24) & 0xFF;
    stm32_buf[1] = (address >> 16) & 0xFF;
    stm32_buf[2] = (address >>  8) & 0xFF;
    stm32_buf[3] = address & 0xFF;
    stm32_buf[4] = stm32_buf[0] ^ stm32_buf[1] ^ stm32_buf[2] ^ stm32_buf[3];

    Serial::send (stm32_buf, 5);

    if (wait_for_ack (1000, true) < 0)
    {
        return -1;
    }
    return 0;
}
#endif // 0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: WRITE MEMORY
 *
 * len must be a multiple of 4!
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_write_memory (uint8_t * image, uint32_t address, unsigned int len)
{
    uint8_t       sum;
    unsigned int  i;

    if (len > 256)
    {
        return -1;
    }

    stm32_write_cmd (bootloader_info[STM32_INFO_WRITE_MEMORY_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command WRITE MEMORY failed<BR>\r\n");
        return -1;
    }

    stm32_buf[0] = (address >> 24) & 0xFF;
    stm32_buf[1] = (address >> 16) & 0xFF;
    stm32_buf[2] = (address >>  8) & 0xFF;
    stm32_buf[3] = address & 0xFF;
    stm32_buf[4] = stm32_buf[0] ^ stm32_buf[1] ^ stm32_buf[2] ^ stm32_buf[3];

    Serial::send (stm32_buf, 5);

    if (wait_for_ack (1000, true) < 0)
    {
        char logbuf[64];
        sprintf (logbuf, "%02x %02x %02x %02x (%02x)", stm32_buf[0], stm32_buf[1], stm32_buf[2], stm32_buf[3], stm32_buf[4]);
        HTTP::send ("WRITE MEMORY: address ");
        HTTP::send (logbuf);
        HTTP::send (" failed<BR>\r\n");
        return -1;
    }

    stm32_buf[0] = len - 1;
    sum = stm32_buf[0];
    Serial::send (stm32_buf, 1);

    for (i = 0; i < len; i++)
    {
        sum ^= image[i];
    }

    Serial::send (image, len);
    stm32_buf[0] = sum;
    Serial::send (stm32_buf, 1);

    if (wait_for_ack (1000, true) < 0)
    {
        char logbuf[64];
        sprintf (logbuf, "%02x %02x %02x %02x", stm32_buf[0], stm32_buf[1], stm32_buf[2], stm32_buf[3]);
        HTTP::send ("WRITE MEMORY: data at address ");
        HTTP::send (logbuf);
        HTTP::send ("failed<BR>\r\n");
        return -1;
    }
    return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: WRITE UNPROTECT
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_write_unprotect (void)
{
    stm32_write_cmd (bootloader_info[STM32_INFO_WRITE_UNPROTECT_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)        // 1st
    {
        HTTP::send ("Command WRITE UNPROTECT (1st) failed<BR>\r\n");
        return -1;
    }

    if (wait_for_ack (1000, true) < 0)        // 2nd
    {
        HTTP::send ("Command WRITE UNPROTECT (2nd) failed<BR>\r\n");
        return -1;
    }
    return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: ERASE
 *
 * n_pages == 0: global erase
 * 1 <= n_pages <= 255: erase N pages
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_erase (uint8_t * pagenumbers, uint16_t n_pages)
{
    int     i;
    uint8_t real_pages;
    uint8_t sum;

    if (n_pages > 256)
    {
        return -1;
    }

    stm32_write_cmd (bootloader_info[STM32_INFO_ERASE_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        return -1;
    }

    real_pages = n_pages;
    n_pages--;                                      // 0x0000 -> 0xFFFF: mass erase
    n_pages &= 0xFF;                                // 0xFFFF -> 0xFF

    stm32_buf[0] = n_pages;
    Serial::send (stm32_buf, 1);

    if (real_pages == 0)
    {
        sum = ~stm32_buf[0];
    }
    else
    {
        sum = stm32_buf[0];

        for (i = 0; i < real_pages; i++)
        {
            stm32_buf[0] = pagenumbers[i];
            Serial::send (stm32_buf, 1);
            sum ^= pagenumbers[i];
        }
    }

    stm32_buf[0] = sum;
    Serial::send (stm32_buf, 1);

    if (wait_for_ack (35000, true) < 0)
    {
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STM32 bootloader command: EXT ERASE
 *
 * n_pages == 0x0000: global mass erase
 * n_pages == 0xFFFF: bank 1 mass erase
 * n_pages == 0xFFFE: bank 2 mass erase
 * 1 <= n_pages < 0xFFF0: erase N pages
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
stm32_ext_erase (uint16_t * pagenumbers, uint16_t n_pages)
{
    int         i;
    uint16_t    real_pages;
    uint8_t     sum;

    stm32_write_cmd (bootloader_info[STM32_INFO_ERASE_CMD_IDX]);

    if (wait_for_ack (1000, true) < 0)
    {
        HTTP::send ("Command EXT ERASE failed<BR>\r\n");
        return -1;
    }

    real_pages = n_pages;
    n_pages--;                                                          // global erase: 0x0000 -> 0xFFFF!

    if (n_pages == 0xFFFF || n_pages == 0xFFFE || n_pages == 0xFFFD)
    {                                                                   // -> 0xFFFF (mass erase), 0xFFFE (bank1 erase), 0xFFFD (bank2 erase)
        stm32_buf[0] = n_pages >> 8;                                    // MSB
        stm32_buf[1] = n_pages & 0xFF;                                  // LSB
        stm32_buf[2] = stm32_buf[0] ^ stm32_buf[1];                     // checksum

        Serial::send (stm32_buf, 3);

        if (wait_for_ack (35000, true) < 0)
        {
            return -1;
        }
    }
    else if (n_pages <= 0xFFF0)
    {
        stm32_buf[0] = n_pages >> 8;                                    // MSB
        stm32_buf[1] = n_pages & 0xFF;                                  // LSB
        Serial::send (stm32_buf, 2);

        sum = stm32_buf[0] ^ stm32_buf[1];

        for (i = 0; i < real_pages ; i++)
        {
            stm32_buf[0] = pagenumbers[i] >> 8;                         // MSB
            stm32_buf[1] = pagenumbers[i] & 0xFF;                       // LSB
            sum ^= stm32_buf[0] ^ stm32_buf[1];
            Serial::send (stm32_buf, 2);
        }

        stm32_buf[0] = sum;
        Serial::send (stm32_buf, 1);

        if (wait_for_ack (35000, true) < 0)
        {
            return -1;
        }
    }
    else
    {
        return -1;                                                      // codes from 0xFFF0 to 0xFFFD are reserved
    }

    return 0;
}

#define PAGESIZE        256
static uint32_t         start_address   = 0x00000000;                   // address of program start

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * stm32_flash_image (do_flash) - check or flash image
 *
 * do_flash == false: only check file
 * do_flash == true: check and flash file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define LINE_BUFSIZE    256
static int
stm32_flash_image (const char * fname, bool do_flash)
{
    char            linebuf[LINE_BUFSIZE];
    char            logbuf[128];
    uint32_t        i;
    int             len;
    int             line;
    uint8_t         pagebuf[PAGESIZE];
    uint32_t        pageaddr        = 0xffffffff;
    uint32_t        pageidx         = 0;
    unsigned char   datalen;
    uint32_t        address_min     = 0xffffffff;               // minimum address (incl.)
    uint32_t        address_max     = 0x00000000;               // maximum address (incl.)
    uint32_t        last_address    = 0x00000000;
    uint32_t        drlo;                                       // DATA Record Load Offset (current address, 2 bytes)
    uint32_t        ulba            = 0x00000000;               // Upper Linear Base Address (address offset, 4 bytes)
    unsigned char   dri;                                        // Data Record Index
    uint32_t        bytes_written;
    uint32_t        bytes_read;
    uint32_t        pages_written;
    int             eof_record_found = 0;
    int             idx;
    int             ch;
    int             errors = 0;
    int             rtc = 0;

    if (do_flash)
    {
        HTTP::send ("STM32 wird geflasht...<br/>");
        HTTP::flush ();
    }
    else
    {
        HTTP::send ("HEX-Datei ");
        HTTP::send (fname);
        HTTP::send (" wird &uuml;berpr&uuml;ft ...<br>");
    }

    FILE * f = fopen (fname, "r");

    if (f)
    {
        line            = 0;
        bytes_read      = 0;
        bytes_written   = 0;
        pages_written   = 0;

        while (! feof (f))
        {
            idx = 0;

            while ((ch = getc (f)) != EOF)
            {
                if (ch != '\r')
                {
                    if (ch == '\n')
                    {
                        linebuf[idx] = '\0';
                        break;
                    }
                    else if (idx < LINE_BUFSIZE - 1)
                    {
                        linebuf[idx] = ch;
                        idx++;
                    }
                }
            }

            if (idx == 0)
            {
                break;
            }

            line++;
            bytes_read += idx;

            len = idx;
    
            if (linebuf[0] == ':' && len >= 11)
            {
                unsigned char   addrh;
                unsigned char   addrl;
                unsigned char   rectype;
                char *          dataptr;
                unsigned char   chcksum;
                int             sum = 0;
                unsigned char   ch;
    
                datalen = hex2toi (linebuf +  1);
                addrh   = hex2toi (linebuf +  3);
                addrl   = hex2toi (linebuf +  5);
                drlo    = (addrh << 8 | addrl);
                rectype = hex2toi (linebuf + 7);
                dataptr = linebuf + 9;
    
                sum = datalen + addrh + addrl + rectype;
    
                if (len == 9 + 2 * datalen + 2)
                {
                    if (rectype == 0)                               // Data Record
                    {
                        for (dri = 0; dri < datalen; dri++)
                        {
                            ch = hex2toi (dataptr);
    
                            pageidx = ulba + drlo + dri;
    
                            if (pageidx - pageaddr >= PAGESIZE)
                            {
                                if (pageaddr != 0xffffffff)
                                {
                                    if (do_flash)
                                    {
                                        // flash (pagebuf, pageaddr, pageidx - pageaddr);
                                        if (stm32_write_memory (pagebuf, pageaddr, pageidx - pageaddr) < 0)
                                        {
                                            rtc = -1;
                                            break;
                                        }
        
                                        if (stm32_read_memory (pageaddr, pageidx - pageaddr) < 0)
                                        {
                                            rtc = -1;
                                            break;
                                        }
        
                                        if (memcmp (pagebuf, stm32_buf, pageidx - pageaddr) != 0)
                                        {
                                            errors++;
                                            char tmpbuf[32];
                                            HTTP::send ("verify failed at address=\r\n");
                                            sprintf (tmpbuf, "%08X \r\n", pageaddr);
                                            HTTP::send (tmpbuf);
                                            sprintf (tmpbuf, "len=%d<BR>\r\n", pageidx - pageaddr);
                                            HTTP::send (tmpbuf);
                                            HTTP::send ("pagebuf:<BR><pre>\r\n");
        
                                            for (i = 0; i < pageidx - pageaddr; i++)
                                            {
                                                sprintf (tmpbuf, "%02X ", pagebuf[i]);
                                                HTTP::send (tmpbuf);
                                            }
        
                                            HTTP::send ("</pre><BR>\r\n");
        
                                            HTTP::send ("stm32_buf:<BR><pre>\r\n");
        
                                            for (i = 0; i < pageidx - pageaddr; i++)
                                            {
                                                sprintf (tmpbuf, "%02X ", stm32_buf[i]);
                                                HTTP::send (tmpbuf);
                                            }
        
                                            HTTP::send ("</pre><BR>\r\n");
                                            rtc = -1;
                                            break;
                                        }
        
                                        bytes_written += pageidx - pageaddr;
                                    }

                                    pages_written++;

                                    if (do_flash)
                                    {
                                        HTTP::send (".");
    
                                        if (pages_written % 80 == 0)
                                        {
                                            HTTP::send ("<br>");
                                        }

                                        HTTP::flush ();
                                    }
                                }
    
                                pageaddr = ulba + drlo + dri;
                                memset (pagebuf, 0xFF, PAGESIZE);
                            }
    
                            pagebuf[pageidx - pageaddr] = ch;

                            if (! do_flash) // only check
                            {
                                if (last_address && last_address + 1 != ulba + drlo + dri)
                                {
                                    // sprintf (logbuf, "Info: gap in line %d, addr 0x%08X. This is normal.\r\n", line, ulba + drlo + dri);
                                    // HTTP::send (logbuf);
                                }
                                last_address = ulba + drlo + dri;
                            }

                            if (address_min > ulba + drlo + dri)
                            {
                                address_min = ulba + drlo + dri;
                            }
    
                            if (address_max < ulba + drlo + dri)
                            {
                                address_max = ulba + drlo + dri;
                            }
    
                            sum += ch;
                            dataptr +=2;
                        }
    
                        if (rtc == -1)
                        {
                            break;
                        }
                    }
                    else if (rectype == 1)                          // End of File Record
                    {
                        eof_record_found = 1;

                        for (dri = 0; dri < datalen; dri++)
                        {
                            ch = hex2toi (dataptr);
    
                            sum += ch;
                            dataptr +=2;
                        }
    
                        if (pageidx - pageaddr > 0)
                        {
                            pageidx++;                              // we have to go behind last byte

                            if (do_flash)
                            {
                                // flash (pagebuf, pageaddr, pageidx - pageaddr);
                                if (stm32_write_memory (pagebuf, pageaddr, pageidx - pageaddr) < 0)
                                {
                                    rtc = -1;
                                    break;
                                }
        
                                if (stm32_read_memory (pageaddr, pageidx - pageaddr) < 0)
                                {
                                    rtc = -1;
                                    break;
                                }
        
                                if (memcmp (pagebuf, stm32_buf, pageidx - pageaddr) != 0)
                                {
                                    errors++;

                                    char tmpbuf[32];

                                    HTTP::send ("verify failed at address \r\n");
                                    sprintf (tmpbuf, "%08X<BR>\r\n", pageaddr);
                                    HTTP::send (tmpbuf);
                                    HTTP::send ("pagebuf:<BR><pre>\r\n");

                                    for (i = 0; i < pageidx - pageaddr; i++)
                                    {
                                        sprintf (tmpbuf, "%02X ", pagebuf[i]);
                                        HTTP::send (tmpbuf);
                                    }

                                    HTTP::send ("</pre><BR>\r\n");
                                    HTTP::send ("stm32_buf:<BR><pre>\r\n");
        
                                    for (i = 0; i < pageidx - pageaddr; i++)
                                    {
                                        sprintf (tmpbuf, "%02X ", stm32_buf[i]);
                                        HTTP::send (tmpbuf);
                                    }
        
                                    HTTP::send ("</pre><BR>\r\n");
                                    rtc = -1;
                                    break;
                                }
        
                                bytes_written += pageidx - pageaddr;
                                pageidx = 0;
                                memset (pagebuf, 0xFF, PAGESIZE);
                            }

                            pages_written++;

                            if (do_flash)
                            {
                                HTTP::send (".");
    
                                if (pages_written % 80 == 0)
                                {
                                    HTTP::send ("<br>");
                                }
                                HTTP::flush ();
                            }
                        }
                        break;                                      // stop reading here
                    }
                    else if (rectype == 4)                          // Extended Linear Address Record
                    {
                        if (drlo == 0)
                        {
                            ulba = 0;
    
                            for (dri = 0; dri < datalen; dri++)
                            {
                                ch = hex2toi (dataptr);
    
                                ulba <<= 8;
                                ulba |= ch;
    
                                sum += ch;
                                dataptr +=2;
                            }
    
                            ulba <<= 16;
                        }
                        else
                        {
                            sprintf (logbuf, "line %d: rectype = %d: address field is 0x%04X<BR>\r\n", line, rectype, drlo);
                            HTTP::send (logbuf);
                            rtc = -1;
                            break;
                        }
                    }
                    else if (rectype == 5)                          // Start Linear Address Record
                    {
                        if (drlo == 0)
                        {
                            start_address = 0;
    
                            for (dri = 0; dri < datalen; dri++)
                            {
                                ch = hex2toi (dataptr);
    
                                start_address <<= 8;
                                start_address |= ch;
    
                                sum += ch;
                                dataptr +=2;
                            }
                        }
                        else
                        {
                            sprintf (logbuf, "line %d: rectype = %d: address field is 0x%04X<BR>\r\n", line, rectype, drlo);
                            HTTP::send (logbuf);
                            rtc = -1;
                            break;
                        }
                    }
                    else
                    {
                        sprintf (logbuf, "line %d: unsupported record type: %d<BR>\r\n", line, rectype);
                        HTTP::send (logbuf);
                        rtc = -1;
                        break;
                    }
    
                    sum = (0x100 - (sum & 0xff)) & 0xff;
                    chcksum = hex2toi (dataptr);
    
                    if (sum != chcksum)
                    {
                        sprintf (logbuf, "invalid checksum: sum: 0x%02X chcksum: 0x%02X<BR>\r\n", sum, chcksum);
                        HTTP::send (logbuf);
                        rtc = -1;
                        break;
                    }
                }
                else
                {
                    sprintf (logbuf, "invalid len: %d, expected %d<BR>\r\n", len, 12 + 2 * datalen + 2);
                    HTTP::send (logbuf);
                    rtc = -1;
                    break;
                }
            }
            else
            {
                sprintf (logbuf, "invalid INTEL HEX format, len: %d<BR>\r\n", len);
                HTTP::send (logbuf);
                rtc = -1;
                break;
            }
        }
    
        fclose (f);

        if (do_flash)
        {
#if 0
            sprintf (logbuf, "<BR>Lines read: %d<BR>\r\n", line);
            HTTP::send (logbuf);
            sprintf (logbuf, "Pages flashed: %u<BR>\r\n", pages_written);
            HTTP::send (logbuf);
            sprintf (logbuf, "Bytes flashed: %u<BR>\r\n", bytes_written);
            HTTP::send (logbuf);
            sprintf (logbuf, "Flash write errors: %d<BR>\r\n", errors);
            HTTP::send (logbuf);
#endif
        }
        else
        {
            if (rtc == 0 && ! eof_record_found)
            {
                HTTP::send ("Fehler: Es wurde kein EOF-Record gefunden.<BR>\r\n");
                rtc = -1;
            }

            if (rtc == 0)
            {
                HTTP::send ("Pr&uuml;fung erfolgreich. \r\n");
                sprintf (logbuf, "Dateigr&ouml;&szlig;e: %d<BR>\r\n", bytes_read + 2 * line);
                HTTP::send (logbuf);
            }
            else
            {
                HTTP::send ("Pr&uuml;fung fehlgeschlagen.<BR>\r\n");
            }
        }
    }
    else
    {
        HTTP::send ("Fehler: Datei kann nicht ge&ouml;ffnet werden.<br/>");
        rtc = -1;
    }

    if (do_flash)
    {
        HTTP::flush ();
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * start STM32 bootloader
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define N_RETRIES    4

static int
stm32_activate_bootloader (void)
{
    uint8_t buffer[1];
    int     rtc = 0;
    int     ch;
    int     i;

    buffer[0] = STM32_BEGIN;

    for (i = 0; i < N_RETRIES; i++)
    {
        Serial::send (buffer, 1);
        ch = stm32_serial_poll (1000, true);

        if (ch == STM32_ACK)
        {
            break;
        }
    }

    if (i == N_RETRIES)
    {
        HTTP::send ("Aktivierung des Bootloaders fehlgeschlagen<br>\r\n");
        rtc = -1;
    }

    return rtc;
}

static int
stm32_bootloader (const char * fname, bool do_unprotect)
{
    uint8_t         buffer[256];
    unsigned long   checktime;
    unsigned long   erasetime;
    unsigned long   flashtime;
    int             rtc = 0;

    rtc = stm32_activate_bootloader ();

    if (rtc < 0)
    {
        return rtc;
    }

    rtc = stm32_get (bootloader_info, STM32_INFO_SIZE);

    if (rtc < 0)
    {
        return rtc;
    }

    HTTP::send ("STM32-Bootloader-Version: ");
    sprintf ((char *) buffer, "%X.%X", bootloader_info[STM32_INFO_BOOTLOADER_VERSION_IDX] >> 4, bootloader_info[STM32_INFO_BOOTLOADER_VERSION_IDX] & 0x0F);
    HTTP::send ((char *) buffer);
    HTTP::send ("<BR>\r\n");

#if 0
    rtc = stm32_get_version (bootloader_version, STM32_VERSION_SIZE);

    if (rtc < 0)
    {
        return rtc;
    }

    HTTP::send ("Option Bytes: ");
    sprintf (buffer, "0x%02X 0x%02X", bootloader_version[STM32_VERSION_OPTION_BYTE1], bootloader_version[STM32_VERSION_OPTION_BYTE2]);
    HTTP::send (buffer);
    HTTP::send ("<BR>\r\n");
#endif

    if (do_unprotect)
    {
        rtc = stm32_write_unprotect ();

        if (rtc >= 0)
        {
            usleep (500000);
            rtc = stm32_activate_bootloader ();
        }
    }
    else
    {
        rtc = 0;
    }

    if (rtc >= 0)
    {
        checktime = Millis::elapsed ();
        rtc = stm32_flash_image (fname, false);
        checktime = Millis::elapsed () - checktime;
    }

    if (rtc >= 0)
    {
        if (bootloader_info[STM32_INFO_ERASE_CMD_IDX] == STM32_CMD_ERASE)
        {
            HTTP::send ("Flash wird gel&ouml;scht... ");
            HTTP::flush ();
            erasetime = Millis::elapsed ();
            rtc = stm32_erase (0, 0);
            erasetime = Millis::elapsed () - erasetime;
        }
        else if (bootloader_info[STM32_INFO_ERASE_CMD_IDX] == STM32_CMD_EXT_ERASE)
        {
            HTTP::send ("Flash wird gel&ouml;scht... ");
            HTTP::flush ();
            erasetime = Millis::elapsed ();
            rtc = stm32_ext_erase (0, 0);
            erasetime = Millis::elapsed () - erasetime;
        }
        else
        {
            HTTP::send ("Fehler: Unbekannte L&ouml;schmethode<br>");
            HTTP::flush ();
            rtc = -1;
        }

        if (rtc >= 0)
        {
            HTTP::send (" erfolgreich!<br>\r\n");
            HTTP::flush ();
        }
    }

    if (rtc >= 0)
    {
        flashtime = Millis::elapsed ();
        rtc = stm32_flash_image (fname, true);
        flashtime = Millis::elapsed () - flashtime;

        if (rtc == 0)
        {
            HTTP::send ("<BR>Flash erfolgreich. ");
        }
        else
        {
            HTTP::send ("<BR>Flash fehlgeschlagen. ");
        }

        HTTP::send ("<span style='border:1px lightgray solid; cursor:pointer;' onclick=\"document.getElementById('details').style.display=''\">Details</span>\r\n");
        HTTP::send ("<table id='details' style='border:1px lightgray solid;display:none'>");

        sprintf ((char *) buffer, "%lu", checktime);
        HTTP::send ("<tr><td>Pr&uuml;fzeit:</td><td align='right'>");
        HTTP::send ((char *) buffer);
        HTTP::send (" msec</td></tr>");

        sprintf ((char *) buffer, "%lu", erasetime);
        HTTP::send ("<tr><td>L&ouml;schzeit:</td><td align='right'>");
        HTTP::send ((char *) buffer);
        HTTP::send (" msec</td></tr>");

        sprintf ((char *) buffer, "%lu", flashtime); 
        HTTP::send ("<tr><td>Programmierzeit:</td><td align='right'>");
        HTTP::send ((char *) buffer);
        HTTP::send (" msec</td></tr></table>");

        HTTP::send ("<BR>\r\n");
        HTTP::flush ();
    }

    return rtc;
}

static void
stm32_start_bootloader (void)
{
    GPIO::activate_stm32_boot0 ();                      // deactivate BOOT0
    GPIO::activate_stm32_nrst ();                       // activate NRST
    usleep (200000);                                    // wait 200ms
    GPIO::deactivate_stm32_nrst ();                     // deactivate NRST

#if 1
    while (stm32_serial_poll (1000, false) >= 0)            // flush characters in serial input
    {
        ;
    }
#else
    uint_fast8_t        ch;
    while (Serial::poll (&ch)) ;
#endif

}

void
STM32::check_hex_file (const char * fname)
{
    stm32_flash_image (fname, false);
}

void
STM32::flash_from_local (const char * fname)
{
    stm32_start_bootloader ();
    HTTP::send ("Bootloader wird gestartet<BR>\r\n");
    HTTP::flush ();
    stm32_bootloader (fname, true);
    HTTP::send ("Bootloader wird beendet<BR>\r\n");
    HTTP::flush ();
    HTTP::send ("STM32 wird neu gestartet<BR>\r\n");
    HTTP::flush ();
    STM32::reset ();
}

void
STM32::reset (void)
{
    GPIO::deactivate_stm32_boot0 ();                    // deactivate BOOT0
    GPIO::activate_stm32_nrst ();                       // activate NRST
    usleep (200000);                                    // wait 200msec
    GPIO::deactivate_stm32_nrst ();                     // deactivate NRST
}

void
STM32::init (void)
{
    if (GPIO::init ())
    {
        GPIO::deactivate_stm32_boot0 ();
        GPIO::deactivate_stm32_nrst ();     
    }
}
