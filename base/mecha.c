#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"

static struct MechaTask tasks[MAX_MECHA_TASKS];
static unsigned char TaskCount = 0;
char MechaName[9], RTCData[19];
static struct MechaIdentRaw MechaIdentRaw;
unsigned char ConMD, ConType, ConTM, ConCEXDEX, ConOP, ConLens, ConRTC, ConRTCStat, ConECR, ConChecksumStat;

int MechaCommandAdd(unsigned short int command, const char *args, unsigned char id, unsigned char tag, unsigned short int timeout, const char *label)
{
    struct MechaTask *task;
    int result;

    if (TaskCount < MAX_MECHA_TASKS)
    {
        task          = &tasks[TaskCount];
        task->command = command;
        task->args[0] = '\0';
        if (args != NULL)
            strcpy(task->args, args);

        task->label   = label;
        task->id      = id;
        task->tag     = tag;
        task->timeout = timeout;

        TaskCount++;
        result = 0;
    }
    else
    {
        PlatShowEMessage("MechaCommandAdd: too many lines.\n");
        result = ENOMEM;
    }

    return result;
}

int MechaCommandExecute(unsigned short int command, unsigned short int timeout, const char *args, char *buffer, unsigned char BufferSize)
{
    char cmd[MECHA_TX_BUFFER_SIZE];
    unsigned short int size;
    int result = 0;

    if (args != NULL)
        sprintf(cmd, "%03x%s\r\n", command, args);
    else
        sprintf(cmd, "%03x\r\n", command);

    PlatDPrintf("PlatWriteCOMPort: %s", cmd);

    if (PlatWriteCOMPort(cmd) == strlen(cmd))
    {
        for (size = 0; size < BufferSize - 1; size++)
        {
            if ((result = PlatReadCOMPort(buffer + size, 1, timeout)) > 0)
            {
                result = 0;

                if ((size + 1 >= 2) && buffer[size - 1] == '\r' && buffer[size] == '\n')
                {
                    size--; // So that the NULL-terminator will overwrite the carriage return character, making the output perfect for functions like strcmp.
                    break;
                }
            }
            else
            {
                if (result == 0)
                {
                    result = -EPIPE;
                    break;
                }
            }
        }

        buffer[size] = '\0';
        if (result == 0)
            result = size;
        PlatDPrintf("PlatReadCOMPort : %s\n", buffer);
    }
    else
        result = -EPIPE;

    return result;
}

int MechaCommandExecuteList(MechaCommandTxHandler_t transmit, MechaCommandRxHandler_t receive)
{
    char RxBuffer[MECHA_RX_BUFFER_SIZE];
    struct MechaTask *task;
    unsigned short int i;
    int result = 0, size;

    for (i = 0, task = tasks; i < TaskCount; i++, task++)
    {
        if (transmit != NULL)
        {
            if ((result = transmit(task)) != 0)
                break;
        }

        switch (task->id)
        {
            case MECHA_TASK_ID_UI:
                RxBuffer[0] = '0';
                RxBuffer[1] = '\0';
                size        = 1;

                switch (task->command)
                {
                    case MECHA_TASK_UI_CMD_SKIP:
                        PlatDPrintf("SKIP: %s\n", task->label);
                        result = 0;
                        break;
                    case MECHA_TASK_UI_CMD_WAIT:
                        PlatSleep(task->timeout);
                        result = 0;
                        break;
                    case MECHA_TASK_UI_CMD_MSG:
                        PlatShowMessageB(task->label);
                        result = 0;
                        break;
                    default:
                        result = 0;
                }
                break;
            default:
                result = MechaCommandExecute(task->command, task->timeout, task->args, RxBuffer, sizeof(RxBuffer));
        }

        if (result >= 0)
        {
            size   = result;
            result = 0;
        }
        else
        {
            if (result == -EPIPE)
                PlatShowEMessage("%02d. %04x%s %s: 101 - rx-Command timed out\n", task->id, task->command, task->args, task->label);
            break;
        }

        if (receive != NULL)
        {
            if ((result = receive(task, RxBuffer, size)) != 0)
                break;
        }
    }

    TaskCount = 0;

    return result;
}

void MechaCommandListClear(void)
{
    TaskCount = 0;
}

int MechaDefaultHandleRes1(MechaTask_t *task, const char *result, short int len)
{
    PlatShowEMessage("%d. %04x%s %s - Rx-command error: %s\n", task->id, task->command, task->args, task->label, result);
    return 1;
}

