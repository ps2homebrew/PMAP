#define MAX_MECHA_TASKS      128
#define MECHA_TASK_ARGS_MAX  24

//Recommended buffer sizes for various functions
#define MECHA_TX_BUFFER_SIZE 32
#define MECHA_RX_BUFFER_SIZE 32

struct MechaIdentRaw
{
    u32 cfc, cfd;
    u16 VersionID; //As the updates for B, C and D chassis are seen to be changing this ID, it's probably not a hardware model number. Known within the SONY tools as ADD0x010
};

typedef struct MechaTask
{
    unsigned char id, tag;
    unsigned short int timeout;
    unsigned short int command;
    const char *label;
    char args[MECHA_TASK_ARGS_MAX];
} MechaTask_t;

#define MECHA_TASK_NORMAL_TO   6000
#define MECHA_TASK_LONG_TO     10000

//Software commands
#define MECHA_TASK_ID_UI       0x00
#define MECHA_TASK_UI_CMD_SKIP 0x0000
#define MECHA_TASK_UI_CMD_WAIT 0x0001
#define MECHA_TASK_UI_CMD_MSG  0x0002

enum MECHA_CMD_TAG_INIT
{
    MECHA_CMD_TAG_INIT_MODEL = 1,
    MECHA_CMD_TAG_INIT_MODEL_2,
    MECHA_CMD_TAG_INIT_CHECKSUM_CHK,
    MECHA_CMD_TAG_INIT_RTC_READ,
    MECHA_CMD_TAG_INIT_EEP_READ,
};

enum MECHA_CMD_TAG_INIT_ID
{
    MECHA_CMD_TAG_INIT_ID_ILINK_ID_0 = 1,
    MECHA_CMD_TAG_INIT_ID_ILINK_ID_1,
    MECHA_CMD_TAG_INIT_ID_ILINK_ID_2,
    MECHA_CMD_TAG_INIT_ID_ILINK_ID_3,
    MECHA_CMD_TAG_INIT_ID_CON_ID_0,
    MECHA_CMD_TAG_INIT_ID_CON_ID_1,
    MECHA_CMD_TAG_INIT_ID_CON_ID_2,
    MECHA_CMD_TAG_INIT_ID_CON_ID_3,
};

enum MECHA_CMD_TAG_EEPROM
{
    MECHA_CMD_TAG_EEPROM_SERIAL_0 = 1,
    MECHA_CMD_TAG_EEPROM_SERIAL_1,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_0,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_1,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_2,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_3,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_4,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_5,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_6,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_7,
    MECHA_CMD_TAG_EEPROM_MODEL_NAME_8,
};

enum MECHA_CMD_TAG_ELECT
{
    MECHA_CMD_TAG_ELECT_CD_TYPE = 1,
    MECHA_CMD_TAG_ELECT_CD_FE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_CD_TE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_OP_TYPE_ERROR,
    MECHA_CMD_TAG_ELECT_CD_FCS_CHECK,
    MECHA_CMD_TAG_ELECT_CD_RFDC_LEVEL,
    MECHA_CMD_TAG_ELECT_CD_TPP,
    MECHA_CMD_TAG_ELECT_DVDSL_DETECT_ADJ,
    MECHA_CMD_TAG_ELECT_DVDSL_WR_WORK0_F0,
    MECHA_CMD_TAG_ELECT_DVDSL_RD_PULL_IN,
    MECHA_CMD_TAG_ELECT_DVDSL_WR_WORK0_NEW,
    MECHA_CMD_TAG_ELECT_DISC_DET_DVDMIN_RD,
    MECHA_CMD_TAG_ELECT_DISC_DET_CDMIN_WR,
    MECHA_CMD_TAG_ELECT_DISC_DET_DVDMAX_WR,
    MECHA_CMD_TAG_ELECT_DVDSL_FE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDSL_TE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDSL_JITTER_256,
    MECHA_CMD_TAG_ELECT_DVDSL_JITTER_256_RTY,
    MECHA_CMD_TAG_ELECT_DVDSL_PIPOCC_RATE,
    MECHA_CMD_TAG_ELECT_DVDSL_PONCC_RATE,
    MECHA_CMD_TAG_ELECT_DVDSL_MIRR,
    MECHA_CMD_TAG_ELECT_DVDSL_MIRR_EEPROM_WR,
    MECHA_CMD_TAG_ELECT_DVDSL_DISC_DET_JUDGE,
    MECHA_CMD_TAG_ELECT_DVDSL_RFDC_LEVEL,
    MECHA_CMD_TAG_ELECT_DVDSL_FOCUS_OFFSET,
    MECHA_CMD_TAG_ELECT_DVDSL_DEFOCUS_OFFSET,
    MECHA_CMD_TAG_ELECT_DVDSL_FB_OFFSET,
    MECHA_CMD_TAG_ELECT_DVDDL_DISC_DET_JUDGE,
    MECHA_CMD_TAG_ELECT_DVDDL_L0_FE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDDL_L0_TE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDDL_L0_JITTER_256,
    MECHA_CMD_TAG_ELECT_DVDDL_L0_JITTER_256_RTY,
    MECHA_CMD_TAG_ELECT_DVDDL_L0_RFDC_LEVEL,
    MECHA_CMD_TAG_ELECT_DVD_SET_DSP_JITTER,
    MECHA_CMD_TAG_ELECT_DVDDL_L1_FE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDDL_L1_TE_LOOP_GAIN,
    MECHA_CMD_TAG_ELECT_DVDDL_L1_JITTER_256,
    MECHA_CMD_TAG_ELECT_DVDDL_L1_JITTER_256_RTY,
    MECHA_CMD_TAG_ELECT_DVDDL_L1_RFDC_LEVEL,
    MECHA_CMD_TAG_ELECT_DEX_NEWLENS,
    MECHA_CMD_TAG_ELECT_EEPROM_CHECKSUM_CHK,
};

