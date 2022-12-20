u16 EEPMapRead(u16 word);
void EEPMapWrite(u16 word, u16 data);
void EEPMapClear(void);

int EEPROMReadWord(unsigned short int word, u16 *data);
int EEPROMWriteWord(unsigned short int word, u16 data);

int EEPROMClear(void);
int EEPROMDefaultAll(void);
int EEPROMDefaultDiscDetect(void);
int EEPROMDefaultServo(void);
int EEPROMDefaultTilt(void);
int EEPROMDefaultTray(void);
int EEPROMDefaultModelName(void);
int EEPROMDefaultEEGS(void);
int EEPROMDefaultRTC(void);
int EEPROMDefaultDVDVideo(void);
int EEPROMDefaultID(void);
int EEPROMDefaultOSD(void);
int EEPROMDefaultSanyoOP(void);
int EEPROMInitSerial(void);
int EEPROMInitModelName(void);
void EEPROMGetSerial(u32 *serial, u8 *emcs);
const char *EEPROMGetModelName(void);
int EEPROMGetEEPROMStatus(void);
int EEPROMIsOSD2InitBitTrue(void);
int EEPROMGetModelID(void);
int EEPROMGetTVSystem(void);

int EEPROMCanClearOSD2InitBit(int chassis);

enum TV_SYSTEM
{
    TV_SYSTEM_NTSC = 0,
    TV_SYSTEM_PAL
};

