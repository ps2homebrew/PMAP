#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "eeprom-id.h"

u8 iLinkID[8], ConsoleID[8];
extern char ConModelName[17];
extern unsigned char ConMD;

static int EEPROMIDSaveiLinkID(const char *data, int len, int offset)
{
    u16 word;

    word                = (u16)strtoul(&data[5], NULL, 16);
    iLinkID[offset + 1] = word >> 8 & 0xFF;
    iLinkID[offset]     = word & 0xFF;
    return 0;
}

static int EEPROMIDSaveConsoleID(const char *data, int len, int offset)
{
    u16 word;

    word                  = (u16)strtoul(&data[5], NULL, 16);
    ConsoleID[offset + 1] = word >> 8 & 0xFF;
    ConsoleID[offset]     = word & 0xFF;
    return 0;
}

static int EEPROMIDRxHandler(MechaTask_t *task, const char *result, short int len)
{
    switch (result[0])
    {
        case '0': // Rx-OK
            switch (task->tag)
            {
                case MECHA_CMD_TAG_INIT_ID_ILINK_ID_0:
                    return EEPROMIDSaveiLinkID(result, len, 0);
                case MECHA_CMD_TAG_INIT_ID_ILINK_ID_1:
                    return EEPROMIDSaveiLinkID(result, len, 2);
                case MECHA_CMD_TAG_INIT_ID_ILINK_ID_2:
                    return EEPROMIDSaveiLinkID(result, len, 4);
                case MECHA_CMD_TAG_INIT_ID_ILINK_ID_3:
                    return EEPROMIDSaveiLinkID(result, len, 6);
                case MECHA_CMD_TAG_INIT_ID_CON_ID_0:
                    return EEPROMIDSaveConsoleID(result, len, 0);
                case MECHA_CMD_TAG_INIT_ID_CON_ID_1:
                    return EEPROMIDSaveConsoleID(result, len, 2);
                case MECHA_CMD_TAG_INIT_ID_CON_ID_2:
                    return EEPROMIDSaveConsoleID(result, len, 4);
                case MECHA_CMD_TAG_INIT_ID_CON_ID_3:
                    return EEPROMIDSaveConsoleID(result, len, 6);
                default:
                    return 0;
            }
            break;
        case '1': // Rx-NGErr
            switch (task->tag)
            {
                default:
                    return MechaDefaultHandleRes1(task, result, len);
            }
            break;
        case '2': // Rx-NGBadCmd
            switch (task->tag)
            {
                default:
                    return MechaDefaultHandleRes2(task, result, len);
            }
        default:
            return MechaDefaultHandleResUnknown(task, result, len);
    }
}

struct RegionData
{
    u8 region, vmode;
};

int MechaInitMechacon(int model, int IsDex)
{
    const struct RegionData CEXregions[] = {
        {0, 0}, // 00 Japan
        {1, 0}, // 01 USA
        {3, 1}, // 02 Australia
        {2, 1}, // 03 Great Britian
        {2, 1}, // 04 Europe
        {4, 0}, // 05 Korea
        {4, 0}, // 06 Hong Kong
        {4, 0}, // 07 Taiwan
        {5, 1}, // 08 Russia
        {6, 0}, // 09 Mainland China
        {1, 0}, // 10 Canada (PAL or NTSC ??)
        {7, 0}, // 11 Mexico
    };
    const struct RegionData DEXregions[] = {
        {0, 0}, // 00
        {1, 0}, // 01
        {1, 1}, // 02
        {0, 0}, // 05
        {0, 0}, // 06
        {1, 1}, // 08
        {6, 0}, // 09
    };
    time_t TimeNow;
    struct tm *tm;
    const struct RegionData *region;
    char data[26];
    unsigned char id;

    region = IsDex ? &DEXregions[model] : &CEXregions[model];
    id     = 1;
    MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "EEPROM WAIT 100ms");
    /*	if(MechaIdentRaw.cfc == 0x00000000)	//Check that the MECHACON ID is 0. Not sure why it's necessary (may be either a safety check for their UI, or might be to prevent reassignments from taking place).
    {	*/
    if (IsDex)
    {
        MechaCommandAdd(MECHA_CMD_INIT_MECHACON, "0001", id++, 0, 6000, "WR INIT DEX");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "EEPROM WAIT 100ms");
    }

    time(&TimeNow);
    srand((unsigned int)TimeNow);
    tm = localtime(&TimeNow);
    // Format: RRYYMMDDHHMMSSrrrr, where R = MagicGate region, Y = Year (from 2000), M = Month (1-12), D = Day of month (1-31), H = Hour (0-23), M = minute (0-59), S = second (0-59), r = random number (first 4 digits from the right).
    // The time and date format is made with Ctime::Format %y%m%d%H%M%S
    snprintf(data, 19, "%02x%02d%02d%02d%02d%02d%02d%04d", region->region, tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, rand() % 10000);
    printf("Shimuke: %s (%zu)\n", data, strlen(data));
    MechaCommandAdd(MECHA_CMD_INIT_SHIMUKE, data, id++, 0, 6000, "WR INIT SHIMUKE");
    MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "EEPROM WAIT 100ms");
    //	}
    MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "00", id++, 0, 6000, "WR INIT ALL DEFAULT");
    MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "EEPROM WAIT 100ms");
    if (region->vmode == 0)
        MechaCommandAdd(MECHA_CMD_SETUP_OSD, "00", id++, 0, 6000, "WR INIT NTSC");
    else
        MechaCommandAdd(MECHA_CMD_SETUP_OSD, "01", id++, 0, 6000, "WR INIT PAL");
    MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "EEPROM WAIT 100ms");

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}

