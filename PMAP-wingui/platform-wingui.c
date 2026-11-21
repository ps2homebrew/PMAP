#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>
#include <time.h>
#include <ctype.h>

#include "platform.h"
#include "mecha.h"

extern HWND g_mainWin;

static HANDLE ComPortHandle = INVALID_HANDLE_VALUE;
static unsigned short RxTimeout;
static FILE *DebugOutputFile = NULL;

/* void ListSerialDevices()
{
    char targetPath[256];
    char dosDeviceName[256];
    char comPortName[] = "COM";

    for (int portNum = 1; portNum <= 256; portNum++)
    {
        snprintf(dosDeviceName, sizeof(dosDeviceName), "%s%d", comPortName, portNum);

        if (QueryDosDevice(dosDeviceName, targetPath, sizeof(targetPath)) != 0)
        {
            PlatShowMessage("Found COM Port: %s\n", dosDeviceName);
            PlatShowMessage("Target Path: %s\n", targetPath);
        }
    }
} */

int PlatOpenCOMPort(const char *device)
{
    // ListSerialDevices();
    COMMTIMEOUTS CommTimeout;
    DCB DeviceControlBlock;
    char devicePath[64];
    int result;

    // COM10 and above must be opened through the "\\.\COMxx" namespace; the bare
    // "COMxx" name only resolves for COM1-COM9, which is why CreateFileA("COM10")
    // fails. Prefixing works for every port number, so always use it (unless the
    // caller already passed a full path).
    if (device[0] == '\\')
        snprintf(devicePath, sizeof(devicePath), "%s", device);
    else
        snprintf(devicePath, sizeof(devicePath), "\\\\.\\%s", device);

    if (ComPortHandle == INVALID_HANDLE_VALUE)
    {
        if ((ComPortHandle = CreateFileA(devicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
        {
            memset(&DeviceControlBlock, 0, sizeof(DeviceControlBlock));
            DeviceControlBlock.DCBlength = sizeof(DCB);
            GetCommState(ComPortHandle, &DeviceControlBlock);
            // Fully specify the line settings instead of trusting whatever state
            // the port was left in. fParity only disables parity *checking*; the
            // Parity field decides whether a parity bit is actually used, so it
            // must be forced to NOPARITY or the link can silently run 8E1/8O1.
            DeviceControlBlock.BaudRate        = CBR_57600;
            DeviceControlBlock.ByteSize        = 8;
            DeviceControlBlock.Parity          = NOPARITY;
            DeviceControlBlock.StopBits        = ONESTOPBIT;
            DeviceControlBlock.fBinary         = TRUE;
            DeviceControlBlock.fParity         = FALSE;
            DeviceControlBlock.fOutxCtsFlow    = FALSE;
            DeviceControlBlock.fOutxDsrFlow    = FALSE;
            DeviceControlBlock.fDtrControl     = DTR_CONTROL_ENABLE;
            DeviceControlBlock.fDsrSensitivity = FALSE;
            DeviceControlBlock.fTXContinueOnXoff = FALSE;
            DeviceControlBlock.fOutX           = FALSE;
            DeviceControlBlock.fInX            = FALSE;
            DeviceControlBlock.fErrorChar      = FALSE;
            DeviceControlBlock.fNull           = FALSE;
            DeviceControlBlock.fRtsControl     = RTS_CONTROL_ENABLE;
            DeviceControlBlock.fAbortOnError   = FALSE;
            if (!SetCommState(ComPortHandle, &DeviceControlBlock))
                PlatShowMessage("SetCommState failed: %lu\n", (unsigned long)GetLastError());

            // Log what the driver actually applied, so a mismatch is visible.
            {
                DCB applied;
                memset(&applied, 0, sizeof(applied));
                applied.DCBlength = sizeof(applied);
                if (GetCommState(ComPortHandle, &applied))
                    PlatDPrintf("DCB applied: baud=%lu bytesize=%u parity=%u stopbits=%u fParity=%d fOutxCts=%d fRts=%d fDtr=%d fOutX=%d fInX=%d\n",
                                (unsigned long)applied.BaudRate, applied.ByteSize, applied.Parity, applied.StopBits,
                                applied.fParity, applied.fOutxCtsFlow, applied.fRtsControl, applied.fDtrControl, applied.fOutX, applied.fInX);
            }
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
        {
            result = ENXIO;
            PlatDPrintf("PlatOpenCOMPort: CreateFileA(%s) failed (GetLastError=%lu)\n", devicePath, (unsigned long)GetLastError());
        }
    }
    else
        result = EMFILE;

    if (result != 0)
        PlatCloseCOMPort();

    PlatDPrintf("PlatOpenCOMPort: device=%s (%s) baud=57600 8N1 result=%d\n", device, devicePath, result);

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
    {
        result = -EIO;
        PlatDPrintf("PlatReadCOMPort: ReadFile failed (GetLastError=%lu)\n", (unsigned long)GetLastError());
    }

    return result;
}

int PlatWriteCOMPort(const char *data)
{
    DWORD BytesWritten;
    int result;

    if (WriteFile(ComPortHandle, data, strlen(data), &BytesWritten, NULL) == TRUE)
        result = BytesWritten;
    else
    {
        result = -EIO;
        PlatDPrintf("PlatWriteCOMPort: WriteFile failed (GetLastError=%lu)\n", (unsigned long)GetLastError());
    }

    if (result < 0)
    {
        PlatShowMessage("Write to COM port failed.\n");
    }

    return result;
}

void PlatCloseCOMPort(void)
{
    if (ComPortHandle != INVALID_HANDLE_VALUE)
    {
        PlatShowMessage("Closing COM port...\n");
        CloseHandle(ComPortHandle);
        ComPortHandle = INVALID_HANDLE_VALUE;
        PlatShowMessage("COM port closed.\n");
    }
    else
    {
        PlatShowMessage("COM port is already closed.\n");
    }
}

void PlatSleep(unsigned short int msec)
{
    Sleep(msec);
}

void PlatShowEMessage(const char *format, ...)
{
    char buffer[256];
    if (format == NULL)
    {
        fprintf(stderr, "Error: format string is NULL\n");
        return; // Exit early if format is NULL
    }

    va_list args;

    // Print to standard output
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args); // Clean up args after vsnprintf

    if (DebugOutputFile != NULL)
    {
        va_start(args, format); // Reinitialize args for vfprintf
        vfprintf(DebugOutputFile, format, args);
        va_end(args); // Clean up args after vfprintf
    }

    MessageBoxA(g_mainWin, buffer, "Error", MB_OK | MB_ICONERROR);
}

void PlatShowMessage(const char *format, ...)
{
    char buffer[256];
    if (format == NULL)
    {
        fprintf(stderr, "Error: format string is NULL\n");
        return; // Exit early if format is NULL
    }

    va_list args;

    // Print to standard output
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args); // Clean up args after vsnprintf

    if (DebugOutputFile != NULL)
    {
        va_start(args, format); // Reinitialize args for vfprintf
        vfprintf(DebugOutputFile, format, args);
        va_end(args); // Clean up args after vfprintf
    }

    MessageBoxA(g_mainWin, buffer, "Information", MB_OK | MB_ICONINFORMATION);
}

void PlatShowMessageB(const char *format, ...)
{
    char buffer[256];
    if (format == NULL)
    {
        fprintf(stderr, "Error: format string is NULL\n");
        return; // Exit early if format is NULL
    }

    va_list args;

    // Print to standard output
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args); // Clean up args after vsnprintf

    if (DebugOutputFile != NULL)
    {
        va_start(args, format); // Reinitialize args for vfprintf
        vfprintf(DebugOutputFile, format, args);
        va_end(args); // Clean up args after vfprintf
    }

    MessageBoxA(g_mainWin, buffer, "Information", MB_OK | MB_ICONINFORMATION);
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
    if (format == NULL)
    {
        fprintf(stderr, "Error: format string is NULL\n");
        return; // Exit early if format is NULL
    }

    va_list args;

    // Print to standard output
    va_start(args, format);
    if (DebugOutputFile != NULL)
    {
        vfprintf(DebugOutputFile, format, args);
        fflush(DebugOutputFile); // Flush immediately so the log survives a hang/crash during a comms failure.
    }
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