/*  Optical Block (0 = SONY, 1 = SANYO)
    10        Type
    a809    0
    a829    1
    b009    0
    b029    1
    8809    0
    8c09    0
    0801    0
    0001    0
    0000    0
    8c08    0
    8808    0

    Lens (0 = T487, 1 = T609K)
    10      12      13      Type
    0000    98c9    7878    1
    0000    97c9    7777    0
    8c08    6d8f    6f6f    1
    8c08    4d8f    4f4f    0
    8c08    4d8f    6f4f    0
    8808    6b8b    6f6f    1
    8808    4d8f    6f5f    0
    0001    98c9    7878    1
    0001    97c9    7777    0
    0801    6d8f    6f6f    1
    0801    4d8f    4f4f    0
    8c09    6d8f    6f6f    1
    8c09    4d8f    4f4f    0
    8c09    4d8f    6f4f    0
    8809    6b8f    6f6f    1
    8809    4d8f    6f4f    0
    a809    6b8b    4f6f    1
    a809    4d8f    6f4f    0
    a829    6d8f    6f6f    0
    b009    -        -      1
    b029    -        -      1

    Chassis - incomplete
    I feel that it's redundant because any console would support a mix and match of any parts compatible with it,
    but some combinations might have never existed.
    10      12      13      26      Type
    0000    98c9    7878            A-chassis DEX
    0000    97c9    7777            A-chassis DEX
    8c08    6d8f    6f6f    0c0a    B-chassis DEX
    8c08    4d8f    4f4f    0c0a    B-chassis DEX
    8c08    4d8f    6f4f    0c0a    B-chassis DEX
    8c08    6d8f    6f6f    0c06    B-chassis DEX
    8c08    4d8f    4f4f    0c06    B-chassis DEX
    8c08    4d8f    6f4f    0c06    B-chassis DEX
    8c08    6d8f    6f6f    0e06    B-chassis DEX
    8c08    4d8f    4f4f    0e06    B-chassis DEX
    8c08    4d8f    6f4f    0e06    B-chassis DEX
    8808    6b8b    6f6f    0c0a    B-chassis DEX
    8808    4d8f    6f5f    0c0a    B-chassis DEX
    8808    6b8b    6f6f    0c06    B-chassis DEX
    8808    4d8f    6f5f    0c06    B-chassis DEX
    8808    6b8b    6f6f    0e06    B-chassis DEX
    8808    4d8f    6f5f    0e06    B-chassis DEX
    8808    6b8b    6f6f    9a4d    D-chassis DEX
    8808    4d8f    6f5f    9a4d    D-chassis DEX
    0001    98c9    7878            A-chassis CEX
    0001    97c9    7777            A-chassis CEX
    0801    6d8f    6f6f            A/AB-chassis CEX
    0801    4d8f    4f4f            A/AB-chassis CEX
    0c09    4d8f    4f4f    0c0a    B-chassis CEX
    8809    4d8f    4f4f    0c0a    B-chassis CEX
    8c09    4d8f    4f4f    0c0a    B-chassis CEX
    0c09    4d8f    6f4f    0c0a    B-chassis CEX
    8809    4d8f    6f4f    0c0a    B-chassis CEX
    8c09    4d8f    6f4f    0c0a    B-chassis CEX
    0c09    4d8f    6f6f    0c0a    B-chassis CEX
    8809    4d8f    6f6f    0c0a    B-chassis CEX
    8c09    4d8f    6f6f    0c0a    B-chassis CEX
    0c09    4d8f    4f4f    0c06    B-chassis CEX
    8809    4d8f    4f4f    0c06    B-chassis CEX
    8c09    4d8f    4f4f    0c06    B-chassis CEX
    0c09    4d8f    6f4f    0c06    B-chassis CEX
    8809    4d8f    6f4f    0c06    B-chassis CEX
    8c09    4d8f    6f4f    0c06    B-chassis CEX
    0c09    4d8f    6f6f    0c06    B-chassis CEX
    8809    4d8f    6f6f    0c06    B-chassis CEX
    8c09    4d8f    6f6f    0c06    B-chassis CEX
    0c09    4d8f    4f4f    0e06    B-chassis CEX
    8809    4d8f    4f4f    0e06    B-chassis CEX
    8c09    4d8f    4f4f    0e06    B-chassis CEX
    0c09    4d8f    6f4f    0e06    B-chassis CEX
    8809    4d8f    6f4f    0e06    B-chassis CEX
    8c09    4d8f    6f4f    0e06    B-chassis CEX
    0c09    4d8f    6f6f    0e06    B-chassis CEX
    8809    4d8f    6f6f    0e06    B-chassis CEX
    8c09    4d8f    6f6f    0e06    B-chassis CEX
    0c09    6d8f    4f4f    0c0a    B-chassis CEX
    8809    6d8f    4f4f    0c0a    B-chassis CEX
    8c09    6d8f    4f4f    0c0a    B-chassis CEX
    0c09    6d8f    6f4f    0c0a    B-chassis CEX
    8809    6d8f    6f4f    0c0a    B-chassis CEX
    8c09    6d8f    6f4f    0c0a    B-chassis CEX
    0c09    6d8f    6f6f    0c0a    B-chassis CEX
    8809    6d8f    6f6f    0c0a    B-chassis CEX
    8c09    6d8f    6f6f    0c0a    B-chassis CEX
    0c09    6d8f    4f4f    0c06    B-chassis CEX
    8809    6d8f    4f4f    0c06    B-chassis CEX
    8c09    6d8f    4f4f    0c06    B-chassis CEX
    0c09    6d8f    6f4f    0c06    B-chassis CEX
    8809    6d8f    6f4f    0c06    B-chassis CEX
    8c09    6d8f    6f4f    0c06    B-chassis CEX
    0c09    6d8f    6f6f    0c06    B-chassis CEX
    8809    6d8f    6f6f    0c06    B-chassis CEX
    8c09    6d8f    6f6f    0c06    B-chassis CEX
    0c09    6d8f    4f4f    0e06    B-chassis CEX
    8809    6d8f    4f4f    0e06    B-chassis CEX
    8c09    6d8f    4f4f    0e06    B-chassis CEX
    0c09    6d8f    6f4f    0e06    B-chassis CEX
    8809    6d8f    6f4f    0e06    B-chassis CEX
    8c09    6d8f    6f4f    0e06    B-chassis CEX
    0c09    6d8f    6f6f    0e06    B-chassis CEX
    8809    6d8f    6f6f    0e06    B-chassis CEX
    8c09    6d8f    6f6f    0e06    B-chassis CEX
    8809    6b8f    6f6f            B/C-chassis
    8809    4d8f    6f4f
    8809    4d8f    6f4f
    a809    6b8f    4f6f            F-chassis
    a809    4d8f    6f4f            F-chassis
    a829    6b8f    6f6f            F-chassis
    b009    -        -              G-chassis
    b029    -        -              G-chassis

    Missing:
    10
    0c08    B-chassis        DEX
    0c09    B/C-chassis      CEX

    RTC IC:
    Type 00 RS5C348AE2
    Type xx BU9861FV-WE2 */

