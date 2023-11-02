#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "platform.h"
#include "main.h"
#include "mecha.h"
#include "eeprom.h"
#include "eeprom-id.h"
#include "resource.h"

HINSTANCE g_hInstance;
HWND g_mainWin;

void DisplayConnHelp(void)
{
    MessageBox(g_mainWin, L"There was a problem communicating with the PlayStation 2 console.\n"
                          L"Check the connections, press the RESET button and try again.",
               L"Connection error",
               MB_OK | MB_ICONERROR);
}

static void InitRawConVerInfoMenu(HWND hwnd)
{
    u8 tm, md;
    const struct MechaIdentRaw *RawData;
    char buffer[32];

    MechaGetMode(&tm, &md);
    RawData = MechaGetRawIdent();

    sprintf(buffer, "TestMode.%d MD1.%d", tm, md);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MD_VER), buffer);
    sprintf(buffer, "0x%s", RawData->cfd);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_CFD), buffer);
    sprintf(buffer, "%#08x", RawData->cfc);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_CFC), buffer);
    sprintf(buffer, "%#04x", RawData->VersionID);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_CON_VER), buffer);
}

static INT_PTR CALLBACK ShowRawConVerInfoDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;

    result = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
            InitRawConVerInfoMenu(hwndDlg);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                case IDCLOSE:
                    EndDialog(hwndDlg, TRUE);
                    break;
                default:
                    result = FALSE;
            }
            break;
        default:
            result = FALSE;
    }

    return result;
}

static void ToggleMainDialogControls(HWND hwndDlg, int connected)
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_CONNECT), !connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_PORT), !connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_DISCONNECT), connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_EEP_MAN), connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_MECHA_ADJ), connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_ELECT_ADJ), connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_ID_MAN), connected);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_SHOW_VER), connected);
}

static int ConnectToConsole(HWND hwnd)
{
    char PortName[] = "COM1";
    int result, port = 0;

    if ((port = SendMessage(GetDlgItem(hwnd, IDC_COMBO_PORT), CB_GETCURSEL, 0, 0)) == CB_ERR)
    {
        MessageBox(hwnd, L"Please select the port.", L"Port not selected", MB_OK | MB_ICONERROR);
        return EINVAL;
    }

    PortName[3] = '0' + port + 1;

    if ((result = PlatOpenCOMPort(PortName)) != 0)
        MessageBox(hwnd, L"Cannot open the selected port.", L"Cannot open port", MB_OK | MB_ICONERROR);

    PlatDebugInit();

    return result;
}

static void InitConsoleInfo(HWND hwnd)
{
    const char *ModelName;
    u32 serial;
    u8 emcs, tm, md, status;
    char buffer[32];

    MechaGetMode(&tm, &md);
    sprintf(buffer, "TestMode.%d MD1.%d", tm, md);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MD_VER), buffer);
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MECHACON), MechaGetDesc());
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_CEXDEX), MechaGetCEXDEX() == 0 ? "DEX" : "CEX");

    if (EEPROMInitModelName() == 0)
    {
        ModelName = EEPROMGetModelName();
    }
    else
        ModelName = "<No model name>";
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MODEL_NAME), ModelName);

    if (EEPROMInitSerial() == MECHA_RTC_RICOH)
    {
        EEPROMGetSerial(&serial, &emcs);
        sprintf(buffer, "%07u", serial);
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_SERIAL), buffer);
        sprintf(buffer, "%02x", emcs);
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_EMCS_ID), buffer);
        sprintf(buffer, "%04x", EEPROMGetModelID());
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MODEL_ID), buffer);
    }
    else
    {
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_SERIAL), "<No serial number>");
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_EMCS_ID), "-");
        SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_MODEL_ID), "-");
    }

    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_OP_TYPE), MechaGetOPTypeName(MechaGetOP()));
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_LENS_TYPE), MechaGetLensTypeName(MechaGetLens()));
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_RTC_TYPE), MechaGetRTCName(MechaGetRTCType()));
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATIC_TV_SYS), MechaGetTVSystemDesc(EEPROMGetTVSystem()));

    // System status
    CheckDlgButton(hwnd, IDC_CHECK_CHKSUM_ERR, MechaGetEEPROMStat() ? BST_UNCHECKED : BST_CHECKED);

    status = MechaGetRTCStat();
    if (MechaGetRTCType() == 0)
    {
        CheckDlgButton(hwnd, IDC_CHECK_RTC_LOW_BATT, (status & 0x40) ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHECK_RTC_NO_BATT, (status & 0x10) ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_CHECK_RTC_LOW_BATT, (status & 0x80) ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHECK_RTC_NO_BATT, (status & 0x80) ? BST_CHECKED : BST_UNCHECKED);
    }

    if (EEPROMGetEEPROMStatus() == 1)
        MessageBox(hwnd, L"EEPROM was erased (defaults must now be loaded).",
                   L"Blank EEPROM",
                   MB_ICONINFORMATION | MB_OK);
}

static void InitMainWindow(HWND hwnd)
{
    HWND portCombo;
    short int i;
    wchar_t PortName[] = L"COM1";

    portCombo          = GetDlgItem(hwnd, IDC_COMBO_PORT);
    for (i = 1; i < 10; i++)
    {
        PortName[3] = L'0' + i;
        SendMessage(portCombo, CB_ADDSTRING, 0, (LPARAM)PortName);
    }

    ToggleMainDialogControls(hwnd, FALSE);
}

static INT_PTR CALLBACK MainDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;
    static unsigned char IsConnected = FALSE;

    result                           = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            PlatCloseCOMPort();
            IsConnected = FALSE;
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
            g_mainWin = hwndDlg;
            InitMainWindow(hwndDlg);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_CONNECT:
                    if (ConnectToConsole(hwndDlg) == 0)
                    {
                        if (MechaInitModel() == 0)
                        {
                            IsConnected = TRUE;
                            InitConsoleInfo(hwndDlg);
                            ToggleMainDialogControls(hwndDlg, TRUE);
                            break;
                        }
                        else
                            DisplayConnHelp();
                    }
                    // Otherwise (if failed to connect), fall through.
                case IDC_BUTTON_DISCONNECT:
                    PlatCloseCOMPort();
                    PlatDebugDeinit();
                    IsConnected = FALSE;
                    ToggleMainDialogControls(hwndDlg, FALSE);
                    break;
                case IDC_BUTTON_EEP_MAN:
                    MenuEEPROM();
                    break;
                case IDC_BUTTON_MECHA_ADJ:
                    MenuMECHA();
                    break;
                case IDC_BUTTON_ELECT_ADJ:
                    MenuELECT();
                    break;
#ifdef ID_MANAGEMENT
                case IDC_BUTTON_ID_MAN:
                    MenuID();
                    break;
#endif
                case IDC_BUTTON_SHOW_VER:
                    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_CON_VER), hwndDlg, &ShowRawConVerInfoDlg);
                    break;
                case IDCLOSE:
                    PlatCloseCOMPort();
                    PlatDebugDeinit();
                    IsConnected = FALSE;
                    EndDialog(hwndDlg, TRUE);
                    break;
                default:
                    result = FALSE;
            }
            break;
        default:
            result = FALSE;
    }

    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, &MainDlg);
    return 0;
}
