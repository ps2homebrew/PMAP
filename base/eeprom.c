#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"

extern unsigned char ConMD, ConType, ConRTC, ConRTCStat;
u8 ConEmcs;
u32 ConSerial;
char ConModelName[17];
static u16 EEP[0x200];
static u32 EEPMap[0x400 / sizeof(u32)];

u16 EEPMapRead(u16 word)
{
    if (EEPMap[word / 32] & (1 << (word % 32)))
        return EEP[word];
    else
    {
        PlatShowEMessage("EEPROMRead: EEPROM 0x%x not initialized!\n", word);
        abort();
        return 0xFFFF;
    }
}

void EEPMapWrite(u16 word, u16 data)
{
    EEP[word] = data;
    EEPMap[word / 32] |= (1 << (word % 32));
}

void EEPMapClear(void)
{
    memset(EEPMap, 0, sizeof(EEPMap));
    memset(EEP, 0xFF, sizeof(EEP));
}

static int EEPROMSaveSerial0(const char *data, int len)
{
    u16 word;

    word = (u16)strtoul(&data[5], NULL, 16);
    ConSerial |= word;
    return 0;
}

static int EEPROMSaveSerial1(const char *data, int len)
{
    u16 word;

    word = (u16)strtoul(&data[5], NULL, 16);
    ConSerial |= ((word & 0xFF) << 16);
    ConEmcs = word >> 8;

    return 0;
}

static int EEPROMSaveModelName(const char *data, int len, int offset)
{
    u16 word;

    word                     = (u16)strtoul(&data[5], NULL, 16);
    ConModelName[offset + 1] = word >> 8 & 0xFF;
    ConModelName[offset]     = word & 0xFF;
    return 0;
}

