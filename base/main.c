#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "platform.h"
#include "main.h"
#include "mecha.h"
#include "eeprom.h"

void DisplayRawIdentData(void)
{
    u8 tm, md;
    const struct MechaIdentRaw *RawData;

    MechaGetMode(&tm, &md);
    RawData = MechaGetRawIdent();

    PlatShowMessage("\nTestMode.%d MD1.%d\n"
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
    PlatShowMessage("\nTestMode.%d MD1.%d\tChecksum: %s\n"
           "MECHA:\t\t%s (%s)\n",
           tm, md, MechaGetEEPROMStat() ? "OK" : "NG",
           MechaGetDesc(), MechaGetCEXDEX() == 0 ? "DEX" : "CEX");
    PlatShowMessage("Serial:\t\t");
    if (EEPROMInitSerial() == 0)
    {
        EEPROMGetSerial(&serial, &emcs);
        PlatShowMessage("%07u\t\t"
               "EMCS:\t%02x\n",
               serial, emcs);
    }
    else
        PlatShowMessage("<no serial number>\n");
    PlatShowMessage("MODELID:\t%04x\t\tTV:\t%s\n", EEPROMGetModelID(), MechaGetTVSystemDesc(EEPROMGetTVSystem()));
    PlatShowMessage("Model:\t\t");
    if (EEPROMInitModelName() == 0)
    {
        ModelName = EEPROMGetModelName();
        PlatShowMessage("%s\n", ModelName);
    }
    else
        PlatShowMessage("<no model name>\n");
    PlatShowMessage("RTC:\t\t%s (%s)\n"
           "OP:\t\t%s\t\tLens:\t%s\n",
           MechaGetRTCName(MechaGetRTCType()), MechaGetRtcStatusDesc(MechaGetRTCType(), MechaGetRTCStat()),
           MechaGetOPTypeName(MechaGetOP()), MechaGetLensTypeName(MechaGetLens()));
    if (EEPROMGetEEPROMStatus() == 1)
        PlatShowMessage("EEPROM was erased (defaults must now be loaded).\n");
}

void DisplayConnHelp(void)
{
    PlatShowMessage("There was a problem communicating with the PlayStation 2 console.\n"
           "Check the connections, press the RESET button and try again.\n\n");
}

int main(int argc, char *argv[])
{
    short int choice;
    unsigned char done;

    if (argc != 2)
    {
        PlatShowMessage("Syntax error. Syntax: PMAP <COM port>\n");
        return EINVAL;
    }

    if (PlatOpenCOMPort(argv[1]) != 0)
    {
        PlatShowMessage("Cannot open %s.\n", argv[1]);
        return ENODEV;
    }

    // TODO!
    PlatDebugInit();

    done = 0;
    do
    {
        do
        {
            PlatShowMessage("\nP.M.A.P (v1.2)\n"
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
