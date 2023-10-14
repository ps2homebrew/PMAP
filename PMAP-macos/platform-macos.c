#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#include "platform.h"
#include "../base/mecha.h"

static int ComPortHandle = -1;
static unsigned short RxTimeout;
static FILE *DebugOutputFile = NULL;

int PlatOpenCOMPort(const char *device)
{
    struct termios options;
    int result;

    if (ComPortHandle == -1)
    {
        ComPortHandle = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ComPortHandle != -1)
        {
            fcntl(ComPortHandle, F_SETFL, 0);
            tcgetattr(ComPortHandle, &options);
            cfsetispeed(&options, B57600);
            cfsetospeed(&options, B57600);
            options.c_cflag &= ~PARENB; // No parity
            options.c_cflag &= ~CSTOPB; // 1 stop bit
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;                     // 8 data bits
            options.c_cflag &= ~CRTSCTS;                // No hardware flow control
            options.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
            options.c_lflag = 0;
            options.c_oflag = 0;
            tcsetattr(ComPortHandle, TCSANOW, &options);
            tcflush(ComPortHandle, TCIOFLUSH);
            RxTimeout = MECHA_TASK_NORMAL_TO;
            result    = 0;
        }
        else
        {
            result = errno;
        }
    }
    else
    {
        result = EMFILE;
    }

    return result;
}

int PlatReadCOMPort(char *data, int n, unsigned short timeout)
{
    struct timespec ts;
    int result;

    if (RxTimeout != timeout)
        RxTimeout = timeout;

    ts.tv_sec  = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;

    if (pselect(ComPortHandle + 1, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &ts, (const sigset_t *)NULL) > 0)
        result = read(ComPortHandle, data, n);
    else
        result = -EIO;

    return result;
}

int PlatWriteCOMPort(const char *data)
{
    int result = write(ComPortHandle, data, strlen(data));

    return result;
}

void PlatCloseCOMPort(void)
{
    close(ComPortHandle);
    ComPortHandle = -1;
}

void PlatSleep(unsigned short int msec)
{
    usleep((useconds_t)msec * 1000);
}

void PlatShowEMessage(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    if (DebugOutputFile != NULL)
        vfprintf(DebugOutputFile, format, args);
    va_end(args);
}

void PlatShowMessage(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    if (DebugOutputFile != NULL)
        vfprintf(DebugOutputFile, format, args);
    va_end(args);
}

void PlatShowMessageB(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    if (DebugOutputFile != NULL)
        vfprintf(DebugOutputFile, format, args);

    // Block until the user presses ENTER
    while (getchar() != '\n')
    {
    };

    va_end(args);
}

void PlatDebugInit(void)
{
    // Get the current time
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[20]; // Adjust the size according to your needs

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format the timestamp (e.g., "2023-10-14_12-34-56")
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", timeinfo);

    // Create the filename with timestamp
    char filename[256]; // Adjust the size according to your needs
    snprintf(filename, sizeof(filename), "pmap_%s.log", timestamp);

    DebugOutputFile = fopen(filename, "w");
}

void PlatDebugDeinit(void)
{
    if (DebugOutputFile != NULL)
    {
        fclose(DebugOutputFile);
        DebugOutputFile = NULL;
    }
}

void PlatDPrintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (DebugOutputFile != NULL)
        vfprintf(DebugOutputFile, format, args);
    va_end(args);
}

int pstricmp(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2);
}

int pstrincmp(const char *s1, const char *s2, int n)
{
    return strncasecmp(s1, s2, (size_t)n);
}