enum MECHA_CMD_TAG_MECHA
{
    MECHA_CMD_TAG_MECHA_CD_TYPE = 1,
    MECHA_CMD_TAG_MECHA_AUTO_TILT,
    MECHA_CMD_TAG_MECHA_DVD_ERROR_RATE,
    MECHA_CMD_TAG_MECHA_CD_ERROR_RATE,
    MECHA_CMD_TAG_MECHA_DISC_DETECT,
    MECHA_CMD_TAG_MECHA_SET_DISC_TYPE,
};

typedef int (*MechaCommandHandler_t)(const char *data, int len);
typedef int (*MechaCommandTxHandler_t)(MechaTask_t *task);
typedef int (*MechaCommandRxHandler_t)(MechaTask_t *task, const char *result, short int len);

int MechaCommandAdd(unsigned short int command, const char *args, unsigned char id, unsigned char tag, unsigned short int timeout, const char *label);
int MechaCommandExecute(unsigned short int command, unsigned short int timeout, const char *args, char *buffer, unsigned char BufferSize);
int MechaCommandExecuteList(MechaCommandTxHandler_t transmit, MechaCommandRxHandler_t receive);
void MechaCommandListClear(void);

int MechaDefaultHandleRes1(MechaTask_t *task, const char *result, short int len);
int MechaDefaultHandleRes2(MechaTask_t *task, const char *result, short int len);
int MechaDefaultHandleResUnknown(MechaTask_t *task, const char *result, short int len);

const struct MechaIdentRaw *MechaGetRawIdent(void);
int MechaInitModel(void);
void MechaGetMode(u8 *tm, u8 *md);
int MechaGetCEXDEX(void);
int MechaGetRTCType(void);
int MechaGetRTCStat(void);
int MechaGetOP(void);
int MechaGetLens(void);
int MechaGetEEPROMStat(void);
int MechaAddPostEEPROMWrCmds(unsigned char id);
int MechaAddPostUpdateCmds(unsigned char ClearOSD2InitBit, unsigned char id);
const char *MechaGetDesc(void);

int IsChassisCex10000(void);
int IsChassisA(void);
int IsChassisB(void);
int IsChassisC(void);
int IsChassisD(void);
int IsChassisF(void);
int IsChassisG(void);
int IsChassisH(void);
int IsChassisDexA(void);
int IsChassisDexB(void);
int IsChassisDexD(void);

int IsAutoTiltModel(void);
int IsOutdatedBCModel(void);

/*	Everything is in HEX, unless otherwise specified.
	The first few digits returned is the status code. 0 = OK (i.e. 0data), non-zero = error.
	Commands and responses are not case-sensitive.	*/
