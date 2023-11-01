#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <Windows.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "elect.h"
#include "main.h"
#include "resource.h"

extern HINSTANCE g_hInstance;
extern HWND g_mainWin;

extern unsigned char ElectConIsT10K, ConType, ConSlim;

static void ToggleMainDialogControls(HWND hwnd, BOOL enabled)
{
    if (enabled)
    {
        SendMessage(GetDlgItem(hwnd, IDCANCEL), 0, 0, (LPARAM)L"Close");
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_CHK_T10K), IsChassisDexA());
    }
    else
    {
        SendMessage(GetDlgItem(hwnd, IDCANCEL), 0, 0, (LPARAM)L"Please Wait...");
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_CHK_T10K), FALSE);
    }
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), enabled);
}

static void InitWindow(HWND hwnd)
{
    switch (ConType)
    {
        case MECHA_TYPE_36:
        case MECHA_TYPE_38:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"A: MD1.36/1.38");
            break;
        case MECHA_TYPE_39:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"A/AB/B/C/D: MD1.39");
            break;
        case MECHA_TYPE_F:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"F: CXP103049 x.3.0.0 - x.3.4.0");
            break;
        case MECHA_TYPE_G:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"G: CXP103049 x.3.6.0");
            break;
        case MECHA_TYPE_G2:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"G: CXP103049 x.3.8.0");
            break;
        case MECHA_TYPE_40:
            if (ConSlim)
                SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"Slims");
            else
                SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"H,I,J,X: CXR706080");
            break;
        default:
            SetWindowText(GetDlgItem(hwnd, IDC_ELECT_MECHACON), L"Unknown");
    }

    ToggleMainDialogControls(hwnd, FALSE);
}

static INT_PTR CALLBACK ElectDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR result;

    result = TRUE;
    switch (uMsg)
    {
        case WM_CLOSE:
            EndDialog(hwndDlg, TRUE);
            break;
        case WM_INITDIALOG:
            InitWindow(hwndDlg);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BTN_START:
                    if (IsChassisDexA())
                        ElectConIsT10K = (IsDlgButtonChecked(hwndDlg, IDC_CHK_T10K) == BST_CHECKED);
                    else
                        ElectConIsT10K = 0;
                    ToggleMainDialogControls(hwndDlg, FALSE);
                    ElectAutoAdjust();
                    ToggleMainDialogControls(hwndDlg, TRUE);
                    break;
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

void MenuELECT(void)
{
    if (MechaInitModel() != 0)
    {
        DisplayConnHelp();
        return;
    }

    if (MessageBox(g_mainWin,
                   L"This tool allows you to re-calibrate the electric circuit of the CD/DVD drive.\n"
                   L"You need to do this if you:\n"
                   L"\t1. Change/remove the OPtical (OP) block\n"
                   L"\t2. Change/remove the spindle motor\n"
                   L"\t3. Change the MECHACON\n"
                   L"Warning! This process MAY damage the laser if the wrong type of disc is used!\n"
                   L"\nContinue with automatic ELECT adjustment?",
                   L"Electric Circuit Adjusment",
                   MB_YESNO | MB_ICONWARNING) == IDYES)
    {
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_ELECT_ADJ), g_mainWin, &ElectDlg);
    }
}