static int EEPROMRxHandler(MechaTask_t *task, const char *result, short int len)
{
    switch (result[0])
    {
        case '0': // Rx-OK
            switch (task->tag)
            {
                case MECHA_CMD_TAG_EEPROM_SERIAL_0:
                    return EEPROMSaveSerial0(result, len);
                case MECHA_CMD_TAG_EEPROM_SERIAL_1:
                    return EEPROMSaveSerial1(result, len);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_0:
                    return EEPROMSaveModelName(result, len, 0);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_1:
                    return EEPROMSaveModelName(result, len, 2);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_2:
                    return EEPROMSaveModelName(result, len, 4);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_3:
                    return EEPROMSaveModelName(result, len, 6);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_4:
                    return EEPROMSaveModelName(result, len, 8);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_5:
                    return EEPROMSaveModelName(result, len, 10);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_6:
                    return EEPROMSaveModelName(result, len, 12);
                case MECHA_CMD_TAG_EEPROM_MODEL_NAME_7:
                    return EEPROMSaveModelName(result, len, 14);
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

int EEPROMReadWord(unsigned short int word, u16 *data)
{
    int result;
    char args[5], buffer[16];

    snprintf(args, 5, "%04x", word);
    if ((result = MechaCommandExecute(MECHA_CMD_EEPROM_READ, MECHA_TASK_NORMAL_TO, args, buffer, sizeof(buffer))) == 9)
    {
        *data  = (u16)strtoul(buffer + 5, NULL, 16);
        result = 0;
    }
    else
    {
        if (result > 0) // Data was read.
            result = (int)strtoul(buffer, NULL, 16);
    }

    return result;
}

int EEPROMWriteWord(unsigned short int word, u16 data)
{
    char args[9], buffer[16];
    int result;

    snprintf(args, 9, "%04x%04x", word, data);
    if ((result = MechaCommandExecute(MECHA_CMD_EEPROM_WRITE, MECHA_TASK_NORMAL_TO, args, buffer, sizeof(buffer))) == 9)
    {
        result = 0;
    }
    else
    {
        if (result > 0) // Data was read.
            result = (int)strtoul(buffer, NULL, 16);
    }

    if (ConMD == 40)
        result = 0;

    return result;
}

int EEPROMClear(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_EEPROM_ERASE, MECHA_TASK_LONG_TO, "ffff", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultAll(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_LONG_TO, "00", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultDiscDetect(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "02", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultServo(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "03", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultTilt(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "04", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultTray(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "04", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "05", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultEEGS(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "05", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "06", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

static int EEPROMDefaultRicohRTC(void)
{
    char buffer[8];
    int result;

    if ((result = MechaCommandExecute(MECHA_CMD_RTC_WRITE, MECHA_TASK_NORMAL_TO, "308801151803258401", buffer, sizeof(buffer))) > 0)
        result = (int)strtoul(buffer, NULL, 16);

    return result;
}

static int EEPROMDefaultRohmRTC(void)
{
    char buffer[8];
    int result;

    // BU9861FV-WE2
    if (ConRTCStat & 0x80)
        PlatShowEMessage("Clear RTC: NO BATTERY!!\n");

    if ((result = MechaCommandExecute(MECHA_CMD_RTC_WRITE, MECHA_TASK_NORMAL_TO, "300001431800221001", buffer, sizeof(buffer))) >= 0)
        result = (int)strtoul(buffer, NULL, 16);

    return result;
}

int EEPROMDefaultRTC(void)
{
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
            result = EEPROMDefaultRicohRTC();
            break;
        case 39:
            switch (ConRTC)
            {
                case MECHA_RTC_RICOH:
                    result = EEPROMDefaultRicohRTC();
                    break;
                case MECHA_RTC_ROHM:
                    result = EEPROMDefaultRohmRTC();
                    break;
                default:
                    result = -EINVAL;
            }
            break;
        case 40:
            result = EEPROMDefaultRohmRTC();
            break;
        default:
            PlatShowEMessage("Clear RTC: unknown MD version\n");
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultDVDVideo(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "09", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "0a", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "0b", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultID(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "07", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "08", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "09", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultModelName(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "07", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "08", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultOSD(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 36:
        case 38:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "06", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        case 39:
            if ((result = MechaCommandExecute(MECHA_CMD_CLEAR_CONF, MECHA_TASK_NORMAL_TO, "07", buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMDefaultSanyoOP(void)
{
    char buffer[8];
    int result;

    switch (ConMD)
    {
        case 39:
            switch (ConType)
            {
                case MECHA_TYPE_F:
                case MECHA_TYPE_G:
                case MECHA_TYPE_G2:
                    if ((result = MechaCommandExecute(MECHA_CMD_SETUP_SANYO, MECHA_TASK_NORMAL_TO, NULL, buffer, sizeof(buffer))) > 0)
                        result = strtoul(buffer, NULL, 16) != 0;
                    break;
                default:
                    result = -EINVAL;
            }
            break;
        case 40:
            if ((result = MechaCommandExecute(MECHA_CMD_SETUP_SANYO, MECHA_TASK_NORMAL_TO, NULL, buffer, sizeof(buffer))) > 0)
                result = strtoul(buffer, NULL, 16) != 0;
            break;
        default:
            result = -EINVAL;
    }

    return result;
}

int EEPROMInitSerial(void)
{
    int result;
    char address[5];
    unsigned char id;

    id        = 1;
    ConSerial = 0;
    ConEmcs   = 0;
    if (ConMD == 40)
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_SERIAL_NEW_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_SERIAL_0, MECHA_TASK_NORMAL_TO, "SERIAL READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_SERIAL_NEW_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_SERIAL_1, MECHA_TASK_NORMAL_TO, "SERIAL READ");
    }
    else
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_SERIAL_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_SERIAL_0, MECHA_TASK_NORMAL_TO, "SERIAL READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_SERIAL_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_SERIAL_1, MECHA_TASK_NORMAL_TO, "SERIAL READ");
    }

    if ((result = MechaCommandExecuteList(NULL, &EEPROMRxHandler)) == 0)
    {
        result = (ConSerial == 0 || ConSerial == 0x00FFFFFF);
    }

    return result;
}

int EEPROMInitModelName(void)
{
    int result;
    char address[5];
    unsigned char id;

    id = 1;
    memset(ConModelName, 0, sizeof(ConModelName));
    if (ConMD == 40)
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_0, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_1, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_2, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_3, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_4);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_4, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_5);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_5, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_6);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_6, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_NEW_7);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_7, MECHA_TASK_NORMAL_TO, "M NAME READ");
    }
    else
    {
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_0);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_0, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_1);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_1, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_2);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_2, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_3);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_3, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_4);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_4, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_5);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_5, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_6);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_6, MECHA_TASK_NORMAL_TO, "M NAME READ");
        snprintf(address, 5, "%04x", EEPROM_MAP_MODEL_NAME_7);
        MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id++, MECHA_CMD_TAG_EEPROM_MODEL_NAME_7, MECHA_TASK_NORMAL_TO, "M NAME READ");
    }
    if ((result = MechaCommandExecuteList(NULL, &EEPROMRxHandler)) == 0)
    {
        result = ((unsigned char)ConModelName[0] == 0 || (unsigned char)ConModelName[0] == 0xFF);
    }

    return result;
}

void EEPROMGetSerial(u32 *serial, u8 *emcs)
{
    *serial = ConSerial;
    *emcs   = ConEmcs;
}

const char *EEPROMGetModelName(void)
{
    return ConModelName;
}

int EEPROMGetEEPROMStatus(void)
{
    u16 word;

    if (ConMD == 40)
    {
        word = EEPMapRead(EEPROM_MAP_CON_NEW);
    }
    else if (ConMD < 40)
    {
        word = EEPMapRead(EEPROM_MAP_CON);
    }
    else
    {
        PlatShowEMessage("EEPROM status: unknown MD version.\n");
        return -1;
    }

    return (word == 0xFFFF);
}

// While this indicates the status of the OSD2 init bit on most models, SONY didn't use it for the A and AB chassis.
int EEPROMIsOSD2InitBitTrue(void)
{
    u16 word;

    if (ConMD == 40)
    {
        word = EEPMapRead(EEPROM_MAP_OSD2_17_NEW);
    }
    else if (ConMD < 40)
    {
        word = EEPMapRead(EEPROM_MAP_OSD2_17);
    }
    else
    {
        PlatShowEMessage("IsOSD2InitBitTrue: unknown MD version.\n");
        return -1;
    }

    return (word & 0x80);
}

int EEPROMGetModelID(void)
{
    if (ConMD == 40)
    {
        return EEPMapRead(EEPROM_MAP_MODEL_ID_NEW);
    }
    else if (ConMD < 40)
    {
        return EEPMapRead(EEPROM_MAP_MODEL_ID);
    }
    else
    {
        PlatShowEMessage("ModelID: unknown MD version.\n");
        return -1;
    }
}

int EEPROMGetTVSystem(void)
{
    u16 word;

    switch (ConMD)
    {
        case 40:
            word = EEPMapRead(EEPROM_MAP_EEGS_NEW_2);
            break;
        case 39:
            word = EEPMapRead(EEPROM_MAP_EEGS_2);
            break;
        case 38:
        case 36:
            return TV_SYSTEM_NTSC;
        default:
            PlatShowEMessage("GetTVSystem: unknown MD version.\n");
            return -1;
    }

    return ((word & 0xC0) || (word & 0x1000));
}

/*  SCPH-10000: OSD2 init bit can never be cleared (not supported)
    A-chassis (not SCPH-10000): if EEPROM[0x188] != 0x0000, OSD2 init bit can be cleared
    AB-chassis: if EEPROM[0x18F] != 0x0000, OSD2 init bit can be cleared
    B-chassis and later: if EEPROM[0x189] & 0x80, init OSD2 bit can be cleared

    This function returns 1 if the OSD2 Init Bit is set and the user can clear it. */
int EEPROMCanClearOSD2InitBit(int chassis)
{
    switch (chassis)
    {
        case MECHA_CHASSIS_MODEL_SCPH_10000:
        case MECHA_CHASSIS_MODEL_DEXA:
        case MECHA_CHASSIS_MODEL_DEXA2:
        case MECHA_CHASSIS_MODEL_DEXA3:
            return 0; // These models do not support the OSD2 init bit.
        case MECHA_CHASSIS_MODEL_A:
            return (EEPMapRead(0x188) != 0x0000);
        case MECHA_CHASSIS_MODEL_AB:
            return (EEPMapRead(0x18F) != 0x0000);
        default:
            return EEPROMIsOSD2InitBitTrue();
    }
}