int MechaDefaultHandleRes2(MechaTask_t *task, const char *result, short int len)
{
    if (!pstricmp(result, "2A0"))
        PlatShowEMessage("%02d. %04x%s %s: 2A0 - Rx-command error, %s\n", task->id, task->command, task->args, task->label);
    else if (!pstricmp(result, "2A1"))
        PlatShowEMessage("%02d. %04x%s %s: 2A1 - Rx-command argument count error\n", task->id, task->command, task->args, task->label);
    else if (!pstricmp(result, "2A2"))
        PlatShowEMessage("%02d. %04x%s %s: 2A2 - Rx-command argument range error\n", task->id, task->command, task->args, task->label);
    else
        PlatShowEMessage("%02d. %04x%s %s: Unrecognized response: %s\n", task->id, task->command, task->args, task->label, result);

    return 1;
}

int MechaDefaultHandleResUnknown(MechaTask_t *task, const char *result, short int len)
{
    PlatShowEMessage("%02d. %04x%s %s: Unrecognized response: %s\n", task->id, task->command, task->args, task->label, result);
    return 1;
}

const struct MechaIdentRaw *MechaGetRawIdent(void)
{
    return &MechaIdentRaw;
}

static void MechaGetNameOfMD(void)
{
    switch (ConMD)
    {
        case 36:
            ConType = MECHA_TYPE_36;
            break;
        case 38:
            ConType = MECHA_TYPE_38;
            break;
        case 39:
            switch (EEPMapRead(EEPROM_MAP_CON))
            {
                case MECHA_CHASSIS_F_SONY:
                case MECHA_CHASSIS_F_SANYO:
                    ConType = MECHA_TYPE_F;
                    break;
                case MECHA_CHASSIS_G_SONY:
                case MECHA_CHASSIS_G_SANYO:
                    if (!pstrincmp(MechaName, "000603", 6))
                        ConType = MECHA_TYPE_G;
                    else if (!pstrincmp(MechaName, "000803", 6))
                        ConType = MECHA_TYPE_G2;
                    else
                        ConType = 0xFF;
                    break;
                default:
                    ConType = MECHA_TYPE_39;
            }
            break;
        case 40:
            ConType = MECHA_TYPE_40;
            break;
        default:
            ConType = 0xFF;
            PlatShowEMessage("MD Name: Unknown MD version.\n");
    }
}

static void MechaParseOP(void)
{
    u16 idReg;

    if (ConMD == 40)
        idReg = EEPMapRead(EEPROM_MAP_CON_NEW);
    else if (ConMD < 40)
        idReg = EEPMapRead(EEPROM_MAP_CON);
    else
    {
        ConOP = 0xFF;
        PlatShowEMessage("OP name: unknown MD version.\n");
        return;
    }

    ConOP = (idReg & 0x20) ? MECHA_OP_SANYO : MECHA_OP_SONY;

    // Old version from EEPROM 2003/03/13:
    /* switch (reg10)
    {
        case MECHA_CHASSIS_F_SONY:
        case MECHA_CHASSIS_G_SONY:
        case MECHA_CHASSIS_BCD:
        case MECHA_CHASSIS_B:
        case MECHA_CHASSIS_AB:
        case MECHA_CHASSIS_A:
        case MECHA_CHASSIS_DEX_A:
        case MECHA_CHASSIS_DEX_B:
        case MECHA_CHASSIS_DEX_BD:
            ConOP = MECHA_OP_SONY;
            break;
        case MECHA_CHASSIS_F_SANYO:
        case MECHA_CHASSIS_G_SANYO:
            ConOP = MECHA_OP_SANYO;
            break;
        default:
            ConOP = 0xFF;
    } */
}