//Disc types (returned by disc detect command c16):
enum DISC_TYPE
{
    DISC_TYPE_NO_DISC = 0x004,
    DISC_TYPE_UNKNOWN,
    DISC_TYPE_CD8 = 0x010, //CD 8cm mode
    DISC_TYPE_CD12,        //CD 12cm mode
    DISC_TYPE_DVDS8,       //DVD-SL 8cm mode
    DISC_TYPE_DVDD8,       //DVD-DL 8cm mode
    DISC_TYPE_DVDS12,      //DVD-SL 12cm mode
    DISC_TYPE_DVDD12,      //DVD-DL 12cm mode
    DISC_TYPE_SACD
};

//Known error codes:
enum RX_ERROR
{
    RX_ERROR_CMD_INPUT = 0x2A0,
    RX_ERROR_CMD_PARAM_COUNT,
    RX_ERROR_CMD_PARAM_VALUE,
    RX_ERROR_TIMEOUT = 0x101,
    RX_ERROR_TRAY_OPEN,
    RX_ERROR_TRAY_CLOSE,
    RX_ERROR_NO_DISC,
    RX_ERROR_DISC_UNKNOWN,
    RX_ERROR_NO_INIT,
    RX_ERROR_DISC_CD8 = 0x110, //CD 8cm mode
    RX_ERROR_DISC_CD12,        //CD 12cm mode
    RX_ERROR_DISC_DVDS8,       //DVD-SL 8cm mode
    RX_ERROR_DISC_DVDD8,       //DVD-DL 8cm mode
    RX_ERROR_DISC_DVDS12,      //DVD-SL 12cm mode
    RX_ERROR_DISC_DVDD12,      //DVD-DL 12cm mode
    RX_ERROR_DISC_SACD,
    RX_ERROR_PARAM_OVER = 0x120,
    RX_ERROR_EXECUTION,
    RX_ERROR_MECHA_TASK,
    RX_ERROR_SWITCH
};

