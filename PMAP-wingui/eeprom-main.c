#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <ctype.h>
#include <errno.h>

#include "platform.h"
#include "main.h"
#include "mecha.h"
#include "eeprom.h"
#include "updates.h"
#include "resource.h"

extern HINSTANCE g_hInstance;
extern HWND g_mainWin;

static void ToggleMainDialogControls(HWND hwnd, BOOL enabled)
{
    if (enabled)
    {
        SendMessage(GetDlgItem(hwnd, IDCANCEL), 0, 0, (LPARAM)L"Close");
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
    }
    else
    {
        SendMessage(GetDlgItem(hwnd, IDCANCEL), 0, 0, (LPARAM)L"Please Wait...");
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_CLR_OSD2_INIT_BIT), FALSE);
    }
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_START), enabled);
    EnableWindow(GetDlgItem(hwnd, IDC_CMB_CHASSIS), enabled);

    EnableWindow(GetDlgItem(hwnd, IDC_CLR_OSD2_INIT_BIT), FALSE);
}

static void InitWindow(HWND hwnd)
{
    HWND combo;
    short int i;
    const wchar_t *chassis[] = {L"A-chassis (SCPH-10000/SCPH-15000, GH-001/3)",
                                L"A-chassis (SCPH-15000/SCPH-18000+ with TI RF-AMP, GH-003)",
                                L"AB-chassis (SCPH-18000, GH-008)",
                                L"B-chassis (SCPH-30001 with Auto-Tilt motor)",
                                L"C-chassis (SCPH-30001/2/3/4)",
                                L"D-chassis (SCPH-300xx/SCPH-350xx)",
                                L"F-chassis (SCPH-30000/SCPH-300xx R)",
                                L"G-chassis (SCPH-390xx)",
                                L"Dragon (SCPH-5x0xx--SCPH-900xx)",
                                L"A-chassis (DTL-H10000)",  // A
                                L"A-chassis (DTL-T10000H)", // A2
                                L"A-chassis (DTL-T10000)",  // A3
                                L"B-chassis (DTL-H30001/2 with Auto-Tilt motor)",
                                L"D-chassis (DTL-H30x0x)",
                                L"Dragon (DTL-5x0xx--DTL-900xx)",
                                NULL};

    combo                    = GetDlgItem(hwnd, IDC_CMB_CHASSIS);
    for (i = 1; chassis[i] != NULL; i++)
        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)chassis[i]);

    ToggleMainDialogControls(hwnd, FALSE);
}

#define EEPROM_UPDATE_FLAG_SANYO    1 // Supports SANYO OP
#define EEPROM_UPDATE_FLAG_NEW_SONY 2 // No support for the old T487