#define EEPROM_MAP_CON_NEW          0x001 // For Dragon models. CEX and DEX can share the same ID.
#define EEPROM_MAP_CON              0x010 // Bit 0 set = CEX, cleared = DEX. Bit 5 set = SANYO OP, cleared = SONY OP
#define EEPROM_MAP_OPT_12           0x012 // Laser OP related
#define EEPROM_MAP_OPT_13           0x013 // Laser OP related
#define EEPROM_MAP_ECR              0x029 // RTC ECR data within EEPROM
#define EEPROM_MAP_FOK              0x02b //"Added support for A-chassis FOK level 0x72a0"
#define EEPROM_MAP_MODEL_NAME_0     0x0D0
#define EEPROM_MAP_MODEL_NAME_1     0x0D1
#define EEPROM_MAP_MODEL_NAME_2     0x0D2
#define EEPROM_MAP_MODEL_NAME_3     0x0D3
#define EEPROM_MAP_MODEL_NAME_4     0x0D4
#define EEPROM_MAP_MODEL_NAME_5     0x0D5
#define EEPROM_MAP_MODEL_NAME_6     0x0D6
#define EEPROM_MAP_MODEL_NAME_7     0x0D7
#define EEPROM_MAP_MODEL_NAME_NEW_0 0x0D8
#define EEPROM_MAP_MODEL_NAME_NEW_1 0x0D9
#define EEPROM_MAP_MODEL_NAME_NEW_2 0x0DA
#define EEPROM_MAP_MODEL_NAME_NEW_3 0x0DB
#define EEPROM_MAP_MODEL_NAME_NEW_4 0x0DC
#define EEPROM_MAP_MODEL_NAME_NEW_5 0x0DD
#define EEPROM_MAP_MODEL_NAME_NEW_6 0x0DE
#define EEPROM_MAP_MODEL_NAME_NEW_7 0x0DF
#define EEPROM_MAP_ILINK_ID_0       0x0E0
#define EEPROM_MAP_ILINK_ID_1       0x0E1
#define EEPROM_MAP_ILINK_ID_2       0x0E2
#define EEPROM_MAP_ILINK_ID_3       0x0E3
#define EEPROM_MAP_MODEL_ID         0x0E4
#define EEPROM_MAP_CON_ID_0         0x0E4
#define EEPROM_MAP_CON_ID_1         0x0E5
#define EEPROM_MAP_CON_ID_2         0x0E6
#define EEPROM_MAP_CON_ID_3         0x0E7
#define EEPROM_MAP_SERIAL_0         0x0E6 // consecutive 3 bytes are the serial number, last byte is the EMCS ID (0 in most consoles, 1 for the MADE IN CHINA units).
#define EEPROM_MAP_SERIAL_1         0x0E7
#define EEPROM_MAP_ILINK_ID_NEW_0   0x0F0
#define EEPROM_MAP_ILINK_ID_NEW_1   0x0F1
#define EEPROM_MAP_ILINK_ID_NEW_2   0x0F2
#define EEPROM_MAP_ILINK_ID_NEW_3   0x0F3
#define EEPROM_MAP_MODEL_ID_NEW     0x0F8
#define EEPROM_MAP_CON_ID_NEW_0     0x0F8
#define EEPROM_MAP_CON_ID_NEW_1     0x0F9
#define EEPROM_MAP_CON_ID_NEW_2     0x0FA
#define EEPROM_MAP_CON_ID_NEW_3     0x0FB
#define EEPROM_MAP_SERIAL_NEW_0     0x0FA // consecutive 3 bytes are the serial number, last byte is the EMCS ID.
#define EEPROM_MAP_SERIAL_NEW_1     0x0FB
#define EEPROM_MAP_EEGS_NEW_0       0x140
#define EEPROM_MAP_EEGS_NEW_1       0x141
#define EEPROM_MAP_EEGS_NEW_2       0x142
#define EEPROM_MAP_EEGS_NEW_3       0x143
#define EEPROM_MAP_EEGS_NEW_4       0x144
#define EEPROM_MAP_EEGS_NEW_5       0x145
#define EEPROM_MAP_EEGS_NEW_6       0x146
#define EEPROM_MAP_EEGS_NEW_7       0x147
#define EEPROM_MAP_EEGS_0           0x148
#define EEPROM_MAP_EEGS_1           0x149
#define EEPROM_MAP_EEGS_2           0x14A
#define EEPROM_MAP_EEGS_3           0x14B
#define EEPROM_MAP_EEGS_4           0x14C
#define EEPROM_MAP_EEGS_5           0x14D
#define EEPROM_MAP_EEGS_6           0x14E
#define EEPROM_MAP_EEGS_7           0x14F
#define EEPROM_MAP_OSD2_NEW_0       0x160
#define EEPROM_MAP_OSD2_NEW_1       0x161
#define EEPROM_MAP_OSD2_17_NEW      0x161 // Byte offset 17 of the OSD configuration block (contains init bit)
#define EEPROM_MAP_OSD2_NEW_2       0x162
#define EEPROM_MAP_OSD2_NEW_3       0x163
#define EEPROM_MAP_OSD2_NEW_4       0x164
#define EEPROM_MAP_OSD2_NEW_5       0x165
#define EEPROM_MAP_OSD2_NEW_6       0x166
#define EEPROM_MAP_OSD2_NEW_7       0x167
#define EEPROM_MAP_OSD2_0           0x188
#define EEPROM_MAP_OSD2_1           0x189
#define EEPROM_MAP_OSD2_17          0x189 // Byte offset 17 of the OSD configuration block (contains init bit)
#define EEPROM_MAP_OSD2_2           0x18a
#define EEPROM_MAP_OSD2_3           0x18b
#define EEPROM_MAP_OSD2_4           0x18c
#define EEPROM_MAP_OSD2_5           0x18d
#define EEPROM_MAP_OSD2_6           0x18e
#define EEPROM_MAP_OSD2_7           0x18f
