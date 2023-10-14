#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "platform.h"
#include "main.h"
#include "mecha.h"
#include "eeprom.h"
#include "updates.h"

static int DumpEEPROM(const char *filename)
{
    FILE *dump;
    int i, progress, result;
    u16 data;

    printf("\nDumping EEPROM:\n");
    if ((dump = fopen(filename, "wb")) != NULL)
    {
        for (i = 0; i < 1024 / 2; i++)
        {
            putchar('\r');
            printf("Progress: ");
            putchar('[');
            for (progress = 0; progress <= (i * 20 / 512); progress++)
                putchar('#');
            for (; progress < (512 * 20 / 512); progress++)
                putchar(' ');
            putchar(']');

            if ((result = EEPROMReadWord(i, &data)) != 0)
            {
                printf("EEPROM read error %d:%d\n", i, result);
                break;
            }
            if (fwrite(&data, sizeof(u16), 1, dump) != 1)
                break;
        }
        putchar('\n');

        fclose(dump);
    }
    else
        result = -EIO;

    return result;
}

static int RestoreEEPROM(const char *filename)
{
    FILE *dump;
    int i, progress, result;
    u16 data;

    printf("\nRestoring EEPROM:\n");
    if ((dump = fopen(filename, "rb")) != NULL)
    {
        for (i = 0; i < 1024 / 2; i++)
        {
            putchar('\r');
            printf("Progress: ");
            putchar('[');
            for (progress = 0; progress <= (i * 20 / 512); progress++)
                putchar('#');
            for (; progress < (512 * 20 / 512); progress++)
                putchar(' ');
            putchar(']');

            if (fread(&data, sizeof(u16), 1, dump) != 1)
                break;

            if ((result = EEPROMWriteWord(i, data)) != 0)
            {
                printf("EEPROM write error %d:%d\n", i, result);
                break;
            }
        }
        putchar('\n');

        fclose(dump);
    }
    else
        result = -ENOENT;

    return result;
}

#define EEPROM_UPDATE_FLAG_SANYO    1 // Supports SANYO OP
#define EEPROM_UPDATE_FLAG_NEW_SONY 2 // No support for the old T487