static void MechaParseLens(u16 reg10, u16 reg12, u16 reg13)
{
    if (ConMD == 40)
    {
        ConLens = MECHA_LENS_T609K; // Starting from the G-chassis, SONY stopped allowing the lens type to be selected. The T609K probably became the standard SONY lens.
    }
    else if (ConMD < 40)
    {
        switch (reg10)
        {
            case MECHA_CHASSIS_DEX_A:
            case MECHA_CHASSIS_A:
                if (reg12 == 0x98c9 && reg13 == 0x7878)
                    ConLens = MECHA_LENS_T609K;
                else if (reg12 == 0x97c9 && reg13 == 0x7777)
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_AB:
                if (reg12 == 0x6d8f && reg13 == 0x6f6f)
                    ConLens = MECHA_LENS_T609K;
                else if (reg12 == 0x4d8f && reg13 == 0x4f4f)
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_DEX_B_OLD:
            case MECHA_CHASSIS_BC_OLD:
            case MECHA_CHASSIS_DEX_B:
            case MECHA_CHASSIS_B:
                if (reg12 == 0x6d8f && reg13 == 0x6f6f)
                    ConLens = MECHA_LENS_T609K;
                else if (reg12 == 0x4d8f && (reg13 == 0x4f4f || reg13 == 0x6f4f))
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_DEX_BD:
            case MECHA_CHASSIS_BCD: // B/C/D-chassis
                if ((reg12 == 0x6d8f || reg12 == 0x6b8b) && reg13 == 0x6f6f)
                    ConLens = MECHA_LENS_T609K;
                else if (reg12 == 0x4d8f && (reg13 == 0x4f4f || reg13 == 0x6f4f || reg13 == 0x6f5f))
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_F_SONY:
                if (reg12 == 0x6b8b && reg13 == 0x4f6f)
                    ConLens = MECHA_LENS_T609K;
                else if (reg12 == 0x4d8f && reg13 == 0x6f4f)
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_F_SANYO: // F-chassis with SANYO OP
                if (reg12 == 0x6d8f && reg13 == 0x6f6f)
                    ConLens = MECHA_LENS_T487;
                else
                    ConLens = 0xFF;
                break;
            case MECHA_CHASSIS_G_SONY:
            case MECHA_CHASSIS_G_SANYO:
                ConLens = MECHA_LENS_T609K;
                break;
            default:
                ConLens = 0xFF;
        }
    }
    else
    {
        ConLens = 0xFF;
        PlatShowEMessage("Lens name: unknown MD version.\n");
    }
}

static void MechaParseCEXDEX(void)
{
    char value[3];
    u8 type;

    if (ConMD == 40)
    {
        strncpy(value, &MechaName[2], 2);
        value[2]  = '\0';
        type      = (u8)strtoul(value, NULL, 16);
        ConCEXDEX = (~type & 1);
    }
    else if (ConMD < 40)
    {
        ConCEXDEX = EEPMapRead(EEPROM_MAP_CON) & 1;
    }
    else
    {
        ConCEXDEX = 0xFF;
        PlatShowEMessage("CEXDEX: Unknown MD version\n");
    }
}

static int MechaCmdInitRxModelHandler(const char *data, int len)
{

    // Model information: TestMode, MD number (i.e.      00 C1 00 24 -> TestMode.193, MD1.36).
    // 5.02 mecha:                                  0 03 03 05 22 40 -> TestMode.0,   MD1.40 from M Renewal Date
    // 5.12                                         0 03 11 20 20 05
    //  MechaIdentRaw.cfd = (u32)strtoul(&data[1], NULL, 16);
    //  MechaIdentRaw.cfd = (u32)strtoul(&data[len-7], NULL, 16);

    strncpy(MechaIdentRaw.cfd, data + 1, len - 1);
    MechaIdentRaw.cfd[len] = '\0';
    if (len < 10)
    {
        char temp[5];
        strncpy(temp, data + 1, 4);
        temp[4] = '\0';
        ConTM   = (u8)strtoul(temp, NULL, 16);
        strncpy(temp, &data[5], 4);
        temp[4] = '\0';
        ConMD   = (u8)strtoul(temp, NULL, 16);
    }
    else
    // Dragons
    {
        ConTM = 0;
        ConMD = 40;
    }

    return 0;
}

static int MechaCmdInitRxModel2Handler(const char *data, int len)
{
    if (data[0] == '0')
    { /*    MECHACON version data: TTmmMMRR
            TT Type (00 = PS2, 01 = PSX)
            mm Minor Version
            MM Major Version
            RR MagicGate Region:
                00 - Japan
                01 - USA
                02 - Europe
                03 - Oceania
                04 - Asia
                05 - Russia
                06 - China
                07 - Mexico
            i.e. 00080304 -> PS2, v3.8, Asia    */
        strcpy(MechaName, &data[1]);
        MechaIdentRaw.cfc = (u32)strtoul(&data[1], NULL, 16);
    }
    else
    {
        MechaName[0]      = '\0';
        MechaIdentRaw.cfc = 0;
    }

    return 0;
}

static int MechaCmdInitRxChecksumChkHandler(const char *data, int len)
{
    ConChecksumStat = strtoul(data, NULL, 16) == 0 ? 1 : 0;
    return 0;
}

static int MechaCmdInitRxRtcReadHandler(const char *data, int len)
{
    char temp[3];

    if (len == 19)
    {
        strcpy(RTCData, &data[1]);
        ConRTC = (data[3] == '0' && data[4] == '0'); // 00 = Rohm, non-zero = Ricoh
        if (ConRTC == MECHA_RTC_ROHM)
        {
            temp[0] = data[1];
            temp[1] = data[2];
        }
        else
        { // Ricoh
            temp[0] = data[3];
            temp[1] = data[4];
        }
        temp[2]    = '\0';
        ConRTCStat = (u8)strtoul(temp, NULL, 16);

        return 0;
    }
    else
    {
        PlatShowEMessage("Error: RTC data length is not 18.\n");
        return 1;
    }
}

