#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

#include "platform.h"
#include "mecha.h"

static int ComPortFD = -1;
static unsigned short RxTimeout;
static struct termios OldTio;

int PlatformOpenCOMPort(const char *device)
{
    struct termios tio;
    int result;

    if (ComPortFD < 0)
    {
        if ((ComPortFD = open(device, O_RDWR | O_NOCTTY)) >= 0)
        {
            tcgetattr(ComPortFD, &OldTio);
            memset(&tio, 0, sizeof(tio));

            tio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
            tio.c_lflag = ICANON;

            tcflush(ComPortFD, TCIFLUSH);
            tcsetattr(ComPortFD, TCSANOW, &tio);
            result = 0;
        }
        else
            result = ComPortFD;
    }
    else
        result = EMFILE;

    return result;
}

int PlatformReadCOMPort(char *data, int n, unsigned short timeout)
{
    struct timeval SelTimeout;
    fd_set RdSet;

    FD_ZERO(&RdSet);
    FD_SET(ComPortFD, &RdSet);
    SelTimeout.tv_sec  = 0;
    SelTimeout.tv_usec = timeout * 1000;
    if (select(ComPortFD + 1, &RdSet, NULL, NULL, &SelTimeout) == 1)
        return read(ComPortFD, data, n);
    else
        return -1;
}

int PlatformWriteCOMPort(const char *data)
{
    return write(ComPortFD, data, strlen(data));
}

void PlatformCloseCOMPort(void)
{
    tcsetattr(ComPortFD, TCSANOW, &OldTio);
    close(ComPortFD);
    ComPortFD = -1;
}

void PlatformSleep(unsigned short int msec)
{
    usleep((unsigned int)msec * 1000);
}

void PlatformEPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int pstricmp(const char *s1, const char *s2)
{
    char s1char, s2char;

    for (s1char = *s1, s2char = *s2; *s1 != '\0' && *s2 != '\0'; s1++, s2++, s1char = *s1, s2char = *s2)
    {
        if (isalpha(s1char))
            s1char = toupper(s1char);
        if (isalpha(s2char))
            s2char = toupper(s2char);
        if (s1char != s2char)
            break;
    }

    return (s1char - s2char);
}

int pstrincmp(const char *s1, const char *s2, int len)
{
    char s1char, s2char;

    for (s1char = *s1, s2char = *s2; *s1 != '\0' && *s2 != '\0' && len > 0; s1++, s2++, s1char = *s1, s2char = *s2, len--)
    {
        if (isalpha(s1char))
            s1char = toupper(s1char);
        if (isalpha(s2char))
            s2char = toupper(s2char);
        if (s1char != s2char)
            break;
    }

    return ((len == 0) ? 0 : s1char - s2char);
}
