#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>
#include <time.h>
#include <ctype.h>

#include "platform.h"
#include "mecha.h"

static HANDLE ComPortHandle = INVALID_HANDLE_VALUE;
static unsigned short RxTimeout;
static FILE *DebugOutputFile = NULL;

void ListSerialDevices()
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(L"COM*", &findFileData); // Use the wide-character version of FindFirstFile

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("No COM ports found.\n");
        return;
    }

    printf("Available serial devices:\n");

    do
    {
        char comPortName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, findFileData.cFileName, -1, comPortName, sizeof(comPortName), NULL, NULL); // Convert from wide to ANSI/ASCII
        printf("COM%s\n", comPortName);
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int PlatOpenCOMPort(const char *device)
{
    ListSerialDevices();
    COMMTIMEOUTS CommTimeout;
    DCB DeviceControlBlock;
    int result;

    if (ComPortHandle == INVALID_HANDLE_VALUE)
    {
        if ((ComPortHandle = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
        {
            memset(&DeviceControlBlock, 0, sizeof(DeviceControlBlock));
            DeviceControlBlock.DCBlength = sizeof(DCB);
            GetCommState(ComPortHandle, &DeviceControlBlock);
            DeviceControlBlock.BaudRate = CBR_57600;
            DeviceControlBlock.fParity  = FALSE;
            DeviceControlBlock.ByteSize = 8;
            DeviceControlBlock.StopBits = ONESTOPBIT;
            SetCommState(ComPortHandle, &DeviceControlBlock);
            CommTimeout.ReadIntervalTimeout        = 0;
            CommTimeout.ReadTotalTimeoutMultiplier = 0;
            CommTimeout.ReadTotalTimeoutConstant = RxTimeout = MECHA_TASK_NORMAL_TO;
            CommTimeout.WriteTotalTimeoutConstant            = 0;
            CommTimeout.WriteTotalTimeoutMultiplier          = 0;
            SetCommTimeouts(ComPortHandle, &CommTimeout);
            PurgeComm(ComPortHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
            result = 0;
        }
        else
            result = ENXIO;
    }
    else
        result = EMFILE;

    if (result != 0)
        PlatCloseCOMPort();

    return result;
}

int PlatReadCOMPort(char *data, int n, unsigned short timeout)
{
    COMMTIMEOUTS CommTimeout;
    DWORD BytesRead;
    int result;

    if (RxTimeout != timeout)
    {
        CommTimeout.ReadIntervalTimeout        = 0;
        CommTimeout.ReadTotalTimeoutMultiplier = 0;
        CommTimeout.ReadTotalTimeoutConstant = RxTimeout = timeout;
        CommTimeout.WriteTotalTimeoutConstant            = 0;
        CommTimeout.WriteTotalTimeoutMultiplier          = 0;
        SetCommTimeouts(ComPortHandle, &CommTimeout);
    }
    if (ReadFile(ComPortHandle, data, n, &BytesRead, NULL) == TRUE)
        result = BytesRead;
    else
        result = -EIO;

    return result;
}

int PlatWriteCOMPort(const char *data)
{
    DWORD BytesWritten;
    int result;

    if (WriteFile(ComPortHandle, data, strlen(data), &BytesWritten, NULL) == TRUE)
        result = BytesWritten;
    else
        result = -EIO;

    if (result < 0)
    {
        printf("Write to COM port failed.\n");
    }

    return result;
}

void PlatCloseCOMPort(void)
{
    if (ComPortHandle != INVALID_HANDLE_VALUE)
    {
        printf("Closing COM port...\n");
        CloseHandle(ComPortHandle);
        ComPortHandle = INVALID_HANDLE_VALUE;
        printf("COM port closed.\n");
    }
    else
    {
        printf("COM port is already closed.\n");
    }
}

void PlatSleep(unsigned short int msec)
{
    Sleep(msec);
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