static int MechaCmdInitRxEepReadHandler(const char *data, int len)
{
    char address[5];
    u16 offset, word;

    if (len == 9)
    {
        strncpy(address, &data[1], 4);
        address[4] = '\0';
        offset     = (u16)strtoul(address, NULL, 16);
        word       = (u16)strtoul(&data[5], NULL, 16);

        EEPMapWrite(offset, word);
        return 0;
    }
    else
    {
        PlatShowEMessage("Error: EEPROM Read response length is not 8.\n");
        return 1;
    }
}

static int InitRxHandler(MechaTask_t *task, const char *result, short int len)
{
    switch (result[0])
    {
        case '0': // Rx-OK
            switch (task->tag)
            {
                case MECHA_CMD_TAG_INIT_MODEL:
                    return MechaCmdInitRxModelHandler(result, len);
                case MECHA_CMD_TAG_INIT_MODEL_2:
                    return MechaCmdInitRxModel2Handler(result, len);
                case MECHA_CMD_TAG_INIT_CHECKSUM_CHK:
                    return MechaCmdInitRxChecksumChkHandler(result, len);
                case MECHA_CMD_TAG_INIT_RTC_READ:
                    return MechaCmdInitRxRtcReadHandler(result, len);
                case MECHA_CMD_TAG_INIT_EEP_READ:
                    return MechaCmdInitRxEepReadHandler(result, len);
                default:
                    return 0;
            }
            break;
        case '1': // Rx-NGErr
            switch (task->tag)
            {
                case MECHA_CMD_TAG_INIT_CHECKSUM_CHK:
                    return MechaCmdInitRxChecksumChkHandler(result, len);
                default:
                    PlatShowEMessage("%02d. %04x%s: Rx-command error: %s\n", task->id, task->command, task->args, task->label, result);
                    return 1;
            }
            break;
        case '2': // Rx-NGBadCmd
            switch (task->tag)
            {
                case MECHA_CMD_TAG_INIT_MODEL_2:
                    return MechaCmdInitRxModel2Handler(result, len);
                default:
                    if (!pstricmp(result, "2A0"))
                        PlatShowEMessage("%02d. %04x%s: 2A0 - Rx-command error\n", task->id, task->command, task->args);
                    else if (!pstricmp(result, "2A1"))
                        PlatShowEMessage("%02d. %04x%s: 2A1 - Rx-command argument count error\n", task->id, task->command, task->args, task->label);
                    else if (!pstricmp(result, "2A2"))
                        PlatShowEMessage("%02d. %04x%s: 2A2 - Rx-command argument range error\n", task->id, task->command, task->args, task->label);
                    else
                        PlatShowEMessage("%02d. %04x%s: Unrecognized response: %s\n", task->id, task->command, task->args, task->label, result);
                    return 1;
            }
        default:
            PlatShowEMessage("%02d. %04x%s: Unrecognized response: %s\n", task->id, task->command, task->args, task->label, result);
            return 1;
    }
}

