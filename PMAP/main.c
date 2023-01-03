#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "platform.h"
#include "../base/main.h"
#include "../base/mecha.h"
#include "../base/eeprom.h"

void DisplayRawIdentData(void)
{
    u8 tm, md;
    const struct MechaIdentRaw *RawData;

    MechaGetMode(&tm, &md);
    RawData = MechaGetRawIdent();

    printf("\nTestMode.%d MD1.%d\n"
           "CFD:\t\t0x%s\n"
           "CFC:\t\t%#08x\n"
           "Version:\t%#04x\n",
           tm, md, RawData->cfd, RawData->cfc, RawData->VersionID);
}

void DisplayCommonConsoleInfo(void)
{
    const char *ModelName;
    u32 serial;
    u8 emcs, tm, md;

    MechaGetMode(&tm, &md);
    printf("\nTestMode.%d MD1.%d\tChecksum: %s\n"
           "MECHA:\t\t%s (%s)\n",
           tm, md, MechaGetEEPROMStat() ? "OK" : "NG",
           MechaGetDesc(), MechaGetCEXDEX() == 0 ? "DEX" : "CEX");
    printf("Serial:\t\t");
    if (EEPROMInitSerial() == 0)
    {
        EEPROMGetSerial(&serial, &emcs);
        printf("%07u\t\t"
               "EMCS:\t%02x\n",
               serial, emcs);
    }
    else
        printf("<no serial number>\n");
    printf("MODELID:\t%04x\t\tTV:\t%s\n", EEPROMGetModelID(), MechaGetTVSystemDesc(EEPROMGetTVSystem()));
    printf("Model:\t\t");
    if (EEPROMInitModelName() == 0)
    {
        ModelName = EEPROMGetModelName();
        printf("%s\n", ModelName);
    }
    else
        printf("<no model name>\n");
    printf("RTC:\t\t%s (%s)\n"
           "OP:\t\t%s\t\tLens:\t%s\n",
           MechaGetRTCName(MechaGetRTCType()), MechaGetRtcStatusDesc(MechaGetRTCType(), MechaGetRTCStat()),
           MechaGetOPTypeName(MechaGetOP()), MechaGetLensTypeName(MechaGetLens()));
    if (EEPROMGetEEPROMStatus() == 1)
        printf("EEPROM was erased (defaults must now be loaded).\n");
}

void DisplayConnHelp(void)
{
    printf("There was a problem communicating with the PlayStation 2 console.\n"
           "Check the connections, press the RESET button and try again.\n\n");
}

int main(int argc, char *argv[])
{
    short int choice;
    unsigned char done;

    if (argc != 2)
    {
        printf("Syntax error. Syntax: PMAP <COM port>\n");
        return EINVAL;
    }

    if (PlatOpenCOMPort(argv[1]) != 0)
    {
        printf("Cannot open %s.\n", argv[1]);
        return ENODEV;
    }

    // TODO!
    PlatDebugInit();

    done = 0;
    do
    {
        do
        {
            printf("\nP.M.A.P (v1.12)\n"
                   "=============================\n"
                   "\t1.\tEEPROM management\n"
                   "\t2.\tAutomatic ELECT adjustment\n"
                   "\t3.\tMECHA adjustment\n"
                   "\t4.\tShow ident data\n"
                   "\t5.\tQuit\n"
#ifdef ID_MANAGEMENT
                   "\t99.\tID management\n"
#endif
                   "Your choice: ");

            choice = 0;
            if (scanf("%hd", &choice) > 0)
                while (getchar() != '\n')
                {
                };

#ifdef ID_MANAGEMENT
            if (choice == 99)
                break;
#endif
        } while (choice < 1 || choice > 5);

        switch (choice)
        {
            case 1:
                MenuEEPROM();
                break;
            case 2:
                MenuELECT();
                break;
            case 3:
                MenuMECHA();
                break;
            case 4:
                if (MechaInitModel() == 0)
                    DisplayRawIdentData();
                else
                    DisplayConnHelp();
                break;
#ifdef ID_MANAGEMENT
            case 99:
                MenuID();
                break;
#endif
            default:
                done = 1;
        }
    } while (!done);

    PlatCloseCOMPort();

    PlatDebugDeinit();

    return 0;
}
