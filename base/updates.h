/*  Define if the RTC should be updated according to the new ELECT tool's method:
    Always set the time if the RTC does not report an "all good" status.

    The old EEPROM tool does not ever update the RTC time, but has a check
    for the CTL1,2 error status (i.e. first 4 digits are "308C" instead of "3088").
    If it detects the CTL1,2 error, it seems to just send "ce53088" to clear the error. */
// #define UPDATE_RTC_NEW          1

// Update region status bits (all cleared = all updated/in order)
#define UPDATE_REGION_EEP_ECR   0x0001 // ECR data on the EEPROM
#define UPDATE_REGION_DISCDET   0x0002
#define UPDATE_REGION_SERVO     0x0004
#define UPDATE_REGION_TILT      0x0008
#define UPDATE_REGION_TRAY      0x0010
#define UPDATE_REGION_EEGS      0x0020
#define UPDATE_REGION_ECR       0x0040 // ECR data on the RTC
#define UPDATE_REGION_RTC       0x0080
#define UPDATE_REGION_RTC_CTL12 0x0100
#define UPDATE_REGION_RTC_TIME  0x0200
#define UPDATE_REGION_DEFAULTS  0x0800

// Update functions
int MechaUpdateChassisCex10000(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisA(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisAB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisC(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisD(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisF(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisG(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexA(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexA2(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexA3(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexB(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexD(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);

int MechaUpdateChassisH(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
int MechaUpdateChassisDexH(int ClearOSD2InitBit, int ReplacedMecha, int lens, int opt);