int MechaInitModel(void)
{
    char address[5];
    int result, i, id;
    static const u16 EEPMapToInit[] = {// EEPROM words to read.
                                       0x0001, 0x0006, 0x0008, 0x000e, 0x0010, 0x0012, 0x0013, 0x0021,
                                       0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0029, 0x002a,
                                       0x002b, 0x002c, 0x002d, 0x002e, 0x0031, 0x0032, 0x0033, 0x0038,
                                       0x003a, 0x003d, 0x003e, 0x0040, 0x0044, 0x004b, 0x00c0, 0x00c1,
                                       0x00c2, 0x00c3, 0x00c4, 0x00e4, 0x00f1, 0x00f2, 0x00f3, 0x00f4,
                                       0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x0140,
                                       0x0141, 0x0142, 0x0143, 0x0144, 0x0145, 0x0146, 0x0147, 0x0148,
                                       0x0149, 0x014a, 0x014b, 0x014c, 0x014d, 0x014e, 0x014f, 0x0160,
                                       0x0161, 0x0162, 0x0163, 0x0164, 0x0165, 0x0166, 0x0167, 0x0188,
                                       0x0189, 0x018a, 0x018b, 0x018c, 0x018d, 0x018e, 0x018f, 0xffff};

    id                              = 1;
    EEPMapClear();
    if ((result = MechaCommandAdd(MECHA_CMD_READ_MODEL, NULL, id++, MECHA_CMD_TAG_INIT_MODEL, MECHA_TASK_NORMAL_TO, "READ MECHACON MD")) == 0 &&
        (result = MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, MECHA_CMD_TAG_INIT_CHECKSUM_CHK, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM CHK")) == 0 &&
        (result = MechaCommandAdd(MECHA_CMD_RTC_READ, NULL, id++, MECHA_CMD_TAG_INIT_RTC_READ, MECHA_TASK_NORMAL_TO, "READ RTC")) == 0 &&
        (result = MechaCommandAdd(MECHA_CMD_READ_MODEL_2, NULL, id++, MECHA_CMD_TAG_INIT_MODEL_2, MECHA_TASK_NORMAL_TO, "READ MECHACON MODEL")) == 0)
    {
        for (i = 0; EEPMapToInit[i] != 0xFFFF; i++, id++)
        {
            sprintf(address, "%04x", EEPMapToInit[i]);
            if ((result = MechaCommandAdd(MECHA_CMD_EEPROM_READ, address, id, MECHA_CMD_TAG_INIT_EEP_READ, MECHA_TASK_NORMAL_TO, "READ EEPROM")) != 0)
                break;
        }

        if (result == 0)
        {
            if ((result = MechaCommandExecuteList(NULL, &InitRxHandler)) == 0)
            {
                MechaIdentRaw.VersionID = EEPMapRead(EEPROM_MAP_CON);
                MechaGetNameOfMD();
                MechaParseCEXDEX();
                MechaParseOP();
                MechaParseLens(EEPMapRead(EEPROM_MAP_CON), EEPMapRead(EEPROM_MAP_OPT_12), EEPMapRead(EEPROM_MAP_OPT_13));
            }
        }
        else
            MechaCommandListClear();
    }
    else
        MechaCommandListClear();

    return result;
}

void MechaGetMode(u8 *tm, u8 *md)
{
    *tm = ConTM;
    *md = ConMD;
}

int MechaGetCEXDEX(void)
{
    return ConCEXDEX;
}

int MechaGetRTCType(void)
{
    return ConRTC;
}

int MechaGetRTCStat(void)
{
    return ConRTCStat;
}

int MechaGetOP(void)
{
    return ConOP;
}

int MechaGetLens(void)
{
    return ConLens;
}

int MechaGetEEPROMStat(void)
{
    return ConChecksumStat;
}

const char *MechaGetDesc(void)
{
    switch (ConType)
    {
        case MECHA_TYPE_36:
            if (ConCEXDEX == 0)
                return "CXP101064-602R";
            else if (ConCEXDEX == 1)
                return "CXP101064-605R";
            else
                return "unknown (Please contact program creator)";
        case MECHA_TYPE_38:
            if (ConCEXDEX == 0)
                if (!pstricmp(MechaName, "00070100"))
                    return "CXP102064-003R";
                else if (!pstricmp(MechaName, "00090100"))
                    return "CXP102064-751R";
                else
                    return "CXP102064-003R,-751R (Please contact program creator)";
            else if (ConCEXDEX == 1)
                return "CXP102064-001R,-002R (Please contact program creator)";
            else
                return "unknown (Please contact program creator)";
        case MECHA_TYPE_39:
            if (!pstricmp(MechaName, "00000200"))
                return "CXP102064-004R (Please contact program creator)";
            else if (!pstricmp(MechaName, "00020200"))
                return "CXP102064-005R";
            else if (!pstricmp(MechaName, "00040201"))
                return "CXP102064-101R";
            else if (!pstricmp(MechaName, "00040202"))
                return "CXP102064-201R";
            else if (!pstricmp(MechaName, "00040203"))
                return "CXP102064-301R";
            else if (!pstricmp(MechaName, "00060201"))
                return "CXP102064-102R";
            else if (!pstricmp(MechaName, "00060202"))
                return "CXP102064-202R";
            else if (!pstricmp(MechaName, "00060203"))
                return "CXP102064-302R";
            else if (!pstricmp(MechaName, "000C0200"))
                return "CXP102064-007R";
            else if (!pstricmp(MechaName, "000C0201"))
                return "CXP102064-103R";
            else if (!pstricmp(MechaName, "000C0202"))
                return "CXP102064-203R";
            else if (!pstricmp(MechaName, "000C0203"))
                return "CXP102064-303R";
            else if (!pstricmp(MechaName, "000E0200"))
                return "CXP102064-008R";
            else if (!pstricmp(MechaName, "000E0201"))
                return "CXP102064-104R";
            else if (!pstricmp(MechaName, "000E0202"))
                return "CXP102064-204R";
            else if (!pstricmp(MechaName, "000E0203"))
                return "CXP102064-304R";
            else if (!pstrincmp(MechaName, "000502", 6))
                return "CXP102064-702R";
            else if (!pstrincmp(MechaName, "000702", 6))
                return "CXP102064-703R";
            else if (!pstrincmp(MechaName, "000D02", 6))
                return "CXP102064-752R,-705R";
            else
                return "CXP102064-xxxR (Please contact program creator)";
        case MECHA_TYPE_F:
            if (!pstricmp(MechaName, "00000301"))
                return "CXP103049-101GG";
            else if (!pstricmp(MechaName, "00000302"))
                return "CXP103049-201GG";
            else if (!pstricmp(MechaName, "00000303"))
                return "CXP103049-301GG";
            else if (!pstricmp(MechaName, "00020300"))
                return "CXP103049-001GG";
            else if (!pstricmp(MechaName, "00020301"))
                return "CXP103049-102GG";
            else if (!pstricmp(MechaName, "00020302"))
                return "CXP103049-202GG";
            else if (!pstricmp(MechaName, "00020303"))
                return "CXP103049-302GG";
            else if (!pstricmp(MechaName, "00040304"))
                return "CXP103049-401GG";
            else
                return "CXP103049-xxx F-chassis (Please contact program creator)";
        case MECHA_TYPE_G:
            if (!pstricmp(MechaName, "00060300"))
                return "CXP103049-002GG";
            else if (!pstricmp(MechaName, "00060301"))
                return "CXP103049-103GG";
            else if (!pstricmp(MechaName, "00060302"))
                return "CXP103049-203GG";
            else if (!pstricmp(MechaName, "00060303"))
                return "CXP103049-303GG";
            else if (!pstricmp(MechaName, "00060304"))
                return "CXP103049-402GG";
            else if (!pstricmp(MechaName, "00060305"))
                return "CXP103049-501GG";
            else
                return "CXP103049-xxx G-chassis (Please contact program creator)";
        case MECHA_TYPE_G2:
            if (!pstricmp(MechaName, "00080300"))
                return "CXP103049-003GG";
            else if (!pstricmp(MechaName, "00080304"))
                return "CXP103049-403GG";
            else
                return "CXP103049-xxx G-chassis (Please contact program creator)";
        case MECHA_TYPE_40: // better check cxd aka M Renewal Date
            if (!pstricmp(MechaName, "00060507") || !pstricmp(MechaName, "00070507"))
                return "CXR706080-106GG";
            else if (!pstrincmp(MechaName, "000005", 6) || !pstrincmp(MechaName, "000105", 6))
                return "CXR706080-101GG";
            else if (!pstrincmp(MechaName, "000205", 6) || !pstrincmp(MechaName, "000305", 6))
                return "CXR706080-102GG";
            else if (!pstrincmp(MechaName, "000405", 6) || !pstrincmp(MechaName, "000505", 6))
                return "CXR706080-103GG";
            else if (!pstrincmp(MechaName, "000605", 6) || !pstrincmp(MechaName, "000705", 6))
                return "CXR706080-104GG";
            else if (!pstrincmp(MechaName, "010A05", 6) || !pstrincmp(MechaName, "010B05", 6))
                return "CXR706080-702GG";
            else if (!pstrincmp(MechaName, "000C05", 6) || !pstrincmp(MechaName, "000D05", 6))
                return "CXR706080-105GG";
            else if (!pstrincmp(MechaName, "010E05", 6) || !pstrincmp(MechaName, "010F05", 6))
                return "CXR706080-703GG/-706GG";
            else if (!pstrincmp(MechaName, "000006", 6) || !pstrincmp(MechaName, "000106", 6))
                return "CXR716080-101GG";
            else if (!pstrincmp(MechaName, "000206", 6) || !pstrincmp(MechaName, "000306", 6))
                return "CXR716080-102GG";
            else if (!pstrincmp(MechaName, "000406", 6) || !pstrincmp(MechaName, "000506", 6))
                return "CXR716080-103GG";
            else if (!pstrincmp(MechaName, "000606", 6) || !pstrincmp(MechaName, "000706", 6))
                return "CXR716080-104GG";
            else if (!pstrincmp(MechaName, "000a06", 6) || !pstrincmp(MechaName, "000b06", 6))
                return "CXR716080-106GG";
            else if (!pstrincmp(MechaName, "000c06", 6) || !pstrincmp(MechaName, "000d06", 6))
                return "CXR726080-301GB";
            else
                return "unknown";
        default:
            return "unknown";
    }
}

int MechaAddPostEEPROMWrCmds(unsigned char id)
{
    if (ConMD <= 39)
    {
        MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM WRITE");
        MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, MECHA_CMD_TAG_INIT_CHECKSUM_CHK, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM CHK");
    }
    else if (ConMD == 40)
    {
        MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM WRITE");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
        MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, MECHA_CMD_TAG_INIT_CHECKSUM_CHK, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM CHK");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
    }
    else
    {
        PlatShowEMessage("EEPROM Update WR CMD: Unsupported MD version.\n");
        return -1;
    }

    return 0;
}

int MechaAddPostUpdateCmds(unsigned char ClearOSD2InitBit, unsigned char id)
{
    char value[9];

    if (ConMD <= 39)
    {
        if (ClearOSD2InitBit)
        {
            sprintf(value, "%04x%04x", EEPROM_MAP_OSD2_17, EEPMapRead(EEPROM_MAP_OSD2_17) & ~0x80);
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, value, id++, 0, MECHA_TASK_NORMAL_TO, "CLEAR OSD2 INIT BIT");
        }

        MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM WRITE");
        MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, MECHA_CMD_TAG_INIT_CHECKSUM_CHK, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM CHK");
        MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "02", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM (DISC DETECT)");
        MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "03", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM (SERVO)");
        if (IsAutoTiltModel())
            MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "04", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM (TILT)");
        switch (ConMD)
        {
            case 36:
            case 38:
                MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "04", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM (TRAY)");
                break;
            case 39:
                MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "05", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM (TRAY)");
                break;
            default:
                // Shouldn't happen.
                return -1;
        }
    }
    else if (ConMD == 40)
    {
        if (ClearOSD2InitBit)
        {
            sprintf(value, "%04x%04x", EEPROM_MAP_OSD2_17_NEW, EEPMapRead(EEPROM_MAP_OSD2_17_NEW) & ~0x80);
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, value, id++, 0, MECHA_TASK_NORMAL_TO, "CLEAR OSD2 INIT BIT");
        }

        MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM WRITE");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
        MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, MECHA_CMD_TAG_INIT_CHECKSUM_CHK, MECHA_TASK_NORMAL_TO, "EEPROM CHECKSUM CHK");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
        MechaCommandAdd(MECHA_CMD_UPLOAD_NEW, "00", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM TO MECHACON-RAM");
        MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
    }
    else
    {
        PlatShowEMessage("Post Update CMD: Unsupported MD version.\n");
        return -1;
    }

    return 0;
}

