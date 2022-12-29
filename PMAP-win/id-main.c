#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <errno.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "eeprom-id.h"
#include "main.h"
#include "resource.h"

extern HINSTANCE g_hInstance;
extern HWND g_mainWin;

extern unsigned char ConType, ConCEXDEX;

static void InitMechacon(void)
{
    int choice, done, dex, NumChoices;

    if (ConType == MECHA_TYPE_40)
    {
        done = 0;
        while (!done)
        {
            do
            {
                printf("MECHACON initialization for H/I-chassis\n"
                       "Select type:\n"
                       "\t1. CEX\n"
                       "\t2. DEX\n"
                       "\t3. Quit\n"
                       "Your choice: ");
                choice = 0;
                if (scanf("%d", &choice) > 0)
                    while (getchar() != '\n')
                    {
                    };
            } while (choice < 1 || choice > 3);

            switch (choice)
            {
                case 1:
                    dex = 0;
                    break;
                case 2:
                    dex = 1;
                    break;
                case 3:
                    done = 1;
            }

            if (!done)
            {
                do
                {
                    if (!dex)
                    {
                        printf("Select model Name:\n"
                               "\t1. SCPH-50000\n"
                               "\t2. SCPH-50001\n"
                               "\t3. SCPH-50002\n"
                               "\t4. SCPH-50003\n"
                               "\t5. SCPH-50004\n"
                               "\t6. SCPH-50005\n"
                               "\t7. SCPH-50006\n"
                               "\t8. SCPH-50007\n"
                               "\t9. SCPH-50008\n"
                               "\t10. SCPH-50009\n"
                               "\t11. SCPH-50010\n"
                               "\t12. Quit\n"
                               "Your choice: ");
                        NumChoices = 12;
                    }
                    else
                    {
                        printf("Select model Name:\n"
                               "\t1. DTL-H50000\n"
                               "\t2. DTL-H50001\n"
                               "\t3. DTL-H50002\n"
                               "\t4. DTL-H50005\n"
                               "\t5. DTL-H50006\n"
                               "\t6. DTL-H50008\n"
                               "\t7. DTL-H50009\n"
                               "\t8. Quit\n"
                               "Your choice: ");
                        NumChoices = 8;
                    }
                    choice = 0;
                    if (scanf("%d", &choice) > 0)
                        while (getchar() != '\n')
                        {
                        };
                } while (choice < 1 || choice > NumChoices);
                if (!dex)
                {
                    switch (choice)
                    {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                            printf("MechaInit: %s\n", MechaInitMechacon(choice, 0) == 0 ? "done" : "failed");
                            break;
                        case 12:
                            done = 1;
                    }
                }
                else
                {
                    switch (choice)
                    {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            printf("MechaInit: %s\n", MechaInitMechacon(choice, 1) == 0 ? "done" : "failed");
                            break;
                        case 8:
                            done = 1;
                    }
                }
            }
        }
    }
    else
        printf("MechaInit: Unsupported chassis.\n");
}

static void InitNTSCPALDefaults(void)
{
    int choice;

    do
    {
        printf("NTSC/PAL selection:\n"
               "\t1. NTSC\n"
               "\t2. PAL\n"
               "\t3. Quit\n"
               "Your choice: ");
        choice = 0;
        if (scanf("%d", &choice) > 0)
            while (getchar() != '\n')
            {
            };
    } while (choice < 1 || choice > 3);

    switch (choice)
    {
        case 1:
            printf("Init NTSC defaults: %s\n", EEPROMNTSCPALDefaults(0) == 0 ? "completed" : "failed");
            break;
        case 2:
            printf("Init PAL defaults: %s\n", EEPROMNTSCPALDefaults(1) == 0 ? "completed" : "failed");
            break;
    }
}

static INT_PTR CALLBACK InitNTSCPALDefsDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;

    result = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
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

static INT_PTR CALLBACK InitMECHADlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;

    result = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
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

static void InitMenu(HWND hwnd)
{
    u8 iLinkID[8], ConsoleID[8];
    char value[3];

    EEPROMGetiLinkID(iLinkID);
    EEPROMGetConsoleID(ConsoleID);

    sprintf(value, "%02x", ConsoleID[0]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_0), value);
    sprintf(value, "%02x", ConsoleID[1]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_1), value);
    sprintf(value, "%02x", ConsoleID[2]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_2), value);
    sprintf(value, "%02x", ConsoleID[3]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_3), value);
    sprintf(value, "%02x", ConsoleID[4]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_4), value);
    sprintf(value, "%02x", ConsoleID[5]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_5), value);
    sprintf(value, "%02x", ConsoleID[6]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_6), value);
    sprintf(value, "%02x", ConsoleID[7]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_7), value);

    sprintf(value, "%02x", iLinkID[0]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_0), value);
    sprintf(value, "%02x", iLinkID[1]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_1), value);
    sprintf(value, "%02x", iLinkID[2]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_2), value);
    sprintf(value, "%02x", iLinkID[3]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_3), value);
    sprintf(value, "%02x", iLinkID[4]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_4), value);
    sprintf(value, "%02x", iLinkID[5]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_5), value);
    sprintf(value, "%02x", iLinkID[6]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_6), value);
    sprintf(value, "%02x", iLinkID[7]);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_7), value);

    if (ConType != MECHA_TYPE_36)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_MODEL_NAME_WR), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MODEL_NAME), TRUE);

        SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_MODEL_NAME), EEPROMGetModelName());
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_MODEL_NAME_WR), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_MODEL_NAME), FALSE);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_INIT_MECHA), (!ConCEXDEX) && (ConType == MECHA_TYPE_40));
    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_INIT_NTSCPAL_DEFS), (!ConCEXDEX) && ((ConType != MECHA_TYPE_36) && (ConType != MECHA_TYPE_38)));
}