#define MECHA_CMD_INIT_SHIMUKE       0xc00
#define MECHA_CMD_INIT_MECHACON      0xc01
#define MECHA_CMD_DISC_MODE_CD_8     0xc10
#define MECHA_CMD_DISC_MODE_CD_12    0xc11
#define MECHA_CMD_DISC_MODE_DVDSL_8  0xc12
#define MECHA_CMD_DISC_MODE_DVDDL_8  0xc13
#define MECHA_CMD_DISC_MODE_DVDSL_12 0xc14
#define MECHA_CMD_DISC_MODE_DVDDL_12 0xc15
#define MECHA_CMD_DISC_DETECT        0xc16
#define MECHA_CMD_FOCUS_UPDOWN       0xc22
#define MECHA_CMD_FOCUS_AUTO_START   0xc23
#define MECHA_CMD_FOCUS_AUTO_STOP    0xc24
#define MECHA_CMD_FCS_SEARCH_CHECK   0xc25
#define MECHA_CMD_LASER_DIODE        0xc20
#define MECHA_CMD_TRACKING           0xc30
#define MECHA_CMD_SLED_CTL_MICRO     0xc41
#define MECHA_CMD_SLED_CTL_BIPHS     0xc42
#define MECHA_CMD_SLED_CTL_POS       0xc43
#define MECHA_CMD_SLED_POS_HOME      0xc44
#define MECHA_CMD_SLED_IN_SW         0xc45
#define MECHA_CMD_SP_CTL             0xc50
#define MECHA_CMD_SP_CLV_S           0xc51
#define MECHA_CMD_SP_CLV_A           0xc52
#define MECHA_CMD_TRAY               0xc60
#define MECHA_CMD_TRAY_SW            0xc61
#define MECHA_CMD_CLEAR_CONF         0xc8d
#define MECHA_CMD_UPLOAD_NEW         0xc8e //00
#define MECHA_CMD_UPLOAD_TO_RAM      0xc93
#define MECHA_CMD_DETECT_ADJ         0xc97
#define MECHA_CMD_WRITE_CHECKSUM     0xc99
#define MECHA_CMD_READ_CHECKSUM      0xc9a
#define MECHA_CMD_SETUP_OSD          0xc9b //00 = NTSC, 01 = PAL, presumed to be supported by DEX B and later.
#define MECHA_CMD_SETUP_SANYO        0xc9e
#define MECHA_CMD_AUTO_ADJ_ST_1      0xca1
#define MECHA_CMD_AUTO_ADJ_ST_2      0xca2
#define MECHA_CMD_AUTO_ADJ_ST_12     0xca3
#define MECHA_CMD_AUTO_ADJ_ST_2MD    0xca4 //Performed during skew adjustment initialization.
#define MECHA_CMD_AUTO_ADJ_FIX_GAIN  0xca5
#define MECHA_CMD_RFDC_LEVEL         0xca7
#define MECHA_CMD_TPP                0xca8
#define MECHA_CMD_MIRR_CHECK         0xcaa
#define MECHA_CMD_FE_OFFSET          0xcab
#define MECHA_CMD_CD_PLAY_1          0xcb0 //1x
#define MECHA_CMD_CD_PLAY_2          0xcb1 //2x
#define MECHA_CMD_CD_PLAY_3          0xcb2 //4x
#define MECHA_CMD_CD_PLAY_4          0xcb3 //5-12x
#define MECHA_CMD_CD_STOP            0xcb4
#define MECHA_CMD_CD_PAUSE           0xcb5
#define MECHA_CMD_CD_TRACK_CTL       0xcb6
#define MECHA_CMD_CD_TRACK_LONG_CTL  0xcb8
#define MECHA_CMD_CD_PLAY_5          0xcb9 //10-24x
#define MECHA_CMD_DVD_PLAY_1         0xcc0 //1x
#define MECHA_CMD_DVD_PLAY_2         0xcc1 //1.6x
#define MECHA_CMD_DVD_PLAY_3         0xcc2 //1.6-4x
#define MECHA_CMD_DVD_STOP           0xcc3
#define MECHA_CMD_DVD_PAUSE          0xcc4
#define MECHA_CMD_DVD_TRACK_CTL      0xcc5
#define MECHA_CMD_DVD_TRACK_LONG_CTL 0xcc7
#define MECHA_CMD_FOCUS_JUMP         0xcc8
#define MECHA_CMD_ADJ_AUTO_TILT      0xcca
#define MECHA_CMD_INIT_AUTO_TILT     0xccb
#define MECHA_CMD_MOV_AUTO_TILT      0xccd
#define MECHA_CMD_SET_DSP            0xcd1
#define MECHA_CMD_GAIN               0xcd3
#define MECHA_CMD_DSP_ERROR_RATE_CTL 0xcde
#define MECHA_CMD_DSP_ERROR_RATE     0xcdf
#define MECHA_CMD_EEPROM_WRITE       0xce0 //ce0aaaadddd	a = address, d = data.
#define MECHA_CMD_EEPROM_READ        0xce1 //ce1aaaa		a = address
#define MECHA_CMD_RTC_READ           0xce4
#define MECHA_CMD_RTC_WRITE          0xce5
#define MECHA_CMD_ECR_READ           0xce6
#define MECHA_CMD_ECR_WRITE          0xce7
#define MECHA_CMD_CD_ERROR           0xce8
#define MECHA_CMD_JITTER             0xce9
#define MECHA_CMD_FOCUS_JUMP_NEW     0xcf2
#define MECHA_CMD_CFA                0xcfa
#define MECHA_CMD_READ_MODEL_2       0xcfc
#define MECHA_CMD_READ_MODEL         0xcfd
#define MECHA_CMD_EEPROM_ERASE       0xcfe

#define MECHA_OP_SONY                0x00
#define MECHA_OP_SANYO               0x01

#define MECHA_LENS_T487              0x00
#define MECHA_LENS_T609K             0x01

#define MECHA_RTC_RICOH              0x00
#define MECHA_RTC_ROHM               0x01

/*	RTC
		SccCCssmmhhddDDMMYY
	Ricoh examples:
		0001047510201030222	No battery
		0001000030301030222	No battery
		0308823151803258401	RTC clear

	ss Seconds
	mm Minutes
	hh Hour
	dd Day of Week
	DD Day of Month
	MM Month (Ricoh: includes century bit; 0x80 -> '99 -> '00)
	YY Year

	Ricoh RS5C348AE2:
		cc: Unknown. Usually just "30" when healthy.
		CC: status 0x10 set -> No battery
		CC: status 0x40 set -> Battery down
	Rohm BU9861FV-WE2:
		cc: status 0x80 set -> No battery
		CC: Always 0x00
		
	ECR -> GOOD -> 019
*/

