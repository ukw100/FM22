/*------------------------------------------------------------------------------------------------------------------------
 * http.cc - http server
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
 */
#include <string>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/wait.h>

#include "http.h"
#include "http-common.h"
#include "http-loco.h"
#include "http-addon.h"
#include "http-led.h"
#include "http-switch.h"
#include "http-sig.h"
#include "http-railroad.h"
#include "http-s88.h"
#include "http-rcl.h"
#include "http-pgm.h"
#include "http-pom.h"
#include "http-pommot.h"
#include "http-pommap.h"
#include "http-pomout.h"
#include "loco.h"
#include "stm32.h"
#include "debug.h"
#include "base.h"

#define WRITE_CHUNK_SIZE        1024

#define MAX_PARAMETERS          2048
#define MAX_PARAMETER_NAME_LEN  64
#define MAX_PARAMETER_VALUE_LEN 256

#define MAX_REQUEST_LEN         8096                                // 8 KB
#define MAX_POST_LEN            131072                              // 128 KB

#define METHOD_NONE             0
#define METHOD_GET              1
#define METHOD_POST             2
#define METHOD_POST_MULTI       3

static char *               boundary        = (char *) 0;

static char                 request_buf[MAX_REQUEST_LEN];
static char                 post_buf[MAX_POST_LEN];

String                      HTTP::response;

static int                  sock_fd;
static struct sockaddr_in   http_listen_addr;
static socklen_t            http_listen_size;
static int                  http_fd;

static char *               request_file;
static char *               request_parameter_name[MAX_PARAMETERS];
static char *               request_parameter_value[MAX_PARAMETERS];
static int                  n_parameters;

extern uint16_t             limit;
extern uint16_t             min_lower_value;
extern uint16_t             max_lower_value;
extern uint16_t             min_upper_value;
extern uint16_t             max_upper_value;
extern uint16_t             min_cnt;
extern uint16_t             max_cnt;

/*------------------------------------------------------------------------------------------------------------------------------------
 * bind_listen_port ()
 *------------------------------------------------------------------------------------------------------------------------------------
 */