static int WriteConsoleID(HWND hwnd)
{
    u8 ConsoleID[8];
    char value[3];
    int result;

    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_0), value, sizeof(value));
    ConsoleID[0] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_1), value, sizeof(value));
    ConsoleID[1] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_2), value, sizeof(value));
    ConsoleID[2] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_3), value, sizeof(value));
    ConsoleID[3] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_4), value, sizeof(value));
    ConsoleID[4] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_5), value, sizeof(value));
    ConsoleID[5] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_6), value, sizeof(value));
    ConsoleID[6] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CON_ID_7), value, sizeof(value));
    ConsoleID[7] = (u8)strtoul(value, NULL, 16);

    if ((result = EEPROMSetConsoleID(ConsoleID)) == 0)
        MessageBox(hwnd, L"Console ID updated successfully.",
                   L"Update Completed",
                   MB_ICONINFORMATION | MB_OK);
    else
        MessageBox(hwnd, L"Console ID update failed.",
                   L"Update Failed",
                   MB_ICONERROR | MB_OK);

    return result;
}

static int WriteiLinkID(HWND hwnd)
{
    u8 iLinkID[8];
    char value[3];
    int result;

    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_0), value, sizeof(value));
    iLinkID[0] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_1), value, sizeof(value));
    iLinkID[1] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_2), value, sizeof(value));
    iLinkID[2] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_3), value, sizeof(value));
    iLinkID[3] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_4), value, sizeof(value));
    iLinkID[4] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_5), value, sizeof(value));
    iLinkID[5] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_6), value, sizeof(value));
    iLinkID[6] = (u8)strtoul(value, NULL, 16);
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_ILINK_ID_7), value, sizeof(value));
    iLinkID[7] = (u8)strtoul(value, NULL, 16);

    if ((result = EEPROMSetiLinkID(iLinkID)) == 0)
        MessageBox(hwnd, L"i.Link ID updated successfully.",
                   L"Update Completed",
                   MB_ICONINFORMATION | MB_OK);
    else
        MessageBox(hwnd, L"i.Link ID update failed.",
                   L"Update Failed",
                   MB_ICONERROR | MB_OK);

    return result;
}

static int WriteModelName(HWND hwnd)
{
    int result;
    char ModelName[17]; // Maximum of 16 characters for the model name. Anything longer will be truncated.

    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_MODEL_NAME), ModelName, sizeof(ModelName));
    result = EEPROMSetModelName(ModelName);
    if (result)
        MessageBox(hwnd, L"Model Name updated successfully.",
                   L"Update Completed",
                   MB_ICONINFORMATION | MB_OK);
    else
        MessageBox(hwnd, L"Model Name update failed.",
                   L"Update Failed",
                   MB_ICONERROR | MB_OK);

    return result;
}

static INT_PTR CALLBACK IDDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;

    result = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
            InitMenu(hwndDlg);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_CON_ID_WR:
                    if (MessageBox(hwndDlg, L"Write console ID?", L"Update console ID", MB_ICONWARNING | MB_YESNO) == IDYES)
                        WriteConsoleID(hwndDlg);
                    break;
                case IDC_BUTTON_ILINK_ID_WR:
                    if (MessageBox(hwndDlg, L"Write i.Link ID?", L"Update i.Link ID", MB_ICONWARNING | MB_YESNO) == IDYES)
                        WriteiLinkID(hwndDlg);
                    break;
                case IDC_BUTTON_MODEL_NAME_WR:
                    if (MessageBox(hwndDlg, L"Write Model Name?", L"Update Model Name", MB_ICONWARNING | MB_YESNO) == IDYES)
                        WriteModelName(hwndDlg);
                    break;
                case IDC_BUTTON_INIT_MECHA:
                    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_INIT_MECHA), hwndDlg, &InitMECHADlg);
                    break;
                case IDC_BUTTON_INIT_NTSCPAL_DEFS:
                    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_INIT_NTSCPAL_DEFS), hwndDlg, &InitNTSCPALDefsDlg);
                    break;
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

void MenuID(void)
{
    if (MechaInitModel() != 0 || EEPROMInitID() != 0)
    {
        DisplayConnHelp();
        return;
    }

    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_ID_MAN), g_mainWin, &IDDlg);
}
