#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "updates.h"
#include "mecha.h"
#include "eeprom.h"

// If any additional data is required/no longer required, please update the EEPROM map initialization code within MechaInitModel().
extern char MechaName[9], RTCData[19];
extern unsigned char ConMD, ConType, ConTM, ConCEXDEX, ConOP, ConLens, ConRTC, ConRTCStat, ConECR, ConChecksumStat;

struct UpdateData
{
    u16 word, data;
    unsigned char type;
};

static int AddUpdateItem(u16 word, u16 value, unsigned char index)
{
    char args[16];
    sprintf(args, "%04x%04x", word, value);
    return MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, args, index, 0, MECHA_TASK_NORMAL_TO, "EEPROM WRITE");
}

int MechaUpdateChassisCex10000(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, MECHA_CHASSIS_A, UPDATE_REGION_SERVO},
        {0x0026, 0x0e0a, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {0x0024, 0x4242, UPDATE_REGION_SERVO},
        {EEPROM_MAP_FOK, 0x72a0, UPDATE_REGION_SERVO},
        {0x002e, 0x213a, UPDATE_REGION_SERVO},
        {0x0032, 0x0a0a, UPDATE_REGION_SERVO},
        {0x0021, 0x1006, UPDATE_REGION_SERVO},
        {0x0022, 0x1010, UPDATE_REGION_SERVO},
        {0x0023, 0x4250, UPDATE_REGION_SERVO},
        {0x0025, 0x4242, UPDATE_REGION_SERVO},
        {0x002a, 0xe000, UPDATE_REGION_SERVO},
        {0x002d, 0x5005, UPDATE_REGION_SERVO},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00fa, 0x0012, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0x00fb, 0x0002, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, (ConMD == 36 || ConMD == 38) ? "04" : "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x7777)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x7777, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x97c9)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x97c9, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0606)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x7878)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x7777, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x98c9)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x98c9, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0808)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(0, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisA(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0026, 0x0e0a, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {0x0024, 0x3000, UPDATE_REGION_SERVO},
        {0x0038, 0x0000, UPDATE_REGION_SERVO},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0x00fb, 0x0002, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x4f4f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x4f4f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0606)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0808)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisAB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, 0x0801, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0x00fb, 0x0002, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens || (EEPMapRead(EEPROM_MAP_OPT_13) != 0x4f4f && EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f4f));
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    if (forceUpdate)
    {
        if (EEPMapRead(0x026) == 0x0c06 || EEPMapRead(0x026) == 0x0e06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        else if (EEPMapRead(0x0026) != 0x9a4d)
        {
            // Not AB-chassis
            MechaCommandListClear();
            return -EINVAL;
        }
        // Do nothing for 0x9a4d
    }
    else
    {
        if (EEPMapRead(0x026) == 0x0c06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        else if (EEPMapRead(0x026) != 0x0e06 && EEPMapRead(0x0026) != 0x9a4d)
        {
            // Not AB-chassis
            MechaCommandListClear();
            return -EINVAL;
        }
        // Do nothing for 0x0e06 and 0x9a4d
    }
    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0606)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        // Strangely not for the T487. So no new value for T487?
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0808)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, MECHA_CHASSIS_B, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0015, UPDATE_REGION_EEP_ECR},
        {0x00c0, 0x003e, UPDATE_REGION_TILT},
        {0x00c1, 0x1430, UPDATE_REGION_TILT},
        {0x00c2, 0x1167, UPDATE_REGION_TILT},
        {0x00c3, 0x012c, UPDATE_REGION_TILT},
        {0x00c4, 0x2805, UPDATE_REGION_TILT},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens || (EEPMapRead(EEPROM_MAP_OPT_13) != 0x4f4f && EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f4f));
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "04", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TILT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    if (forceUpdate)
    {
        if (EEPMapRead(0x026) == 0x0c0a || EEPMapRead(0x026) == 0x0c06 || EEPMapRead(0x026) == 0x0e06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        // Do nothing for 0x9a4d (also does nothing if not 0x9a4d).
    }
    else
    {
        if (EEPMapRead(0x026) == 0x0c0a || EEPMapRead(0x026) == 0x0c06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        // Do nothing for 0x0e06 and 0x9a4d (also does nothing if not any of these).
    }
    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0606)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        // Strangely not for the T487. So no new value for T487?
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0808)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x15)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "15", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisC(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, MECHA_CHASSIS_BCD, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0015, UPDATE_REGION_EEP_ECR},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens || (EEPMapRead(EEPROM_MAP_OPT_13) != 0x4f4f && EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f4f));
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    if (forceUpdate)
    {
        if (EEPMapRead(0x026) == 0x0c0a || EEPMapRead(0x026) == 0x0c06 || EEPMapRead(0x026) == 0x0e06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        // Do nothing for 0x9a4d (also does nothing if not 0x9a4d).
    }
    else
    {
        if (EEPMapRead(0x026) == 0x0c0a || EEPMapRead(0x026) == 0x0c06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        // Do nothing for 0x0e06 and 0x9a4d (also does nothing if not any of these).
    }
    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0606)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        // Strangely not for the T487. So no new value for T487?
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x0808)
        {
            AddUpdateItem(0x027, 0x0606, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x15)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "15", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisD(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    unsigned int version;
    char versionOnly[5];
    u16 value;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, MECHA_CHASSIS_BCD, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0013, UPDATE_REGION_EEP_ECR},
        {0xFFFF, 0xFFFF, 0xFF}};

    id         = 1;
    UpdateStat = 0;
    strncpy(versionOnly, &MechaName[2], 4);
    versionOnly[4] = '\0';
    version        = (unsigned int)strtoul(versionOnly, NULL, 16);
    if (version < 0x0206)
    { // Not a D-chassis MECHACON
        return EINVAL;
    }

    switch (EEPMapRead(0x026))
    {
        case 0x0c06:
        case 0x0e06:
            if (version != 0x0206)
                ReplacedMecha = 1;
            break;
        case 0x9a4d:
            if (version < 0x0206)
            {
                PlatShowEMessage("Warning, MECHACON is changed to a lower version.\n");
                ReplacedMecha = 1;
            }
            break;
        default:
            ReplacedMecha = 1;
    }

    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }

    if (forceUpdate)
    {
        if (version == 0x0206)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    {
        value = EEPMapRead(0x026);
        if (value == 0x0c06)
        {
            AddUpdateItem(0x026, 0x0e06, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        else if ((value != 0x0e06) && (value != 0x9a4d))
        {
            // Do nothing for 0x0e06 and 0x9a4d. Do nothing if not any of these, other than displaying a warning ("unknown data at 0x26 on D-chassis").
            PlatShowEMessage("unknown data at 0x26 on D-chassis\n");
        }
    }

    if (lens == MECHA_LENS_T487)
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f5f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f5f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }

        if (!forceUpdate)
        {
            if (EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (EEPMapRead(0x02d) != 0x5005)
            {
                AddUpdateItem(0x02d, 0x5005, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (EEPMapRead(0x03a) != 0x8080)
            {
                AddUpdateItem(0x03a, 0x8080, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }
    }
    else
    {
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }

        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6b8b)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6b8b, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x02d) != 0x1405)
        {
            AddUpdateItem(0x02d, 0x1405, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x03a) != 0x8060)
        {
            AddUpdateItem(0x03a, 0x8060, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }

    if (forceUpdate || ConECR != 0x13)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "13", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisF(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x002a, 0xf050, UPDATE_REGION_SERVO},
        {0x00f9, 0xb038, UPDATE_REGION_TRAY},
        {0x00fa, 0x0068, UPDATE_REGION_TRAY},
        {0x0033, 0xfd03, UPDATE_REGION_SERVO},
        {0x003a, 0x8060, UPDATE_REGION_SERVO},
        {0x003d, 0x2213, UPDATE_REGION_SERVO},
        {0x00fb, 0x0007, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    /*  Instead of making individual checks, just check once here.
        As of MAR 2003, there was no official support for a SANYO Optical Block with T609K lens.
        However, the original code that handles the T609K has checks for the SANYO OP,
        and sometimes uses different values from the SONY OP.
        Where applicable, the code for both OPs are merged. */
    if (lens == MECHA_LENS_T609K && opt == MECHA_OP_SANYO)
    { // Not supported.
        return -1;
    }
    // The tool checks to ensure that only either a SANYO or SONY OP was selected.
    if (opt != MECHA_OP_SONY && opt != MECHA_OP_SANYO)
    { // Not supported
        return -1;
    }

    UpdateStat  = 0;
    id          = 1;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens || opt != ConOP);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
        if (opt == MECHA_OP_SANYO)
            MechaCommandAdd(MECHA_CMD_SETUP_SANYO, NULL, id++, 0, MECHA_TASK_NORMAL_TO, "297 SANYO DEFAULTS");
    }

    if (lens == MECHA_LENS_T487)
    {
        if (opt == MECHA_OP_SANYO)
        {
            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f6f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f6f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }
        else
        { // SONY OP
            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x6f4f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_13, 0x6f4f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x4d8f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x4d8f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }

        // For both OPs with T487
        if (forceUpdate || EEPMapRead(0x027) != 0x4d4d)
        {
            AddUpdateItem(0x027, 0x4d4d, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x02d) != 0x5005)
        {
            AddUpdateItem(0x02d, 0x5005, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x02c) != 0x2424)
        {
            AddUpdateItem(0x02c, 0x2424, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x044) != 0x0404)
        {
            AddUpdateItem(0x044, 0x0404, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    else
    { // T609K lens
        if (opt == MECHA_OP_SONY)
        {
            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6b8b)
            {
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x6b8b, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (forceUpdate || EEPMapRead(0x02d) != 0x1405)
            {
                AddUpdateItem(0x02d, 0x1405, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }
        else
        { // SANYO OP
            if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_12) != 0x6d8f)
            {
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x6d8f, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (forceUpdate || EEPMapRead(0x02d) != 0x5005)
            {
                AddUpdateItem(0x02d, 0x5005, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }

        // For both OPs with T609K
        if (forceUpdate || EEPMapRead(EEPROM_MAP_OPT_13) != 0x4f6f)
        {
            AddUpdateItem(EEPROM_MAP_OPT_13, 0x4f6f, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x027) != 0x4d9a)
        {
            AddUpdateItem(0x027, 0x4d9a, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x02c) != 0x2324)
        {
            AddUpdateItem(0x02c, 0x2324, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
        if (forceUpdate || EEPMapRead(0x044) != 0x0417)
        {
            AddUpdateItem(0x044, 0x0417, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }
    }
    if (!forceUpdate)
    { // Only if not forced (otherwise, no update is made).
        if (opt == MECHA_OP_SONY)
        {
            if (EEPMapRead(0x008) != 0x4300)
            {
                AddUpdateItem(0x008, 0x4300, id++);
                UpdateStat |= UPDATE_REGION_DISCDET;
            }
        }
        else
        {
            if (EEPMapRead(0x008) != 0x8800)
            {
                AddUpdateItem(0x008, 0x8800, id++);
                UpdateStat |= UPDATE_REGION_DISCDET;
            }
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }

    if (ConRTC == MECHA_RTC_RICOH)
    { // RS5C348AE2
        if (forceUpdate || EEPMapRead(0x029) != 0xf113)
        {
            AddUpdateItem(0x029, 0xf113, id++);
            UpdateStat |= UPDATE_REGION_EEP_ECR;
        }

        if (forceUpdate || ConECR != 0x13)
        {
            MechaCommandAdd(MECHA_CMD_ECR_WRITE, "13", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
            UpdateStat |= UPDATE_REGION_ECR;
        }

#ifdef UPDATE_RTC_NEW
        if (pstrincmp("3088", RTCData, 4))
        {
            strcpy(RTCData, "3088");
            MechaGetTimeString(&RTCData[4]);
            MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
            UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
        }
#else
        if (!pstrincmp("30C8", RTCData, 4))
        {
            strncpy(RTCData, "3088", 4);
            MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
            UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
        }
        else if (pstrincmp("3088", RTCData, 4))
        {
            strcpy(RTCData, "308801151803258401");
            MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
            UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
        }
#endif
    }
    else
    { // BU9861FV-WE2
        if (forceUpdate || EEPMapRead(0x029) != 0xf100)
        {
            AddUpdateItem(0x029, 0xf100, id++);
            UpdateStat |= UPDATE_REGION_EEP_ECR;
        }

        if (forceUpdate || ConECR != 0x00)
        {
            MechaCommandAdd(MECHA_CMD_ECR_WRITE, "00", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
            UpdateStat |= UPDATE_REGION_ECR;
        }

        if (pstrincmp("3000", RTCData, 4))
        {
#ifdef UPDATE_RTC_NEW
            strcpy(RTCData, "3000");
            MechaGetTimeString(&RTCData[4]);
#else
            strcpy(RTCData, "300001431800221001");
#endif
            MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
            UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
        }
    }
    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisG(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{ // In EEPROM tool 2003/03/13, the newer G-chassis had no updates. In the (later) combined tool, both versions of the G-chassis share the same updates.
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0038, 0x0808, UPDATE_REGION_SERVO},
        {0x003e, 0x4032, UPDATE_REGION_SERVO},
        {0x0006, 0x6a33, UPDATE_REGION_DISCDET},
        {0x0040, 0x1039, UPDATE_REGION_SERVO},
        {0x000e, 0xFFFF, UPDATE_REGION_SERVO},
        {0xFFFF, 0xFFFF, 0xFF}};

    // Instead of making individual checks, just check once here.
    // The tool checks to ensure that only either a SANYO or SONY OP was selected.
    if (opt != MECHA_OP_SONY && opt != MECHA_OP_SANYO)
    { // Not supported
        return -1;
    }
    // The tool checks and supports only the first and second versions of the G-chassis.
    if (ConMD != MECHA_TYPE_G && ConMD != MECHA_TYPE_G2)
    { // Not supported.
        return -1;
    }

    if (EEPMapRead(0x00e) != 0xFFFF)
        ReplacedMecha = 1;

    UpdateStat  = 0;
    id          = 1;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || opt != ConOP);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "000effff", id++, 0, MECHA_TASK_NORMAL_TO, "EEPROM WRITE 0x00E -> 0xFFFF");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
        if (opt == MECHA_OP_SANYO)
            MechaCommandAdd(MECHA_CMD_SETUP_SANYO, NULL, id++, 0, MECHA_TASK_NORMAL_TO, "SANYO DEFAULTS");
    }

    if (forceUpdate)
    {
        AddUpdateItem(0x024, 0x3008, id++);

        if (opt == MECHA_OP_SANYO)
        {
            AddUpdateItem(EEPROM_MAP_OPT_12, 0x6482, id++); // Only for SANYO OP
            AddUpdateItem(0x031, 0x25c8, id++);
            AddUpdateItem(0x04b, 0x1a1a, id++); // Only for SANYO OP
        }
        else
        {
            AddUpdateItem(0x031, 0x1cc8, id++);
        }
        UpdateStat |= UPDATE_REGION_SERVO;
    }
    else
    {
        if ((EEPMapRead(0x024) & 0xFF) != 0x08)
        {
            AddUpdateItem(0x024, (EEPMapRead(0x024) & 0xFF00) | 0x08, id++);
            UpdateStat |= UPDATE_REGION_SERVO;
        }

        if (opt == MECHA_OP_SANYO)
        {
            if (EEPMapRead(EEPROM_MAP_OPT_12) != 0x6482)
            { // Only for SANYO OP
                AddUpdateItem(EEPROM_MAP_OPT_12, 0x6482, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (EEPMapRead(0x008) != 0x8800)
            { // Only when update is not forced.
                AddUpdateItem(0x008, 0x8800, id++);
                UpdateStat |= UPDATE_REGION_DISCDET;
            }

            if (EEPMapRead(0x031) != 0x25c8)
            {
                AddUpdateItem(0x031, 0x25c8, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }

            if (EEPMapRead(0x04b) != 0x1a1a)
            { // Only for SANYO OP
                AddUpdateItem(0x04b, 0x1a1a, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }
        else
        { // SONY OP
            if (EEPMapRead(0x008) != 0x4300)
            { // Only when update is not forced.
                AddUpdateItem(0x008, 0x4300, id++);
                UpdateStat |= UPDATE_REGION_DISCDET;
            }

            if (EEPMapRead(0x031) != 0x1cc8)
            {
                AddUpdateItem(0x031, 0x1cc8, id++);
                UpdateStat |= UPDATE_REGION_SERVO;
            }
        }
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }

    // BU9861FV-WE2
    if (forceUpdate || ConECR != 0x00)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "00", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

    if (pstrincmp("3000", RTCData, 4))
    {
#ifdef UPDATE_RTC_NEW
        strcpy(RTCData, "3000");
        MechaGetTimeString(&RTCData[4]);
#else
        strcpy(RTCData, "300001431800221001");
#endif
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexA(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0026, 0x0e0a, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {EEPROM_MAP_FOK, 0xb2a0, UPDATE_REGION_SERVO},
        {0x002e, 0x213a, UPDATE_REGION_SERVO},
        {0x0032, 0x0a0a, UPDATE_REGION_SERVO},
        {0x002d, 0x5005, UPDATE_REGION_SERVO},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00fa, 0x0012, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0x0140, 0x0000, UPDATE_REGION_EEGS},
        {0x0147, 0x0000, UPDATE_REGION_EEGS},
        {0x00fb, 0x0002, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, (ConMD == 36 || ConMD == 38) ? "04" : "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(0, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexA2(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, (ConMD == 36 || ConMD == 38) ? "04" : "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(0, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexA3(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_ECR, 0x0019, UPDATE_REGION_EEP_ECR},
        {EEPROM_MAP_FOK, 0xb2a0, UPDATE_REGION_SERVO},
        {0x002e, 0x213a, UPDATE_REGION_SERVO},
        {0x0032, 0x0a0a, UPDATE_REGION_SERVO},
        {0x002d, 0x5005, UPDATE_REGION_SERVO},
        {0x00f1, 0x0f38, UPDATE_REGION_TRAY},
        {0x00f2, 0x1e47, UPDATE_REGION_TRAY},
        {0x00f3, 0x5626, UPDATE_REGION_TRAY},
        {0x00f4, 0x6b27, UPDATE_REGION_TRAY},
        {0x00f9, 0x9038, UPDATE_REGION_TRAY},
        {0x00fa, 0x0012, UPDATE_REGION_TRAY},
        {0x00f5, 0x0029, UPDATE_REGION_TRAY},
        {0x00f6, 0x1929, UPDATE_REGION_TRAY},
        {0x00f7, 0x6013, UPDATE_REGION_TRAY},
        {0x00f8, 0x7029, UPDATE_REGION_TRAY},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, (ConMD == 36 || ConMD == 38) ? "04" : "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x19)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "19", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(0, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {EEPROM_MAP_CON, MECHA_CHASSIS_DEX_B, UPDATE_REGION_SERVO},
        {0x0026, 0x0e06, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0015, UPDATE_REGION_EEP_ECR},
        {0x00c0, 0x003e, UPDATE_REGION_TILT},
        {0x00c1, 0x1430, UPDATE_REGION_TILT},
        {0x00c2, 0x1167, UPDATE_REGION_TILT},
        {0x00c3, 0x012c, UPDATE_REGION_TILT},
        {0x00c4, 0x2805, UPDATE_REGION_TILT},
        {0x0140, 0x0001, UPDATE_REGION_EEGS},
        {0x0147, 0x0100, UPDATE_REGION_EEGS},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "04", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TILT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    if (forceUpdate || ConECR != 0x15)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "15", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexD(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0013, 0x6f5f, UPDATE_REGION_SERVO},
        {EEPROM_MAP_ECR, 0x0013, UPDATE_REGION_EEP_ECR},
        {0x0140, 0x0001, UPDATE_REGION_EEGS},
        {0x0147, 0x0100, UPDATE_REGION_EEGS},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT DISC DETECT");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT SERVO");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "05", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }

    if (forceUpdate || ConECR != 0x13)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "13", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

#ifdef UPDATE_RTC_NEW
    if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "3088");
        MechaGetTimeString(&RTCData[4]);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#else
    if (!pstrincmp("30C8", RTCData, 4))
    {
        strncpy(RTCData, "3088", 4);
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12);
    }
    else if (pstrincmp("3088", RTCData, 4))
    {
        strcpy(RTCData, "308801151803258401");
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
#endif

    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

// Note: EEPROM regions may not be labelled correctly because the data was extracted from the March 2003 ELECT tool (which does not give this information).
int MechaUpdateChassisH(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0019, 0xa640, UPDATE_REGION_SERVO},
        {0x003c, 0x703c, UPDATE_REGION_SERVO},
        {0x006f, 0x8f8f, UPDATE_REGION_SERVO},
        {0x007e, 0x8c8a, UPDATE_REGION_SERVO},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT FIXED DATA");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT VAL DATA");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "04", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
        if (opt == MECHA_OP_SANYO)
            MechaCommandAdd(MECHA_CMD_SETUP_SANYO, NULL, id++, 0, MECHA_TASK_NORMAL_TO, "SANYO DEFAULTS");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    // BU9861FV-WE2
    if (forceUpdate || ConECR != 0x00)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "00", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

    if (pstrincmp("3000", RTCData, 4))
    {
#ifdef UPDATE_RTC_NEW
        strcpy(RTCData, "3000");
        MechaGetTimeString(&RTCData[4]);
#else
        strcpy(RTCData, "300001431800221001");
#endif
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
    MechaCommandAdd(MECHA_TASK_UI_CMD_WAIT, NULL, MECHA_TASK_ID_UI, 0, 100, "WAIT 100ms");
    if (!pstrincmp(MechaName, "000405", 6))
    { // The data here seems to appear at the end of the EEPROM (+0x320).
        MechaCommandAdd(MECHA_CMD_CFA, "00b1ea8bc0c8198435", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0103dccd7dd383ff90", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "02a6848324ddf69aeb", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0303dccd7dd383ff90", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "04617d23a351054e8c", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0503dccd7dd383ff90", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "062ea7ea3314ad99d4", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "075da60dccd71b3d78", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "08cb2751b48540f096", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "09b6daef699d964b9c", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0a956d23a4e123571c", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0bce8ed32f1cc6554b", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0ccdfb5aefd39b3ad8", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0db4899cb772970027", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0ebadf03dccd7dd383", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0fe6e874df39d0d517", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1088ccb01b751b05a5", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "11d621b070d9a5768b", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1224b6090c6547f105", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "137cd2b18287ff3da3", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "14fced984b964ff83f", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "15327cfa386b13c769", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "169a24b37564c23965", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "172d432c30153d01b2", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "187d3464661f675020", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "19c8b413bdc93098fe", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1ae2e723073e3a51d2", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1bff90b77afdff00e3", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
    }
    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}

int MechaUpdateChassisDexH(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt)
{
    unsigned short int UpdateStat;
    unsigned char forceUpdate, i, id;
    const struct UpdateData data[] = {
        {0x0019, 0xa640, UPDATE_REGION_SERVO},
        {0x003c, 0x703c, UPDATE_REGION_SERVO},
        {0x006f, 0x8f8f, UPDATE_REGION_SERVO},
        {0x007e, 0x8c8a, UPDATE_REGION_SERVO},
        {0xFFFF, 0xFFFF, 0xFF}};

    id          = 1;
    UpdateStat  = 0;
    forceUpdate = (ReplacedMecha || ConChecksumStat == 0 || lens != ConLens);
    if (forceUpdate)
    {
        UpdateStat |= UPDATE_REGION_DEFAULTS;
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "02", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT FIXED DATA");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "03", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT VAL DATA");
        MechaCommandAdd(MECHA_CMD_CLEAR_CONF, "04", id++, 0, MECHA_TASK_NORMAL_TO, "DEFAULT TRAY");
        if (opt == MECHA_OP_SANYO)
            MechaCommandAdd(MECHA_CMD_SETUP_SANYO, NULL, id++, 0, MECHA_TASK_NORMAL_TO, "SANYO DEFAULTS");
    }

    for (i = 0; data[i].type != 0xFF; i++, id++)
    {
        if (forceUpdate || EEPMapRead(data[i].word) != data[i].data)
        {
            AddUpdateItem(data[i].word, data[i].data, id);
            UpdateStat |= data[i].type;
        }
    }
    // BU9861FV-WE2
    if (forceUpdate || ConECR != 0x00)
    {
        MechaCommandAdd(MECHA_CMD_ECR_WRITE, "00", id++, 0, MECHA_TASK_NORMAL_TO, "ECR WRITE");
        UpdateStat |= UPDATE_REGION_ECR;
    }

    if (pstrincmp("3000", RTCData, 4))
    {
#ifdef UPDATE_RTC_NEW
        strcpy(RTCData, "3000");
        MechaGetTimeString(&RTCData[4]);
#else
        strcpy(RTCData, "300001431800221001");
#endif
        MechaCommandAdd(MECHA_CMD_RTC_WRITE, RTCData, id++, 0, MECHA_TASK_NORMAL_TO, "RTC WRITE");
        UpdateStat |= (UPDATE_REGION_RTC | UPDATE_REGION_RTC_CTL12 | UPDATE_REGION_RTC_TIME);
    }
    if (!pstrincmp(MechaName, "000505", 6))
    { // Something here checks for 0x19, 0x1A, 0x1B and 0x1C. If it's any of those, then the codes for the CEX H-chassis are used instead.
        // No idea what it actually checks for because the UI doesn't seem to set it (always 0xFFFFFFFF).
        // The data here seems to appear at the very end of the EEPROM (+0x320)
        MechaCommandAdd(MECHA_CMD_CFA, "0003dccd7dd383ff90", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0103dccd7dd383ff90", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "02ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "03ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "04ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "05ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "06ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "07ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "08ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "09ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0affffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0bffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0cffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0dffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0effffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "0fffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "10ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "11ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "12ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "13ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "14ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "15ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "16ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "17ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "18ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "19ffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1affffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
        MechaCommandAdd(MECHA_CMD_CFA, "1bffffffffffffffff", id++, 0, MECHA_TASK_NORMAL_TO, "PCEA1240");
    }
    if (MechaAddPostUpdateCmds(ClearOSD2InitBit, id) != 0)
    {
        MechaCommandListClear();
        return 0;
    }

    return UpdateStat;
}