int IsChassisCex10000(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_A:
            case MECHA_CHASSIS_AB:
                return 1;
        }
    }

    return 0;
}

int IsChassisA(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_AB:
                return 1;
        }
    }

    return 0;
}

int IsChassisB(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_BC_OLD:
            case MECHA_CHASSIS_BCD:
            case MECHA_CHASSIS_B:
                switch (EEPMapRead(EEPROM_MAP_OPT_13))
                {
                    case 0x4f4f:
                    case 0x6f4f:
                    case 0x6f6f:
                        switch (EEPMapRead(0x029))
                        {
                            case 0x0019:
                            case 0x0015:
                                switch (EEPMapRead(0x026))
                                {
                                    case 0x0c0a:
                                    case 0x0c06:
                                    case 0x0e06:
                                        switch (EEPMapRead(EEPROM_MAP_OPT_12))
                                        {
                                            case 0x4d8f:
                                            case 0x6d8f:
                                                return 1;
                                        }
                                }
                        }
                }
        }
    }

    return 0;
}

int IsChassisC(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_BC_OLD:
            case MECHA_CHASSIS_BCD:
                switch (EEPMapRead(EEPROM_MAP_OPT_13))
                {
                    case 0x4f4f:
                    case 0x6f4f:
                    case 0x6f6f:
                        switch (EEPMapRead(0x029))
                        {
                            case 0x0019:
                            case 0x0015:
                                switch (EEPMapRead(0x026))
                                {
                                    case 0x0c0a:
                                    case 0x0c06:
                                    case 0x0e06:
                                        switch (EEPMapRead(EEPROM_MAP_OPT_12))
                                        {
                                            case 0x4d8f:
                                            case 0x6d8f:
                                                return 1;
                                        }
                                }
                        }
                }
        }
    }

    return 0;
}