static int UpdateEEPROM(HWND hwndDlg)
{
    struct UpdateData
    {
        int (*update)(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
        unsigned int flags;
    };
    int ClearOSD2InitBit = 0, ReplacedMecha, OpticalBlock, ObjectLens, result, chassis;
    char choice;
    struct UpdateData *selected;
    struct UpdateData data[MECHA_CHASSIS_MODEL_COUNT] = {
        {&MechaUpdateChassisCex10000, 0},
        {&MechaUpdateChassisA, 0},
        {&MechaUpdateChassisAB, 0},
        {&MechaUpdateChassisB, 0},
        {&MechaUpdateChassisC, 0},
        {&MechaUpdateChassisD, 0},
        {&MechaUpdateChassisF, EEPROM_UPDATE_FLAG_SANYO},
        {&MechaUpdateChassisG, EEPROM_UPDATE_FLAG_SANYO | EEPROM_UPDATE_FLAG_NEW_SONY},
        {&MechaUpdateChassisH, EEPROM_UPDATE_FLAG_SANYO | EEPROM_UPDATE_FLAG_NEW_SONY},
        {&MechaUpdateChassisDexA, 0},
        {&MechaUpdateChassisDexA2, 0},
        {&MechaUpdateChassisDexA3, 0},
        {&MechaUpdateChassisDexB, 0},
        {&MechaUpdateChassisDexD, 0},
        {&MechaUpdateChassisDexH, EEPROM_UPDATE_FLAG_SANYO | EEPROM_UPDATE_FLAG_NEW_SONY},
    };

    if ((chassis = SendMessage(GetDlgItem(hwndDlg, IDC_CMB_CHASSIS), CB_GETCURSEL, 0, 0)) == CB_ERR)
        return -1;

    if (chassis >= 0)
    {
        selected      = &data[chassis];
        ReplacedMecha = IsDlgButtonChecked(hwndDlg, IDC_CB_REPLACED_MECHACON) == BST_CHECKED;

        if (selected->flags & EEPROM_UPDATE_FLAG_SANYO)
        {
            do
            {
                printf("Please select the optical block:\n"
                       "\t1. SONY\n"
                       "\t2. SANYO\n"
                       "Your choice: ");
                OpticalBlock = 0;
                if (scanf("%d", &OpticalBlock) > 0)
                    while (getchar() != '\n')
                    {
                    };
            } while (OpticalBlock < 1 || OpticalBlock > 2);
            OpticalBlock--;
        }
        else
            OpticalBlock = MECHA_OP_SONY;

        if (!(selected->flags & EEPROM_UPDATE_FLAG_NEW_SONY) && (OpticalBlock != MECHA_OP_SANYO))
        {
            do
            {
                printf("Please select the object lens:\n"
                       "\t1. T487\n"
                       "\t2. T609K\n"
                       "Your choice: ");
                ObjectLens = 0;
                if (scanf("%d", &ObjectLens) > 0)
                    while (getchar() != '\n')
                    {
                    };
            } while (ObjectLens < 1 || ObjectLens > 2);
            ObjectLens--;
        }
        else
            ObjectLens = MECHA_LENS_T487;

        if (EEPROMCanClearOSD2InitBit(chassis))
        {
            do
            {
                printf("The OSD2 init bit is set. Clear it? (y/n)");
                choice = getchar();
                while (getchar() != '\n')
                {
                };
            } while (choice != 'y' && choice != 'n');
            ClearOSD2InitBit = choice == 'y';
        }

        if ((result = selected->update(ClearOSD2InitBit, ReplacedMecha, ObjectLens, OpticalBlock)) > 0)
        {
            printf("Actions available:\n");
            if (result & UPDATE_REGION_EEP_ECR)
                printf("\tEEPROM ECR\n");
            if (result & UPDATE_REGION_DISCDET)
                printf("\tDisc detect\n");
            if (result & UPDATE_REGION_SERVO)
                printf("\tServo\n");
            if (result & UPDATE_REGION_TILT)
                printf("\tAuto-tilt\n");
            if (result & UPDATE_REGION_TRAY)
                printf("\tTray\n");
            if (result & UPDATE_REGION_EEGS)
                printf("\tEE & GS\n");
            if (result & UPDATE_REGION_ECR)
                printf("\tRTC ECR\n");
            if (result & UPDATE_REGION_RTC)
            {
                printf("\tRTC:\n");
                if (result & UPDATE_REGION_RTC_CTL12)
                    printf("\t\tRTC CTL1,2 ERROR\n");
                if (result & UPDATE_REGION_RTC_TIME)
                    printf("\t\tRTC TIME ERROR\n");
            }
            if (result & UPDATE_REGION_DEFAULTS)
                printf("\tMechacon defaults\n");

            do
            {
                printf("Proceed with updates? (y/n) ");
                choice = getchar();
                while (getchar() != '\n')
                {
                };
            } while (choice != 'y' && choice != 'n');
            if (choice == 'y')
            {
                return MechaCommandExecuteList(NULL, NULL);
            }
            else
            {
                MechaCommandListClear();
                result = 0;
            }
        }
        else
        {
            printf("An error occurred. Wrong chassis selected?\n");
        }

        return result;
    }
    else
    {
        printf("Unsupported chassis selected.\n");
        return -EINVAL;
    }
}

static INT_PTR CALLBACK EepDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                case IDCLOSE:
                    EndDialog(hwndDlg, TRUE);
                    break;
                case IDC_BTN_ERASE_EEPROM:
                    if (EEPROMClear() == 0)
                        MessageBox(g_mainWin, L"EEPROM erase completed.", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"EEPROM erase failed.", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_MECHA_DEFAULTS:
                    if (EEPROMDefaultAll() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (all).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Faile to load EEPROM defaults (all).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_SAVE_EEPROM:
                    break;
                case IDC_BTN_RESTORE_EEPROM:
                    break;
                case IDC_BTN_DEFAULT_DISC_DET:
                    if (EEPROMDefaultDiscDetect() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (disc detect).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (disc detect).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_SERVO:
                    if (EEPROMDefaultServo() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (servo).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (servo).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_TILT_MOTOR:
                    if (EEPROMDefaultTilt() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (tilt motor).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (tilt motor).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_TRAY:
                    if (EEPROMDefaultTray() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (tray).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (tray).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_EEGS:
                    if (EEPROMDefaultEEGS() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (EEGS).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (EEGS).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_OSD:
                    if (EEPROMDefaultOSD() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (OSD).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (OSD).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_MNAME:
                    if (EEPROMDefaultModelName() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (model name).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Faile to load EEPROM defaults (model name).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_PS2ID:
                    if (EEPROMDefaultID() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (ID).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (ID).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_CLOCK:
                    if (EEPROMDefaultRTC() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (RTC).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (RTC).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_DEFAULT_DVDPL:
                    if (EEPROMDefaultDVDVideo() == 0)
                        MessageBox(g_mainWin, L"Loaded EEPROM defaults (DVD Video Player).", L"EEPROM Adjusment", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBox(g_mainWin, L"Failed to load EEPROM defaults (DVD Video Player).", L"EEPROM Adjusment", MB_OK | MB_ICONERROR);
                    break;
                case IDC_BTN_UPDATE:
                    UpdateEEPROM(hwndDlg);
                    break;
                case IDC_BTN_START:
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

void MenuEEPROM(void)
{
    if (MechaInitModel() != 0)
    {
        DisplayConnHelp();
        return;
    }
    if (IsOutdatedBCModel())
        MessageBox(g_mainWin, L"B/C-chassis: EEPROM update required.\n", L"EEPROM Adjusment", MB_OK | MB_ICONWARNING);

    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_EEP_MAN), g_mainWin, &EepDlg);

    MessageBox(g_mainWin, L"If the EEPROM was updated, please reboot the MECHACON\n"
                          L"by leaving this menu before pressing the RESET button.\n",
               L"EEPROM Adjustment", MB_OK | MB_ICONINFORMATION);
}