static int
bind_listen_port (int port)
{
    int so_reuseaddr;
    int opt;

    if ((sock_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)                               // open socket
    {
        return -1;
    }

    http_listen_size = sizeof (http_listen_addr);
    (void) memset ((char *) &http_listen_addr, 0, http_listen_size);

    http_listen_addr.sin_family      = AF_INET;
    http_listen_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    http_listen_addr.sin_port        = htons (port);

    so_reuseaddr = 1;                                                                   // re-use port immediately
    (void) setsockopt (sock_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof so_reuseaddr);

    if (bind (sock_fd, (struct sockaddr *) &http_listen_addr, http_listen_size) < 0)
    {
        close (sock_fd);
        sock_fd = -1;
        return -1;
    }

    opt = 1;
    (void) setsockopt (sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, sizeof (opt));

    if (listen (sock_fd, 5) < 0)
    {
        close (sock_fd);
        sock_fd = -1;
        return -1;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------------------------
 * accept_port ()
 *------------------------------------------------------------------------------------------------------------------------------------
 */
static int
accept_port (long timeout_usec)
{
    static unsigned char    addr[4];                                        // ip address
    int                     new_fd;
    int                     opt;

    if (timeout_usec > 0)
    {
        struct timeval  tv;
        fd_set          fdset;
        int             max_fd;
        int             select_rtc;

        tv.tv_sec   = 0;
        tv.tv_usec  = timeout_usec;

        max_fd = sock_fd;

        FD_ZERO(&fdset);
        FD_SET (sock_fd, &fdset);

        select_rtc = select (max_fd + 1, &fdset, (fd_set *) 0, (fd_set *) 0, &tv);

        if (select_rtc <= 0)                        // 0 = timeout, -1 = error
        {
            return select_rtc;
        }
    }

    new_fd = accept (sock_fd, (struct sockaddr *) &http_listen_addr, &http_listen_size);

    if (new_fd >= 0)
    {
        (void) memcpy (addr, &(http_listen_addr.sin_addr.s_addr), 4);

        opt = 1;
        (void) setsockopt (new_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, sizeof (opt));
    }
    return new_fd;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_request ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_request (void)
{
    char    buf[1];
    int     request_len;
    int     len;
    int     ch;
    int     last_ch = '\0';

    request_len = 0;
    request_buf[0] = '\0';

    while (1)
    {
        len = recv (http_fd, buf, 1, 0);

        if (len > 0)
        {
            ch = buf[0];

            if (ch == '\r')
            {
                continue;
            }

            if (ch == '\n' && last_ch == '\n')
            {
                request_buf[request_len] = '\0';        // terminate for debugging
                break;
            }

            if (request_len < MAX_REQUEST_LEN - 2)
            {
                request_buf[request_len++] = ch;
            }

            last_ch = ch;
        }
        else
        {
            return len;
        }
    }

    return (request_len);
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_post ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_post (int len)
{
    int     n;
    int     nsum    = 0;

    do
    {
        n = recv (http_fd, post_buf + nsum, MAX_POST_LEN - nsum - 1, 0);

        if (n < 0)
        {
            perror ("recv");
            nsum = -1;
            break;
        }
        else if (n == 0)
        {
            Debug::printf (DEBUG_LEVEL_NORMAL, "http_post: got 0\n");
        }
        else
        {
            nsum += n;
        }

    } while (n > 0 && nsum < len);

    if (nsum >= 0)
    {
        post_buf[nsum] = '\0';
    }

    return nsum;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_write ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_write (const char * buf, int len)
{
    int rtc = 0;

    while (len >= WRITE_CHUNK_SIZE)
    {
        rtc = write (http_fd, buf, WRITE_CHUNK_SIZE);

        if (rtc < 0)
        {
            break;
        }

        len -= WRITE_CHUNK_SIZE;
        buf += WRITE_CHUNK_SIZE;
    }

    if (rtc >= 0 && len)
    {
        rtc = write (http_fd, buf, len);
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * http_puts ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_puts (const char * s)
{
    int len = strlen (s);
    int rtc = http_write (s, len);

    if (rtc >= 0)
    {
        rtc = 0;
    }
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * http_puts ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_puts (String str)
{
    const char *    s   = str.c_str();
    int             len = str.length();
    int             rtc;

    rtc = http_write (s, len);

    if (rtc >= 0)
    {
        rtc = 0;
    }
    return rtc;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_header ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_header (void)
{
    http_puts ("HTTP/1.1 200 OK\r\nServer: FM/1.1.1 (Linux)\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n");
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP::parameter ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
const char *
HTTP::parameter (const char * name)
{
    int idx;

    for (idx = 0; idx < n_parameters; idx++)
    {
        if (! strcmp (request_parameter_name[idx], name))
        {
            Debug::printf (DEBUG_LEVEL_VERBOSE, "HTTP::parameter: name='%s' value='%s'\n", name, request_parameter_value[idx]);
            return request_parameter_value[idx];
        }
    }
    Debug::printf (DEBUG_LEVEL_VERBOSE, "HTTP::parameter: name='%s' value=<none>\n", name);
    return (const char *) "";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP::parameter_number ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
int
HTTP::parameter_number (const char * name)
{
    return atoi (HTTP::parameter (name));
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP::parameter ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
const char *
HTTP::parameter (String sname)
{
    const char *    name = sname.c_str();
    return HTTP::parameter (name);
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * HTTP::parameter_number ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
int
HTTP::parameter_number (String sname)
{
    return atoi (HTTP::parameter (sname));
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_normalize ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_normalize (char * str)
{
    if (str)
    {
        char * p;
        char * t;

        for (p = str, t = str; *p; p++, t++)
        {
            if (*p == '+')
            {
                *t = ' ';
                continue;
            }
            else if (*p == '%' && *(p + 1) && *(p + 2))
            {
                *t = htoi (p + 1, 2);
                p += 2;
            }
            else if (*t != *p)
            {
                *t = *p;
            }
        }

        if (*t != '\0')
        {
            *t = '\0';
        }
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_pre_actions ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_pre_actions (const char * action)
{
    if (strcmp (action, "delete") == 0)
    {
        char    fname[256];
        char *  filename;
        size_t  len;

        strcpy (fname, HTTP::parameter ("fname"));       // copy, because basename will alter buffer
        filename = basename (fname);                    // accept only file names in current path
        len = strlen (filename);

        if (len > 4 && ! strcasecmp (filename + len - 4, ".hex"))
        {
            unlink (fname);
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Fehler: Ung&uuml;ltiger Dateiname.</font><BR><BR>\r\n";
        }
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_post_actions ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_post_actions (const char * action)
{
    if (strcmp (action, "check") == 0)
    {
        char    fname[256];
        char *  filename;
        size_t  len;

        strcpy (fname, HTTP::parameter ("fname"));       // copy, because basename will alter buffer
        filename = basename (fname);                    // accept only file names in current path
        len = strlen (filename);

        HTTP::response += (String) "<BR>\r\n";

        if (len > 4 && ! strcasecmp (filename + len - 4, ".hex"))
        {
            STM32::check_hex_file (fname);
        }
        else
        {
            HTTP::response += (String) "<font color='red'>Fehler: Ung&uuml;ltiger Dateiname.</font><BR>\r\n";
        }
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * show_directory ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
show_directory (const char * action, String url)
{
    DIR * dirp;

    handle_pre_actions (action);

    dirp = opendir (".");

    if (dirp)
    {
        struct dirent * entry;

        HTTP::response += (String)
            "<B>Directory:</B>\r\n"
            "<table style='border:1px gray solid\'>\r\n"
            "<tr bgcolor='#e0e0e0'><th width='120' align='left'>Dateiname</th><th>Gr&ouml;&szlig;e</th><th>Datum</th><th colspan='3'>Aktion</th></tr>\r\n";
 
        while ((entry = readdir (dirp)) != 0)
        {
            char *  filename = entry->d_name;
            size_t  len = strlen (filename);

            if (len > 4 && ! strcasecmp (filename + len - 4, ".hex"))
            {
                struct stat st;
                struct tm * tmp;
                char        stime[80];

                if (stat (filename, &st) == 0)
                {
                    tmp = localtime (&st.st_mtime);

                    sprintf (stime, "%04d-%02d-%02d %02d:%02d", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min);

                    HTTP::response += (String)
                        "<tr>"
                        "<td nowrap>" + filename +
                        "</td><td align='right'>" + std::to_string (st.st_size) +
                        "</td><td align='right' nowrap>" + stime +
                        "</td>"
                        "<td>\r\n"
                        "<form action='" + url +
                        "' method='GET'>\r\n"
                        "  <input type='hidden' name='action' value='delete'>\r\n"
                        "  <input type='hidden' name='fname'  value='" + filename +
                        "'>\r\n"
                        "  <input type='submit' value='Delete'>\r\n"
                        "</form>\r\n"
                        "</td>\r\n"
                        "<td>\r\n"
                        "<form action='" + url +
                        "' method='GET'>\r\n"
                        "  <input type='hidden' name='action' value='check'>\r\n"
                        "  <input type='hidden' name='fname'  value='" + filename +
                        "'>\r\n"
                        "  <input type='submit' value='Check'>\r\n"
                        "</form>\r\n"
                        "</td>\r\n"
                        "<td>\r\n"
                        "<form action='/flash' method='GET'>\r\n"
                        "  <input type='hidden' name='action' value='flash'>\r\n"
                        "  <input type='hidden' name='fname'  value='" + filename +
                        "'>\r\n"
                        "  <input type='submit' value='Flash'>\r\n"
                        "</form>\r\n"
                        "</td>\r\n"
                        "</tr>\r\n";
                }
            }
        }

        HTTP::response += (String) "</table>\r\n";
    }
    
    handle_post_actions (action);
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * print_upload_form ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
print_upload_form (void)
{
    HTTP::response += (String)
        "<BR>\r\n"
        "<form method='POST' action='/doupload' enctype='multipart/form-data'>\r\n"
        "HEX-Datei hochladen:<br><br>\r\n"
        "<input type='file' accept='.hex' name='file'>\r\n"
        "<input type='submit' value='Upload'>\r\n"
        "</form>\r\n";
}

#define REASON_NONE                 0
#define REASON_CANNOT_OPEN_FILE     1
#define REASON_INVALID_FILENAME     2

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_doupload ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_doupload (void)
{
    String          title               = "Upload Hex";
    String          url                 = "/upl";
    const char *    action              = HTTP::parameter ("action");
    uint_fast8_t    reason              = REASON_NONE;
    char *          fname               = (char *) 0;
    bool            upload_successful   = false;

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String)
        "<P>\r\n"
        "<div style='margin-top:10px;margin-bottom:10px;margin-left:20px;padding:10px;border:1px lightgray solid;display:inline-block;'>\r\n";

    if (n_parameters > 0 && boundary)
    {
        char    start_boundary[128];
        char    end_boundary[128];
        char *  par = request_parameter_name[0];
        char *  p;

        sprintf (start_boundary, "--%s\r\n", boundary);
        sprintf (end_boundary, "\r\n--%s--", boundary);

        if (! strncmp (par, start_boundary, strlen (start_boundary)))
        {
            p = strchr (par, '\n');

            if (p)
            {
                p++;

                if (! strncmp (p, "Content-Disposition: form-data; name=\"file\"; filename=\"", 55))
                {
                    fname = p + 55;
                    p = strchr (p + 55, '"');

                    if (p)
                    {
                        size_t  len;

                        *p = '\0';
                        fname = basename (fname);               // don't accept filenames with full path
                        len = strlen (fname);

                        if (len > 4 && ! strcasecmp (fname + len - 4, ".hex"))
                        {
                            FILE * fp = fopen (fname, "w");

                            if (fp)
                            {
                                char * pp;

                                p = request_parameter_value[0];
                                pp = strstr (p, end_boundary);

                                if (pp)
                                {
                                    *pp = '\0';

                                    while (*p)
                                    {
                                        putc (*p, fp);
                                        p++;
                                    }
                                }

                                upload_successful = true;
                                fclose (fp);
                            }
                            else
                            {
                                reason = REASON_CANNOT_OPEN_FILE;
                            }
                        }
                        else
                        {
                            reason = REASON_INVALID_FILENAME;
                        }
                    }
                    else
                    {
                        fname = (char *) 0;
                    }
                }
            }
        }
    }

    if (upload_successful)
    {
        HTTP::response += (String) "<font color='darkgreen'>Upload erfolgreich.</font><BR>\r\n";

        if (fname && *fname)
        {
            HTTP::response += (String) "<font color='darkgreen'>Dateiname: " + fname + "</font>\r\n";
        }
    }
    else
    {
        HTTP::response += (String) "<font color='red'>Upload fehlgeschlagen. ";

        if (reason == REASON_INVALID_FILENAME)
        {
            if (fname && *fname)
            {
                HTTP::response += (String) "Ung&uuml;ltiger Dateiname.<BR>\r\n";
            }
            else
            {
                HTTP::response += (String) "Keine Datei angegeben.<BR>\r\n";
            }
        }
        else if (reason == REASON_CANNOT_OPEN_FILE)
        {
            HTTP::response += (String) "Die Datei konnte nicht ge&oouml;ffnet werden.\r\n";
        }

        HTTP::response += (String) "</font><BR>\r\n";

        if (fname && *fname)
        {
            HTTP::response += (String) "Dateiname: " + fname + "<BR>\r\n";
        }
    }

    HTTP::response += (String) "<P>\r\n";
    show_directory (action, url);
    print_upload_form ();

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_net ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_net (void)
{
    String    title   = "Network";
    String    url     = "/net";
    bool      connect = false;
    bool      ap      = false;
    const char * msg  = (char *) 0;

    const char *    action = HTTP::parameter ("action");
    // int state = atoi (server.arg("state"));

    if (strcmp (action, "connect") == 0)
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "todo: handle_net(): connect\n");
        connect = true;
    }
    else if (strcmp (action, "ap") == 0)
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "todo: handle_net(): ap\n");
        ap = true;
    }

    HTTP_Common::html_header (title, title, url, true);               // save SSIDs and KEYs in UTF-8
    HTTP::response += (String) "<div style='margin-left:20px;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    if (connect)
    {
        HTTP::response += (String) "<P><B>Connecting, please try again later...</B>\r\n";
    }
    else if (ap)
    {
        HTTP::response += (String) "<P><B>Starting as AP, please try again later...</B>\r\n";
    }
    else
    {
        HTTP::response += (String) "handle_net(): todo\r\n";

        if (msg)
        {
            HTTP::response += (String) "<BR><font color='red'>" + msg + "</font>\r\n";
        }
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_upl ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_upl (void)
{
    String          title   = "Upload Hex";
    String          url     = "/upl";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<P>\r\n";
    HTTP::response += (String) "<div style='margin-top:10px;margin-bottom:10px;margin-left:20px;padding:10px;border:1px lightgray solid;display:inline-block;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);
    show_directory (action, url);
    print_upload_form ();
    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_flash ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_flash (void)
{
    String          title   = "Flash STM32";
    String          url     = "/flash";
    const char *    action  = HTTP::parameter ("action");

    HTTP_Common::html_header (title, title, url, true);
    HTTP::response += (String) "<P><div style='margin-top:10px;margin-bottom:10px;margin-left:20px;padding:10px;border:1px lightgray solid;display:inline-block;'>\r\n";
    HTTP_Common::add_action_handler ("head", "", 200, true);

    show_directory (action, url);

    if (strcmp (action, "flash") == 0)
    {
        const char *    fname = HTTP::parameter ("fname"); // todo

        HTTP::response += (String) "<BR>\r\n";
        STM32::flash_from_local (fname);
        HTTP::flush ();
    }
    else
    {
        if (strcmp (action, "reset") == 0)
        {
            STM32::reset ();
        }

        HTTP::response += (String)
            "<form method='GET' action='" + url + "'>\r\n"
            "<P><button type='submit' name='action' value='reset'>Reset STM32</button>\r\n"
            "</form>\r\n";
    }

    HTTP::response += (String) "</div>\r\n";
    HTTP_Common::html_trailer ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_action ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_action (void)
{
    const char *    action = HTTP::parameter ("action");

    Debug::printf (DEBUG_LEVEL_VERBOSE, "handle_action: action=%s\n", action);

    HTTP::response = "";

    if (strcmp (action, "estop") == 0)
    {
        Locos::estop ();
    }
    else if (strcmp (action, "on") == 0)
    {
        HTTP_Common::action_on ();
    }
    else if (strcmp (action, "off") == 0)
    {
        HTTP_Common::action_off ();
    }
    else if (strcmp (action, "rstalert") == 0)
    {
        HTTP::set_alert ("");
    }
    else if (strcmp (action, "head") == 0)
    {
        HTTP_Common::head_action ();
    }
    else if (strcmp (action, "locos") == 0)
    {
        HTTP_Loco::action_locos ();
    }
    else if (strcmp (action, "led") == 0)
    {
        HTTP_Led::action_led ();
    }
    else if (strcmp (action, "rr") == 0)
    {
        HTTP_Railroad::action_rr ();
    }
    else if (strcmp (action, "s88") == 0)
    {
        HTTP_S88::action_s88 ();
    }
    else if (strcmp (action, "rcl") == 0)
    {
        HTTP_RCL::action_rcl ();
    }
    else if (strcmp (action, "macro") == 0)
    {
        HTTP_Loco::action_macro ();
    }
    else if (strcmp (action, "setspeed") == 0)
    {
        HTTP_Loco::action_setspeed ();
    }
    else if (strcmp (action, "go") == 0)
    {
        HTTP_Common::action_go ();
    }
    else if (strcmp (action, "togglefunction") == 0)
    {
        HTTP_Loco::action_togglefunction ();
    }
    else if (strcmp (action, "togglefunctionaddon") == 0)
    {
        HTTP_AddOn::action_togglefunctionaddon ();
    }
    else if (strcmp (action, "setdestination") == 0)
    {
        HTTP_Loco::action_setdestination ();
    }
    else if (strcmp (action, "loco") == 0)
    {
        HTTP_Loco::action_loco ();
    }
    else if (strcmp (action, "getf") == 0)
    {
        HTTP_Loco::action_getf ();
    }
    else if (strcmp (action, "getfa") == 0)
    {
        HTTP_AddOn::action_getfa ();
    }
    else if (strcmp (action, "setled") == 0)
    {
        HTTP_Led::action_setled ();
    }
    else if (strcmp (action, "setsw") == 0)
    {
        HTTP_Switch::action_setsw ();
    }
    else if (strcmp (action, "switch") == 0)
    {
        HTTP_Switch::action_switch ();
    }
    else if (strcmp (action, "setsig") == 0)
    {
        HTTP_Signal::action_setsig ();
    }
    else if (strcmp (action, "sig") == 0)
    {
        HTTP_Signal::action_sig ();
    }
    else if (strcmp (action, "rrset") == 0)
    {
        HTTP_Railroad::action_rrset ();
    }
    else if (strcmp (action, "setcondmapesu") == 0)
    {
        HTTP_POMMAP::action_setcondmapesu ();
    }
    else if (strcmp (action, "setoutputmapesu") == 0)
    {
        HTTP_POMMAP::action_setoutputmapesu ();
    }
    else if (strcmp (action, "setoutputmaplenz") == 0)
    {
        HTTP_POMMAP::action_setoutputmaplenz ();
    }
    else if (strcmp (action, "setoutputmapzimo") == 0)
    {
        HTTP_POMMAP::action_setoutputmapzimo ();
    }
    else if (strcmp (action, "setoutputmaptams") == 0)
    {
        HTTP_POMMAP::action_setoutputmaptams ();
    }
    else if (strcmp (action, "savemapesu") == 0)
    {
        HTTP_POMMAP::action_savemapesu ();
    }
    else if (strcmp (action, "savemaplenz") == 0)
    {
        HTTP_POMMAP::action_savemaplenz ();
    }
    else if (strcmp (action, "savemapzimo") == 0)
    {
        HTTP_POMMAP::action_savemapzimo ();
    }
    else if (strcmp (action, "savemaptams") == 0)
    {
        HTTP_POMMAP::action_savemaptams ();
    }
    else if (strcmp (action, "setoutputesu") == 0)
    {
        HTTP_POMOUT::action_setoutputesu ();
    }
    else if (strcmp (action, "saveoutputesu") == 0)
    {
        HTTP_POMOUT::action_saveoutputesu ();
    }
    else
    {
        HTTP::response = (String) "unkown action: " + action;
        printf ("unknown action: %s\r\n", action);
    }

    http_puts (HTTP::response);
}

static void
print_style_hide (void)
{
    HTTP::response += (String)
        "button, input[type=button], input[type=submit], input[type=reset] { background-color: #EEEEEE; border: 1px solid #AAAAEE; }\r\n"
        "button:hover, input[type=button]:hover, input[type=submit]:hover, input[type=reset] { background-color: #DDDDDD; }\r\n"
        "@media screen and (max-width: 1000px) { .hide1000 { display:none; }}\r\n"
        "@media screen and (max-width: 1200px) { .hide1200 { display:none; }}\r\n"
        "@media screen and (max-width: 1800px) { .hide1800 { display:none; }}\r\n";
}

static void
print_iframe_buttons (void)
{
    HTTP::response += (String)
        "<script>\r\n"
        "function estop()"
        "{"
        "  var http = new XMLHttpRequest(); http.open ('GET', '/action?action=estop'); http.send (null);"
        "  document.getElementById('estop').style.color = 'white';"
        "  document.getElementById('estop').style.backgroundColor = 'red';"
        "  setTimeout(function()"
        "  {"
        "    document.getElementById('estop').style.color = '';"
        "    document.getElementById('estop').style.backgroundColor = '';"
        "  }, 500);"
        "}\r\n"
        "window.onkeyup = function (event) { if (event.keyCode == 27) { estop(); }}\r\n"
        "</script>\r\n"
        "<button class='estop' id='estop' onclick='estop()'>STOP</font></button><BR>\r\n"
        "<button onclick=\"window.location.href='/';\">1</button>\r\n"
        "<button class='hide1200' onclick=\"window.location.href='/2';\">2</button>\r\n"
        "<button class='hide1800' onclick=\"window.location.href='/3';\">3</button>\r\n"
        "<button class='hide1200' onclick=\"window.location.href='/4';\">4</button>\r\n"
        "<button class='hide1800' onclick=\"window.location.href='/6';\">6</button>\r\n";
}

static void
print_iframe_x_header_pre (void)
{
    HTTP::response = (String)
        "<!DOCTYPE html>\r\n"
        "<html>\r\n"
        "<head>\r\n"
        "<meta charset='UTF-8'>"
        "<title>DCC FM22</title>\r\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'/>\r\n";
}

static void
print_iframe_x_header_post (void)
{
    HTTP::response += (String)
        "</head>\r\n";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_iframe2 ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_iframe2 (void)
{
    print_iframe_x_header_pre ();
    HTTP::response += (String)
        "<style>\r\n"
        "div { height:100%; width:100%; }"
        "span { display:inline-block; width:50%; }"
        "iframe { height:96vh; width:100%; border: 1px dotted gray; }"
        ".estop { width:100%; height:32px; color:red; }\r\n"
        ".estop:hover { color:white; background-color:red; }\r\n";
    print_style_hide ();
    HTTP::response += (String)
        "</style>\r\n";
    print_iframe_x_header_post ();
    print_iframe_buttons ();

    HTTP::response += (String)
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "</body></html>\r\n";

    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_iframe3 ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_iframe3 (void)
{
    print_iframe_x_header_pre ();
    HTTP::response += (String)
        "<style>\r\n"
        "div { height:100%; width:100%; }"
        "span { display:inline-block; width:33%; }"
        "iframe { height:96vh; width:100%; border: 1px dotted gray; }"
        ".estop { width:100%; height:32px; color:red; }\r\n"
        ".estop:hover { color:white; background-color:red; }\r\n";
    print_style_hide ();
    HTTP::response += (String)
        "</style>\r\n";
    print_iframe_x_header_post ();
    print_iframe_buttons ();

    HTTP::response += (String)
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "</body></html>\r\n";

    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_iframe4 ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_iframe4 (void)
{
    print_iframe_x_header_pre ();
    HTTP::response += (String)
        "<style>\r\n"
        "div { height:50%; width:100%; }"
        "span { display:inline-block; width:50%; }"
        "iframe { height:48vh; width:100%; border: 1px dotted gray; }"
        ".estop { width:100%; height:32px; color:red; }\r\n"
        ".estop:hover { color:white; background-color:red; }\r\n";
    print_style_hide ();
    HTTP::response += (String)
        "</style>\r\n";
    print_iframe_x_header_post ();
    print_iframe_buttons ();

    HTTP::response += (String)
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "</body></html>\r\n";

    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_iframe6 ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_iframe6 (void)
{
    print_iframe_x_header_pre ();
    HTTP::response += (String)
        "<style>\r\n"
        "div { height:50%; width:100%; }"
        "span { display:inline-block; width:33%; }"
        "iframe { height:48vh; width:100%; border: 1px dotted gray; }"
        ".estop { width:100%; height:32px; color:red; }\r\n"
        ".estop:hover { color:white; background-color:red; }\r\n";
    print_style_hide ();
    HTTP::response += (String)
        "</style>\r\n";
    print_iframe_x_header_post ();
    print_iframe_buttons ();

    HTTP::response += (String)
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "<div><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span><span><iframe src='/'></iframe></span></div>"
        "</body></html>\r\n";

    HTTP::flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * handle_nothing ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
handle_nothing (void)
{
    String    title   = "Error";
    String    url     = request_file;

    HTTP_Common::html_header (title, title, url, true);

    HTTP::response += (String)
        "<P>\r\n"
        "<div style='margin-top:10px;margin-bottom:10px;margin-left:20px;padding:10px;border:1px lightgray solid;display:inline-block;'>\r\n"
        "Invalid page: " + request_file +
        "</div>\r\n";

    HTTP_Common::html_trailer ();
}

typedef struct
{
    const char *    url;
    void            (* func) (void);
} PAGEENTRY;

static PAGEENTRY    pageentry[] =
{
    { "/",          HTTP_Loco::handle_loco              },
    { "/loco",      HTTP_Loco::handle_loco              },
    { "/addon",     HTTP_AddOn::handle_addon            },
    { "/led",       HTTP_Led::handle_led                },
    { "/switch",    HTTP_Switch::handle_switch          },
    { "/rr",        HTTP_Railroad::handle_rr            },
    { "/rredit",    HTTP_Railroad::handle_rr_edit       },
    { "/swtest",    HTTP_Switch::handle_switch_test     },
    { "/sig",       HTTP_Signal::handle_sig             },
    { "/sigtest",   HTTP_Signal::handle_sig_test        },
    { "/s88",       HTTP_S88::handle_s88,               },
    { "/s88edit",   HTTP_S88::handle_s88_edit           },
    { "/lmedit",    HTTP_Loco::handle_loco_macro_edit   },
    { "/rcl",       HTTP_RCL::handle_rcl                },
    { "/rcledit",   HTTP_RCL::handle_rcl_edit           },
    { "/pgminfo",   HTTP_PGM::handle_pgminfo            },
    { "/pgmaddr",   HTTP_PGM::handle_pgmaddr            },
    { "/pgmcv",     HTTP_PGM::handle_pgmcv              },
    { "/pominfo",   HTTP_POM::handle_pominfo            },
    { "/pomaddr",   HTTP_POM::handle_pomaddr            },
    { "/pomcv",     HTTP_POM::handle_pomcv              },
    { "/pommot",    HTTP_POMMOT::handle_pommot          },
    { "/pommap",    HTTP_POMMAP::handle_pommap          },
    { "/pomout",    HTTP_POMOUT::handle_pomout          },
    { "/info",      HTTP_Common::handle_info            },
    { "/setup",     HTTP_Common::handle_setup           },
    { "/net",       handle_net                          },
    { "/upl",       handle_upl                          },
    { "/flash",     handle_flash                        },
    { "/doupload",  handle_doupload                     },
    { "/action",    handle_action                       },
    { "/2",         handle_iframe2                      },
    { "/3",         handle_iframe3                      },
    { "/4",         handle_iframe4                      },
    { "/6",         handle_iframe6                      }
};

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_page ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_page (void)
{
    uint_fast16_t   npages = sizeof (pageentry) / sizeof (PAGEENTRY);
    uint_fast16_t   idx;

    for (idx = 0; idx < npages; idx++)
    {
        if (! strcmp (request_file, pageentry[idx].url))
        {
            (*pageentry[idx].func) ();
            break;
        }
    }

    if (idx == npages)
    {
        handle_nothing ();
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_exec ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_exec (void)
{
    int     in_par_name     = 0;
    int     in_par_value    = 0;
    int     par_idx         = -1;
    int     offset          = 0;
    int     method;
    int     rtc;

    boundary = (char *) 0;
    n_parameters = 0;

    request_file = (char *) NULL;

    rtc = http_request ();

    if (rtc > 0)
    {
        method = METHOD_NONE;

        if (! strncmp (request_buf, "GET ", 4))
        {
            method = METHOD_GET;
            offset = 4;
        }
        else if (! strncmp (request_buf, "POST ", 5))
        {
            char *  p;
            int     len = 0;

            Debug::printf (DEBUG_LEVEL_VERBOSE, "http_exec: post: request_buf:\n%s\n", request_buf);

            p = strcasestr (request_buf, "Content-Length: ");

            if (p)
            {
                len = atoi (p + 16);
            }

            boundary = strcasestr (request_buf, "Content-Type: multipart/form-data; boundary=");

            if (boundary)
            {
                boundary += 44;

                p = strchr (boundary, '\r');

                if (p)
                {
                    *p = '\0';
                }
                else
                {
                    p = strchr (boundary, '\n');

                    if (p)
                    {
                        *p = '\0';
                    }
                    else
                    {
                        boundary = (char *) 0;
                    }
                }
            }

            if (boundary)
            {
                method = METHOD_POST_MULTI;
            }
            else
            {
                method = METHOD_POST;
            }

            offset = 5;

            http_post (len);
        }

        if (method == METHOD_GET || method == METHOD_POST)
        {
            char * p = request_buf + offset;

            request_file = p;

            while (*p && *p != ' ' && *p != '\r' && *p != '\n')
            {
                if (in_par_name)
                {
                    if (*p == '=')
                    {
                        *p = '\0';
                        request_parameter_value[par_idx] = p + 1;
                        in_par_name = 0;
                        in_par_value = 1;
                    }
                }
                else if (in_par_value)
                {
                    if (*p == '&')
                    {
                        *p = '\0';
                        par_idx++;

                        if (par_idx >= MAX_PARAMETERS)
                        {
                            Debug::printf (DEBUG_LEVEL_NONE, "Fatal: maximum number of parameters (%d) reached\n", MAX_PARAMETERS);
                            exit (1);
                        }

                        request_parameter_name[par_idx] = p + 1;
                        in_par_name = 1;
                        in_par_value = 0;
                    }
                }
                else if (*p == '?')
                {
                    if (method == METHOD_POST)
                    {
                        break;
                    }

                    *p = '\0';
                    par_idx++;

                    if (par_idx >= MAX_PARAMETERS)
                    {
                        Debug::printf (DEBUG_LEVEL_NONE, "Fatal: maximum number of parameters (%d) reached\n", MAX_PARAMETERS);
                        exit (1);
                    }

                    in_par_name = 1;
                    in_par_value = 0;
                    request_parameter_name[par_idx] = p + 1;
                }
                p++;
            }

            *p = '\0';
        }
        
        if (method == METHOD_POST)
        {
            char * p = post_buf;

            in_par_name = 1;
            in_par_value = 1;
            par_idx = 0;
            request_parameter_name[par_idx] = p;

            while (*p && *p != ' ' && *p != '\r' && *p != '\n')
            {
                if (in_par_name)
                {
                    if (*p == '=')
                    {
                        *p = '\0';
                        request_parameter_value[par_idx] = p + 1;
                        in_par_name = 0;
                        in_par_value = 1;
                    }
                }
                else if (in_par_value)
                {
                    if (*p == '&')
                    {
                        *p = '\0';
                        par_idx++;

                        if (par_idx >= MAX_PARAMETERS)
                        {
                            Debug::printf (DEBUG_LEVEL_NONE, "Fatal: maximum number of parameters (%d) reached\n", MAX_PARAMETERS);
                            exit (1);
                        }

                        request_parameter_name[par_idx] = p + 1;
                        in_par_name = 1;
                        in_par_value = 0;
                    }
                }
                p++;
            }
        }
        else if (method == METHOD_POST_MULTI)
        {
            Debug::printf (DEBUG_LEVEL_VERBOSE, "http_exec: post_buf:\n%s\n", post_buf);
            char * p = request_buf + offset;
            request_file = p;
            p = strchr (request_file, ' ');

            if (p)
            {
                *p = '\0';

                p = strstr (post_buf, "\r\n\r\n");

                if (p)
                {
                    *p = '\0';
                    p += 4;
                    request_parameter_name[0] = post_buf;
                    request_parameter_value[0] = p;
                    par_idx = 0;

                    Debug::printf (DEBUG_LEVEL_VERBOSE, "http_exec: request_parameter_name[0]:\n%s\n", request_parameter_name[0]);
                    Debug::printf (DEBUG_LEVEL_VERBOSE, "http_exec: request_parameter_value[0]:\n%s\n", request_parameter_value[0]);
                }
            }
            else
            {
                request_file = (char *) 0;
            }
        }

        if (request_file)
        {
            Debug::printf (DEBUG_LEVEL_VERBOSE, "http_exec: request_file=%s\n", request_file);
            n_parameters = par_idx + 1;

            http_normalize (request_file);

            if (method == METHOD_GET || method == METHOD_POST)
            {
                for (par_idx = 0; par_idx < n_parameters; par_idx++)
                {
                    http_normalize (request_parameter_name[par_idx]);
                    http_normalize (request_parameter_value[par_idx]);
                }
            }

            http_header ();
            http_page ();
        }
        else
        {
            Debug::printf (DEBUG_LEVEL_NORMAL, "no GET or POST statement\n");
            puts (request_buf);
        }
    }
    else if (rtc < 0)
    {
        Debug::printf (DEBUG_LEVEL_NORMAL, "http_request error\n");
    }
}

void
HTTP::set_alert (const char * msg)
{
    HTTP_Common::alert_msg = msg;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_server ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
bool
HTTP::server (bool edit)
{
    http_fd = accept_port (100);                    // real daemon: timeout = 100 usec = 1/10000 sec

    HTTP_Common::edit_mode = edit;

    if (http_fd > 0)
    {
        http_exec ();
        (void) close (http_fd);
        http_fd = 0;
    }

    return HTTP_Common::edit_mode;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_send ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP::send (const char * str)
{
    HTTP::response += (String) str;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http_flush ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP::flush (void)
{
    http_puts (HTTP::response);
    HTTP::response = "";
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * init ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP::init (void)
{
    int listen_port = 9999;
    
    signal (SIGPIPE, SIG_IGN);

    if (bind_listen_port (listen_port) < 0)
    {
        perror ("bind_listen_port");
        exit (1);
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * deinit ()
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
HTTP::deinit (void)
{
    close (sock_fd);
}