int IsChassisD(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_BCD:
                switch (EEPMapRead(EEPROM_MAP_OPT_13))
                {
                    case 0x6f4f:
                    case 0x6f5f:
                    case 0x6f6f:
                        switch (EEPMapRead(0x026))
                        {
                            case 0x0c06:
                            case 0x0e06:
                            case 0x9a4d:
                                switch (EEPMapRead(0x029))
                                {
                                    case 0x0019:
                                    case 0x0013:
                                        switch (EEPMapRead(EEPROM_MAP_OPT_12))
                                        {
                                            case 0x4d8f:
                                            case 0x6b8b:
                                                return 1;
                                        }
                                }
                        }
                }
        }
    }

    return 0;
}

int IsChassisF(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_F_SONY:
            case MECHA_CHASSIS_F_SANYO:
                return 1;
        }
    }

    return 0;
}

int IsChassisG(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_G_SONY:
            case MECHA_CHASSIS_G_SANYO:
                return 1;
        }
    }

    return 0;
}

int IsChassisDragon(void)
{
    if (ConMD == 40)
    {
        switch (EEPMapRead(EEPROM_MAP_CON_NEW))
        {
            case MECHA_CHASSIS_H_SONY:
            case MECHA_CHASSIS_H_SANYO:
            case MECHA_CHASSIS_SLIM:
                return 1;
            default:
                return 0;
        }
    }
    else if (ConMD > 40)
        PlatShowEMessage("IsChassisDragon: Unknown MD version.\n");

    return 0;
}