int EEPROMNTSCPALDefaults(int vmode)
{
    u8 id;

    id = 1;
    if (vmode == 0)
        MechaCommandAdd(MECHA_CMD_SETUP_OSD, "00", id++, 0, 6000, "WR INIT NTSC");
    else
        MechaCommandAdd(MECHA_CMD_SETUP_OSD, "01", id++, 0, 6000, "WR INIT PAL");

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}

int EEPROMInitID(void)
{
    char address[5];
    unsigned char id;

    memset(iLinkID, 0, sizeof(iLinkID));
    memset(ConsoleID, 0, sizeof(ConsoleID));

    id = 1;
    if (ConMD == 40)
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_NEW_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_0, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_NEW_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_1, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_NEW_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_2, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_NEW_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_3, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_NEW_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_0, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_NEW_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_1, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_NEW_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_2, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_NEW_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_3, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
    }
    else
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_0, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_1, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_2, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_ILINK_ID_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_ILINK_ID_3, MECHA_TASK_NORMAL_TO, "i.Link ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_0, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_1, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_2, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_CON_ID_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_INIT_ID_CON_ID_3, MECHA_TASK_NORMAL_TO, "CONSOLE ID READ");
    }

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}

void EEPROMGetiLinkID(u8 *id)
{
    memcpy(id, iLinkID, sizeof(iLinkID));
}

void EEPROMGetConsoleID(u8 *id)
{
    memcpy(id, ConsoleID, sizeof(ConsoleID));
}

int EEPROMSetiLinkID(const u8 *NewiLinkID)
{
    char arg[9];
    unsigned char id;

    memcpy(iLinkID, NewiLinkID, sizeof(iLinkID));

    id = 1;
    if (ConMD == 40)
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_NEW_0, iLinkID[1], iLinkID[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_NEW_1, iLinkID[3], iLinkID[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_NEW_2, iLinkID[5], iLinkID[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_NEW_3, iLinkID[7], iLinkID[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
    }
    else
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_0, iLinkID[1], iLinkID[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_1, iLinkID[3], iLinkID[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_2, iLinkID[5], iLinkID[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_ILINK_ID_3, iLinkID[7], iLinkID[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "i.Link ID WRITE");
    }
    MechaAddPostEEPROMWrCmds(id);

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}

int EEPROMSetConsoleID(const u8 *NewConID)
{
    char arg[9];
    unsigned char id;

    memcpy(ConsoleID, NewConID, sizeof(ConsoleID));

    id = 1;
    if (ConMD == 40)
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_NEW_0, ConsoleID[1], ConsoleID[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_NEW_1, ConsoleID[3], ConsoleID[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_NEW_2, ConsoleID[5], ConsoleID[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_NEW_3, ConsoleID[7], ConsoleID[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
    }
    else
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_0, ConsoleID[1], ConsoleID[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_1, ConsoleID[3], ConsoleID[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_2, ConsoleID[5], ConsoleID[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_CON_ID_3, ConsoleID[7], ConsoleID[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "CONSOLE ID WRITE");
    }
    MechaAddPostEEPROMWrCmds(id);

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}

int EEPROMSetModelName(const char *ModelName)
{
    char arg[9];
    unsigned char id;

    memset(ConModelName, 0, sizeof(ConModelName));
    strncpy(ConModelName, ModelName, sizeof(ConModelName) - 1);

    id = 1;
    if (ConMD == 40)
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_0, ConModelName[1], ConModelName[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_1, ConModelName[3], ConModelName[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_2, ConModelName[5], ConModelName[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_3, ConModelName[7], ConModelName[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_4, ConModelName[9], ConModelName[8]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_5, ConModelName[11], ConModelName[10]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_6, ConModelName[13], ConModelName[12]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_NEW_7, ConModelName[15], ConModelName[14]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
    }
    else
    {
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_0, ConModelName[1], ConModelName[0]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_1, ConModelName[3], ConModelName[2]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_2, ConModelName[5], ConModelName[4]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_3, ConModelName[7], ConModelName[6]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_4, ConModelName[9], ConModelName[8]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_5, ConModelName[11], ConModelName[10]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_6, ConModelName[13], ConModelName[12]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
        snprintf(arg, 9, "%04x%02x%02x", EEPROM_MAP_MODEL_NAME_7, ConModelName[15], ConModelName[14]);
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, arg, id++, 0, MECHA_TASK_NORMAL_TO, "MODEL NAME WRITE");
    }
    MechaAddPostEEPROMWrCmds(id);

    return MechaCommandExecuteList(NULL, &EEPROMIDRxHandler);
}