/*	MECHACON name: TestMode, MD
	00C10024 -> TestMode193 MD1.36
	
	TestMode.193	MD1.36 CXP101064-605R	-> Type 6
	TestMode.194	MD1.36 CXP101064-602R	-> Type 6
	TestMode.6		MD1.38 CXP102064-003R	-> Type 8
	TestMode.19		MD1.39 CXP102064-005R	-> Type 0
	TestMode.139	CXP103049-xxx for F-chassis
	
	Type (reg 0x10) 0xA809 -> Type 1
	Type (reg 0x10) 0xA829 -> Type 1
	Type (reg 0x10) 0xB009 -> Type (reg 0x10) 0xB029:
		MECHA 000603 -> Type 2
		MECHA 000803 -> Type 3	*/

enum MECHA_TYPE
{
    MECHA_TYPE_39 = 0, //A, AB, B, C, D (MD1.39)
    MECHA_TYPE_F  = 1, //F-chassis (MD1.39, x.3.0.0 - x.3.4.0)
    MECHA_TYPE_G  = 2, //G-chassis (MD1.39, x.3.6.0)
    MECHA_TYPE_G2 = 3, //G-chassis with newer MECHACON	 (MD1.39, x.3.8.0)
    MECHA_TYPE_40 = 4, //H/I-chassis (MD1.40)
    MECHA_TYPE_36 = 6, //A-chassis	(MD1.36)
    MECHA_TYPE_38 = 8, //A-chassis (MD1.38)
};

//B, C and D-chassis share the same MECHACON chip (CXP102064). B-chassis is differentiated from C and D because it's the only model with the auto-tilt motor.
#define MECHA_CHASSIS_DEX_A     0x0000
#define MECHA_CHASSIS_DEX_B     0x8c08
#define MECHA_CHASSIS_DEX_B_OLD 0x0c08 //Old ID for B-chassis DEX (no C-chassis DEX)
#define MECHA_CHASSIS_DEX_BD    0x8808 //Since B-chassis DEX end up with 0x8c08, this is likely only for D-chassis (no C-chassis CEX).
#define MECHA_CHASSIS_A         0x0001
#define MECHA_CHASSIS_AB        0x0801
#define MECHA_CHASSIS_B         0x8c09
#define MECHA_CHASSIS_BCD       0x8809 //Since B-chassis CEX end up with 0x8c09, this is likely only for C & D-chassis.
#define MECHA_CHASSIS_BC_OLD    0x0c09 //Old ID for B and C-chassis
#define MECHA_CHASSIS_F_SONY    0xa809
#define MECHA_CHASSIS_F_SANYO   0xa829
#define MECHA_CHASSIS_G_SONY    0xb009
#define MECHA_CHASSIS_G_SANYO   0xb029
#define MECHA_CHASSIS_H_SONY    0xb41b //Also used for the DEX H-chassis
#define MECHA_CHASSIS_H_SANYO   0xb43b

const char *MechaGetRtcStatusDesc(int type, int status);
const char *MechaGetRTCName(int rtc);
const char *MechaGetOPTypeName(int type);
const char *MechaGetLensTypeName(int type);
const char *MechaGetTVSystemDesc(int type);

void MechaGetTimeString(char *TimeString);

enum MECHA_CHASSIS_MODEL
{
    MECHA_CHASSIS_MODEL_SCPH_10000,
    MECHA_CHASSIS_MODEL_A,
    MECHA_CHASSIS_MODEL_AB,
    MECHA_CHASSIS_MODEL_B,
    MECHA_CHASSIS_MODEL_C,
    MECHA_CHASSIS_MODEL_D,
    MECHA_CHASSIS_MODEL_F,
    MECHA_CHASSIS_MODEL_G,
    MECHA_CHASSIS_MODEL_H,
    MECHA_CHASSIS_MODEL_DEXA,
    MECHA_CHASSIS_MODEL_DEXA2,
    MECHA_CHASSIS_MODEL_DEXA3,
    MECHA_CHASSIS_MODEL_DEXB,
    MECHA_CHASSIS_MODEL_DEXD,
    MECHA_CHASSIS_MODEL_DEXH,

    MECHA_CHASSIS_MODEL_COUNT
};
