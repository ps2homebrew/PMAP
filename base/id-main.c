#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "eeprom-id.h"
#include "main.h"

extern unsigned char ConType;

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
                               "\t1. SCPH-xx000 (Japan)\n"
                               "\t2. SCPH-xx001 (USA)\n"
                               "\t3. SCPH-xx002 (Australia)\n"
                               "\t4. SCPH-xx003 (Great Britian)\n"
                               "\t5. SCPH-xx004 (Europe)\n"
                               "\t6. SCPH-xx005 (Korea)\n"
                               "\t7. SCPH-xx006 (Hong Kong)\n"
                               "\t8. SCPH-xx007 (Taiwan)\n"
                               "\t9. SCPH-xx008 (Russia)\n"
                               "\t10. SCPH-50009 (China)\n"
                               "\t11. SCPH-xx010 (SCPH-50010: Canada, Slims: Mexico)\n"
                               "\t12. SCPH-x0011 (SCPH-50011: Mexico, SCPH-70011: USA)\n"
                               "\t13. SCPH-70012 (Canada)\n"
                               "\t14. Quit\n"
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
                        case 12:
                        case 13:
                            printf("MechaInit: %s\n", MechaInitMechacon(choice, 0) == 0 ? "done" : "failed");
                            break;
                        case 14:
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

static void WriteiLinkID(void)
{
    u8 iLinkID[8];
    unsigned char i;
    unsigned short int NewiLinkIDInput[8];

    EEPROMGetiLinkID(iLinkID);
    printf("Current i.Link ID:\t%02x %02x %02x %02x %02x %02x %02x %02x\n"
           "Enter new ID:\t\t",
           iLinkID[0], iLinkID[1], iLinkID[2], iLinkID[3], iLinkID[4], iLinkID[5], iLinkID[6], iLinkID[7]);
    if (scanf("%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx",
              &NewiLinkIDInput[0], &NewiLinkIDInput[1], &NewiLinkIDInput[2], &NewiLinkIDInput[3], &NewiLinkIDInput[4],
              &NewiLinkIDInput[5], &NewiLinkIDInput[6], &NewiLinkIDInput[7]) == 8)
    {
        for (i = 0; i < 8; i++)
            iLinkID[i] = (u8)NewiLinkIDInput[i];
        printf("iLink ID update %s\n", (EEPROMSetiLinkID(iLinkID) == 0) ? "completed" : "failed");
    }
    else
    {
        printf("Operation aborted.\n");
    }

    while (getchar() != '\n')
    {
    };
}

static void WriteConsoleID(void)
{
    u8 ConsoleID[8];
    unsigned char i;
    unsigned short int NewConsoleIDInput[8];

    EEPROMGetConsoleID(ConsoleID);
    printf("Current console ID:\t%02x %02x %02x %02x %02x %02x %02x %02x\n"
           "Enter new ID:\t\t",
           ConsoleID[0], ConsoleID[1], ConsoleID[2], ConsoleID[3], ConsoleID[4], ConsoleID[5], ConsoleID[6], ConsoleID[7]);
    if (scanf("%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx",
              &NewConsoleIDInput[0], &NewConsoleIDInput[1], &NewConsoleIDInput[2], &NewConsoleIDInput[3], &NewConsoleIDInput[4],
              &NewConsoleIDInput[5], &NewConsoleIDInput[6], &NewConsoleIDInput[7]) == 8)
    {
        for (i = 0; i < 8; i++)
            ConsoleID[i] = (u8)NewConsoleIDInput[i];
        printf("Console ID update %s\n", (EEPROMSetConsoleID(ConsoleID) == 0) ? "completed" : "failed");
    }
    else
    {
        printf("Operation aborted.\n");
    }

    while (getchar() != '\n')
    {
    };
}

static void WriteModelName(void)
{
    const char *ModelName;
    char NewModelName[32]; // Maximum of 16 characters for the model name. Anything longer will be truncated.

    switch (ConType)
    {
        case MECHA_TYPE_36:
            printf("This model does not support a model name.\n");
            return;
    }

    ModelName = EEPROMGetModelName();
    printf("Current model name:\t%s\n"
           "Maximum length is 16\n"
           "Enter new name:\t\t",
           ModelName[0] == 0x00 ? "<No model name>" : ModelName);
    if (fgets(NewModelName, sizeof(NewModelName), stdin))
    {
        NewModelName[16] = '\0';
        printf("Model name update %s\n", (EEPROMSetModelName(NewModelName) == 0) ? "completed" : "failed");
    }
}

static void DisplayIDInfo(void)
{
    u8 iLinkID[8], ConsoleID[8];

    EEPROMGetiLinkID(iLinkID);
    EEPROMGetConsoleID(ConsoleID);
    printf("i.Link ID:\t%02x %02x %02x %02x %02x %02x %02x %02x\n"
           "Console ID:\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
           iLinkID[0], iLinkID[1], iLinkID[2], iLinkID[3], iLinkID[4], iLinkID[5], iLinkID[6], iLinkID[7],
           ConsoleID[0], ConsoleID[1], ConsoleID[2], ConsoleID[3], ConsoleID[4], ConsoleID[5], ConsoleID[6], ConsoleID[7]);
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

void MenuID(void)
{
    char done;
    int choice;

    if (MechaInitModel() != 0 || EEPROMInitID() != 0)
    {
        DisplayConnHelp();
        return;
    }

    done = 0;
    do
    {
        do
        {
            DisplayCommonConsoleInfo();
            DisplayIDInfo();
            printf("ID Managenent:\n"
                   "\t1. Write i.Link ID\n"
                   "\t2. Write console ID\n"
                   "\t3. Write model name (AB-chassis and later only)\n"
                   "\t4. Initialize NTSC/PAL defaults (B-chassis DEX and later only)\n"
                   "\t5. Initialize MECHACON (H/I-chassis only)\n"
                   "\t6. Quit\n"
                   "Your choice: ");
            choice = 0;
            if (scanf("%d", &choice) > 0)
                while (getchar() != '\n')
                {
                };
        } while (choice < 1 || choice > 6);
        putchar('\n');

        switch (choice)
        {
            case 1:
                WriteiLinkID();
                if (EEPROMInitID() != 0)
                {
                    DisplayConnHelp();
                    return;
                }
                break;
            case 2:
                WriteConsoleID();
                if (EEPROMInitID() != 0)
                {
                    DisplayConnHelp();
                    return;
                }
                break;
            case 3:
                WriteModelName();
                break;
            case 4:
                InitNTSCPALDefaults();
                break;
            case 5:
                InitMechacon();
                break;
            default:
                done = 1;
        }
    } while (!done);
}