int IsChassisDexA(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_DEX_A:
                return 1;
        }
    }

    return 0;
}

int IsChassisDexB(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_DEX_B_OLD:
            case MECHA_CHASSIS_DEX_BD:
            case MECHA_CHASSIS_DEX_B:
                switch (EEPMapRead(0x026))
                {
                    case 0x0c0a:
                    case 0x0c06:
                    case 0x0e06:
                        return 1;
                }
        }
    }

    return 0;
}

int IsChassisDexD(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_DEX_BD:
                switch (EEPMapRead(0x026))
                {
                    case 0x9a4d:
                        return 1;
                }
        }
    }

    return 0;
}

// Non-SONY helper functions
int IsAutoTiltModel(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_DEX_B:
            case MECHA_CHASSIS_B:
                return 1;
        }
    }

    return 0;
}

int IsOutdatedBCModel(void)
{
    if (ConMD <= 39)
    {
        switch (EEPMapRead(EEPROM_MAP_CON))
        {
            case MECHA_CHASSIS_BC_OLD:
            case MECHA_CHASSIS_DEX_B_OLD:
                return 1;
        }
    }

    return 0;
}

const char *MechaGetRtcStatusDesc(int type, int status)
{
    if (type == MECHA_RTC_RICOH)
    {
        if (status & 0x40)
            return "Low Battery";
        if (status & 0x10)
            return "No Battery";
    }
    else
    {
        if (status & 0x80)
            return "No Battery";
    }

    return "OK";
}

const char *MechaGetRTCName(int rtc)
{
    switch (rtc)
    {
        case MECHA_RTC_RICOH:
            return "RS5C348AE2";
        case MECHA_RTC_ROHM:
            return "BU9861FV-WE2";
        default:
            return "unknown";
    }
}

const char *MechaGetOPTypeName(int type)
{
    switch (type)
    {
        case MECHA_OP_SONY:
            return "SONY";
        case MECHA_OP_SANYO:
            return "SANYO";
        default:
            return "unknown";
    }
}

const char *MechaGetLensTypeName(int type)
{
    switch (type)
    {
        case MECHA_LENS_T487:
            return "T487";
        case MECHA_LENS_T609K:
            return "T609K";
        default:
            return "unknown";
    }
}

const char *MechaGetTVSystemDesc(int type)
{
    switch (type)
    {
        case TV_SYSTEM_NTSC:
            return "NTSC";
        case TV_SYSTEM_PAL:
            return "PAL";
        default:
            return "unknown";
    }
}

#define itob(i) ((i) / 10 * 16 + (i) % 10) // int to BCD

void MechaGetTimeString(char *TimeString)
{
    time_t RawTime;
    struct tm *TimeInfo;
    u8 month, year;

    // The PlayStation 2 clock is in JST
    time(&RawTime);
    TimeInfo = gmtime(&RawTime);
    TimeInfo->tm_hour += 9;
    mktime(TimeInfo);

    /*                        ssmmhhddDDMMYY
        Ricoh default:   "308801151803258401"
        Rohm default:    "300001431800221001"
        The EEPROM tool dated 23 MAR 2003 used these defaults,
        but the combined ELECT tool will use the system time. */
    month = itob(TimeInfo->tm_mon + 1);
    if (TimeInfo->tm_year + 1900 >= 2000)
        month |= 0x80; //'99 -> '00 Century bit
    year = itob(TimeInfo->tm_year % 100);
    sprintf(TimeString, "%02x%02x%02x%02x%02x%02x%02x", itob(TimeInfo->tm_sec), itob(TimeInfo->tm_min), itob(TimeInfo->tm_hour), itob(TimeInfo->tm_wday),
            itob(TimeInfo->tm_mday), month, year);
}