static int UpdateEEPROM(int chassis)
{
    struct UpdateData
    {
        int (*update)(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
        unsigned int flags;
    };
    int ClearOSD2InitBit, ReplacedMecha, OpticalBlock, ObjectLens, result;
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

    printf("Update EEPROM\n\n");
    if (chassis >= 0)
    {
        selected = &data[chassis];

        do
        {
            printf("Was the MECHACON replaced (y/n)? ");
            choice = getchar();
            while (getchar() != '\n')
            {
            };
        } while (choice != 'y' && choice != 'n');
        ReplacedMecha = choice == 'y';

        if (selected->flags & EEPROM_UPDATE_FLAG_SANYO)
        {
            do
            {
                printf("Please select the optical block:\n"
                       "\t1. SONY\n"
                       "\t2. SANYO\n"
                       "Your choice: ");
                OpticalBlock = 0;
                if (scanf("%d", &OpticalBlock))
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
                if (scanf("%d", &ObjectLens))
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
        else
            ClearOSD2InitBit = 0;

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
            printf("An error occurred. Wrong chassis selected?, result = %d\n", result);
        }

        return result;
    }
    else
    {
        printf("Unsupported chassis selected.\n");
        return -EINVAL;
    }
}

static int SelectChassis(void)
{
    typedef int (*ChassisProbe_t)(void);
    struct ChassisData
    {
        ChassisProbe_t probe;
        const char *label;
    };
    struct ChassisData data[MECHA_CHASSIS_MODEL_COUNT] = {
        {&IsChassisCex10000, "A-chassis (SCPH-10000/SCPH-15000, GH-001/3)"},
        {&IsChassisA, "A-chassis (SCPH-15000/SCPH-18000+ with TI RF-AMP, GH-003)"},
        {&IsChassisB, "AB-chassis (SCPH-18000, GH-008)"},
        {&IsChassisB, "B-chassis (SCPH-30001 with Auto-Tilt motor)"},
        {&IsChassisC, "C-chassis (SCPH-30001/2/3/4)"},
        {&IsChassisD, "D-chassis (SCPH-300xx/SCPH-350xx)"},
        {&IsChassisF, "F-chassis (SCPH-30000/SCPH-300xx R)"},
        {&IsChassisG, "G-chassis (SCPH-390xx)"},
        {&IsChassisDragon, "Dragon (SCPH-500xx--SCPH-900xx)"},
        {&IsChassisDexA, "A-chassis (DTL-H10000)"},  // A
        {&IsChassisDexA, "A-chassis (DTL-T10000H)"}, // A2
        {&IsChassisDexA, "A-chassis (DTL-T10000)"},  // A3
        {&IsChassisDexB, "B-chassis (DTL-H30001/2 with Auto-Tilt motor)"},
        {&IsChassisDexD, "D-chassis (DTL-H30000)"},
        {&IsChassisDragon, "Dragon (DTL-500xx--DTL-900xx)"}};
    int SelectCount, LastSelectIndex, i, choice;

    DisplayCommonConsoleInfo();
    printf("Chassis:\n");
    for (i = 0, SelectCount = 0, LastSelectIndex = -1; i < MECHA_CHASSIS_MODEL_COUNT; i++)
    {
        if (data[i].probe() != 0)
        {
            printf("\t%2d. %s\n", i + 1, data[i].label);
            SelectCount++;
            LastSelectIndex = i;
        }
    }

    if (SelectCount > 1)
    {
        printf("\t%2d. None\n", i + 1);
        do
        {
            printf("Choice: ");
            choice = 0;
            if (scanf("%d", &choice) > 0)
                while (getchar() != '\n')
                {
                };
        } while (choice < 1 || choice > i + 1);

        --choice;
        if (choice == i)
            choice = -1;
    }
    else
    {
        choice = LastSelectIndex;
    }

    return choice;
}

void MenuEEPROM(void)
{
    static const char *ChassisNames[MECHA_CHASSIS_MODEL_COUNT] = {
        "A-chassis (SCPH-10000/SCPH-15000, GH-001/3)",
        "A-chassis (SCPH-15000/SCPH-18000+ w/ TI RF-AMP, GH-003)",
        "AB-chassis (SCPH-18000, GH-008)",
        "B-chassis (SCPH-30001 with Auto-Tilt motor)",
        "C-chassis",
        "D-chassis",
        "F-chassis",
        "G-chassis (SCPH-3900x)",
        "H-chassis",
        "A-chassis (DTL-H10000)",
        "A-chassis (DTL-T10000H)",
        "A-chassis (DTL-T10000)",
        "B-chassis (DTL-H30001/2)",
        "D-chassis (DTL-H30000)"
        "H-chassis (DTL-H500xx)"};
    unsigned char done;
    short int choice, chassis;
    char filename[256];

    done = 0;
    do
    {
        if (MechaInitModel() != 0)
        {
            DisplayConnHelp();
            return;
        }
        if (IsOutdatedBCModel())
            printf("B/C-chassis: EEPROM update required.\n");
        chassis = SelectChassis();
        do
        {
            printf("\nSelected chassis: %s\n"
                   "EEPROM operations:\n"
                   "\t1. Display console information\n"
                   "\t2. Dump EEPROM\n"
                   "\t3. Restore EEPROM\n"
                   "\t4. Erase EEPROM\n"
                   "\t5. Load defaults (All)\n"
                   "\t6. Load defaults (Disc Detect)\n"
                   "\t7. Load defaults (Servo)\n"
                   "\t8. Load defaults (Tilt)\n"
                   "\t9. Load defaults (Tray)\n"
                   "\t10. Load defaults (EEGS)\n"
                   "\t11. Load defaults (OSD)\n"
                   "\t12. Load defaults (RTC)\n"
                   "\t13. Load defaults (DVD Player)\n"
                   "\t14. Load defaults (ID)\n"
                   "\t15. Load defaults (Model Name)\n"
                   "\t16. Load defaults (SANYO OP)\n"
                   "\t17. Update EEPROM\n"
                   "\t18. Quit\n"
                   "\nYour choice: ",
                   chassis < 0 ? "Unknown" : ChassisNames[chassis]);
            choice = 0;
            if (scanf("%hd", &choice) > 0)
                while (getchar() != '\n')
                {
                };
        } while (choice < 1 || choice > 18);

        switch (choice)
        {
            case 1:
                DisplayCommonConsoleInfo();
                printf("Press ENTER to continue\n");
                while (getchar() != '\n')
                {
                };
                break;
            case 2:
            {
                char useDefault;
                char default_filename[256];
                u32 serial = 0;
                u8 emcs    = 0;

                if (EEPROMInitSerial() == 0)
                    EEPROMGetSerial(&serial, &emcs);

                const char *model = EEPROMGetModelName();

                // Format the filename
                sprintf(default_filename, "%s_%07u.bin", model, serial);

                printf("Default filename: %s\n", default_filename);
                printf("Do you want to use the default filename? (Y/N): ");
                scanf(" %c", &useDefault);

                if (useDefault == 'Y' || useDefault == 'y')
                    strcpy(filename, default_filename);
                else
                {
                    printf("Enter dump filename: ");
                    if (fgets(filename, sizeof(filename), stdin))
                        filename[strlen(filename) - 1] = '\0';
                }

                printf("Dump %s.\n", DumpEEPROM(filename) == 0 ? "completed" : "failed");
            }
            break;
            case 3:
                printf("Enter dump filename: ");
                if (fgets(filename, sizeof(filename), stdin))
                {
                    filename[strlen(filename) - 1] = '\0';
                    // gets(filename);
                    printf("Restore %s.\n", RestoreEEPROM(filename) == 0 ? "completed" : "failed");
                }
                break;
            case 4:
#ifdef ID_MANAGEMENT
                printf("EEPROM erase %s.\n", EEPROMClear() == 0 ? "completed" : "failed");
#else
                printf("Function disabled.\n");
#endif
                break;
            case 5:
#ifdef ID_MANAGEMENT
                printf("Defaults (all) load: %s.\n", EEPROMDefaultAll() == 0 ? "completed" : "failed");
#else
                printf("Function disabled.\n");
#endif
                break;
            case 6:
                printf("Defaults (disc detect) load: %s.\n", EEPROMDefaultDiscDetect() == 0 ? "completed" : "failed");
                break;
            case 7:
                printf("Defaults (servo) load: %s.\n", EEPROMDefaultServo() == 0 ? "completed" : "failed");
                break;
            case 8:
                printf("Defaults (tilt) load: %s.\n", EEPROMDefaultTilt() == 0 ? "completed" : "failed");
                break;
            case 9:
                printf("Defaults (tray) load: %s.\n", EEPROMDefaultTray() == 0 ? "completed" : "failed");
                break;
            case 10:
                printf("Defaults (EEGS) load: %s.\n", EEPROMDefaultEEGS() == 0 ? "completed" : "failed");
                break;
            case 11:
                printf("Defaults (OSD) load: %s.\n", EEPROMDefaultOSD() == 0 ? "completed" : "failed");
                break;
            case 12:
                printf("Defaults (RTC) load: %s.\n", EEPROMDefaultRTC() == 0 ? "completed" : "failed");
                break;
            case 13:
                printf("Defaults (DVD Player) load: %s.\n", EEPROMDefaultDVDVideo() == 0 ? "completed" : "failed");
                break;
            case 14:
#ifdef ID_MANAGEMENT
                printf("Defaults (ID) load: %s.\n", EEPROMDefaultID() == 0 ? "completed" : "failed");
#else
                printf("Function disabled.\n");
#endif
                break;
            case 15:
#ifdef ID_MANAGEMENT
                printf("Defaults (Model Name) load: %s.\n", EEPROMDefaultModelName() == 0 ? "completed" : "failed");
#else
                printf("Function disabled.\n");
#endif
                break;
            case 16:
                printf("Defaults (Sanyo OP) load: %s.\n", EEPROMDefaultSanyoOP() == 0 ? "completed" : "failed");
                break;
            case 17:
                printf("EEPROM update: %s.\n", UpdateEEPROM(chassis) == 0 ? "completed" : "failed");
                break;
            case 18:
                done = 1;
                break;
        }

        printf("\nIf the EEPROM was updated, please reboot the MECHACON\n"
               "by leaving this menu before pressing the RESET button.\n");
    } while (!done);
}
