/*------------------------------------------------------------------------------------------------------------------------
 * serial.cc - serial functions
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
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "serial.h"

#if 0
#define DEVICE  "/dev/ttyS0"
#define DEVICE  "/dev/ttyUSB0"
#else                                                                                       // RPi
#define DEVICE  "/dev/ttyAMA0"
#endif

static int              fd = -1;
static struct termios   tty;
static struct termios   tty_old;
static int              raw_mode;

/*------------------------------------------------------------------------------------------------------------------------------------
 * Serial::read - read a character
 *------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
Serial::poll (uint_fast8_t * chp)
{
    uint_fast8_t rtc = 0;

    if (fd >= 0)
    {
        uint8_t buf[1];

        if (read (fd, buf, 1) == 1)
        {
            *chp = buf[0];
            rtc = 1;
        }
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------------------
 * Serial::send - write a character
 *------------------------------------------------------------------------------------------------------------------------------------
 */
int
Serial::send (uint_fast8_t ch)
{
    int     rtc;

    if (fd >= 0)
    {
        uint8_t buf[1];

        buf[0] = ch;
        rtc = write (fd, buf, 1);
    }
    else
    {
        rtc = -1;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------------------
 * Serial::send - write a buffer
 *------------------------------------------------------------------------------------------------------------------------------------
 */
void
Serial::send (uint8_t * bufp, uint32_t len)
{
    while (len--)
    {
        Serial::send (*bufp++);
    }
}

/*------------------------------------------------------------------------------------------------------------------------------------
 * restore_tty - restore tty
 *------------------------------------------------------------------------------------------------------------------------------------
 */
static void
restore_tty (void)
{
    if (raw_mode && fd >= 0)
    {
        tcsetattr (fd, TCSANOW, &tty_old);
    }
}

/*------------------------------------------------------------------------------------------------------------------------------------
 * Serial::init - init serial tty
 *
 * Cases:
 * 1) TIME > 0 && MIN > 0
 *
 *      TIME specifies how long to wait after each input character to see if more input arrives.
 *      After the first character received, read keeps waiting until either MIN bytes have arrived in all,
 *      or TIME elapses with no further input. read always blocks until the first character arrives, even if
 *      TIME elapses first. read can return more than MIN characters if more than MIN happen to be in the queue.
 *
 * 2) MIN == 0 && TIME == 0
 *
 *      read always returns immediately with as many characters as are available in the queue, up to the number
 *      requested. If no input is immediately available, read returns a value of zero.
 *
 * 3) MIN == 0 && TIME > 0
 *
 *      read waits for time TIME for input to become available; the availability of a single byte is enough to satisfy
 *      the read request and cause read to return. When it returns, it returns as many characters as are available, up
 *      to the number requested. If no input is available before the timer expires, read returns a value of zero.
 *
 * 4) TIME is zero but MIN has a nonzero value.
 *
 *      read waits until at least MIN bytes are available in the queue. At that time, read returns as many characters as are
 *      available, up to the number requested. read can return more than MIN characters if more than MIN happen to be in the queue. 
 *
 *------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t 
Serial::init (void)
{
    char    junk;

    fd = open(DEVICE, O_RDWR);

    if (fd < 0)
    {
        perror (DEVICE);
        return 0;
    }

    if (tcgetattr(fd, &tty) != 0)
    {
        close (fd);
        fd = -1;
        perror (DEVICE);
        return 0;
    }

    if (tcgetattr(fd, &tty_old) != 0)
    {
        close (fd);
        fd = -1;
        perror (DEVICE);
        return 0;
    }

    tty.c_cflag |= PARENB;   //enable parity
    tty.c_cflag &= ~PARODD;
//  tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 0;                                                                    // don't wait
    tty.c_cc[VMIN]  = 0;                                                                    // min is 0

    cfsetispeed (&tty, B115200);
    cfsetospeed (&tty, B115200);

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        close (fd);
        fd = -1;
        perror (DEVICE);
        return 0;
    }

    raw_mode = 1;                                                                           // restore on signal exit
    atexit (restore_tty);                                                                   // restore terminal modes at exit

#if 0
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1)
    {
        perror ("TIOCMGET failed");
    }
    else
    {
        if (status & TIOCM_RTS)
        {
            puts("TIOCM_RTS is not set");
        }
        else
        {
            puts("TIOCM_RTS is set");
        }
    }
#endif

    while (read (fd, &junk, 1))
    {
    }

    return 1;
}
