#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "main.h"

/*  Adjustment process:
    1. Remove tray (Not required for B-chassis)
    2. Put adjustment disc with chuck (clamp)
    3. Perform DVD-SL adjustment.
    4. Press OUT button and then the PLAY 1x button.
    5. Push 16 or 256-read buttons to read the jitter level.
    6. Perform radial and tangential skew adjustment.
        For B-chassis, enable auto-tilt radial skew adjustment (ADJ button).
    7. Stop jitter measurement.
    8. Stop play mode.
    9. Press HOME button and take out the disc.
    10. Put the tray back and check the eject mechanism. */

enum MECHA_ADJ_STATE
{
    MECHA_ADJ_STATE_NONE = 0,
    MECHA_ADJ_STATE_CD,
    MECHA_ADJ_STATE_CD_1,
    MECHA_ADJ_STATE_CD_2,
    MECHA_ADJ_STATE_CD_4,
    MECHA_ADJ_STATE_CD_512,
    MECHA_ADJ_STATE_CD_1024,
    MECHA_ADJ_STATE_CD_PAUSE,
    MECHA_ADJ_STATE_DVDSL,
    MECHA_ADJ_STATE_DVDSL_1,
    MECHA_ADJ_STATE_DVDSL_1p6,
    MECHA_ADJ_STATE_DVDSL_1p64,
    MECHA_ADJ_STATE_DVDSL_PAUSE,
    MECHA_ADJ_STATE_DVDDL,
    MECHA_ADJ_STATE_DVDDL_1,
    MECHA_ADJ_STATE_DVDDL_1p6,
    MECHA_ADJ_STATE_DVDDL_1p64,
    MECHA_ADJ_STATE_DVDDL_PAUSE,
};

struct DvdError
{
    u16 PICorrect;
    u16 PINCorrect;
    u16 PIMax;
    u16 POCorrect;
    u16 PONCorrect;
    u16 POMax;
    u16 jitter;
};

struct CdError
{
    u16 c1, c2;
};

extern unsigned char ConType;
static unsigned char ConIsT10K, status, SledIsAtHome, DiscDetect;
static unsigned short int DvdJitter, StepAmount;
static struct DvdError DvdError;
static struct CdError CdError;

typedef int (*MechaCmdFunction_t)(short int argc, char *argv[]);

#define MECHA_ADJ_MAX_ARGS   4
#define MECHA_ADJ_SYNTAX_ERR "Syntax error. For help, type HELP for help.\n"

static int MechaAdjInit(short int argc, char *argv[]);
static int MechaAdjHelp(short int argc, char *argv[]);
static int MechaAdjQuit(short int argc, char *argv[]);
static int MechaAdjSled(short int argc, char *argv[]);
static int MechaAdjPlay(short int argc, char *argv[]);
static int MechaAdjStop(short int argc, char *argv[]);
static int MechaAdjPause(short int argc, char *argv[]);
static int MechaAdjAutoTilt(short int argc, char *argv[]);
static int MechaAdjTray(short int argc, char *argv[]);
static int MechaAdjJitter(short int argc, char *argv[]);
static int MechaAdjGetError(short int argc, char *argv[]);

static int MechaTestDiscControl(short int argc, char *argv[]);
static int MechaTestLaserControl(short int argc, char *argv[]);
static int MechaTestServoControl(short int argc, char *argv[]);
static int MechaTestPlay(short int argc, char *argv[]);
static int MechaTestSpindle(short int argc, char *argv[]);
static int MechaTestHelp(short int argc, char *argv[]);

struct MechaDiagCommand
{
    const char *command;
    const char *syntax;
    const char *description;
    MechaCmdFunction_t function;
};

static int MechaAdjTxHandler(MechaTask_t *task)
{
    switch (task->tag)
    {
        case MECHA_CMD_TAG_ELECT_CD_TYPE:
            if (ConIsT10K)
            {
                task->command = MECHA_CMD_DISC_MODE_CD_12;
                task->label   = "DISC MODE CD 12cm";
            }
            return 0;
        case MECHA_CMD_TAG_MECHA_AUTO_TILT:
            if (!IsAutoTiltModel())
            {
                task->id      = 0;
                task->tag     = 0;
                task->command = 0;
            }
            return 0;
        case MECHA_CMD_TAG_MECHA_SET_DISC_TYPE:
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    printf("Disc type CD 12cm\n");
                    task->command = MECHA_CMD_DISC_MODE_CD_12;
                    status        = MECHA_ADJ_STATE_CD;
                    return 0;
                case DISC_TYPE_DVDS12:
                    printf("Disc type DVD-SL 12cm\n");
                    task->command = MECHA_CMD_DISC_MODE_DVDSL_12;
                    status        = MECHA_ADJ_STATE_DVDSL;
                    return 0;
                case DISC_TYPE_DVDD12:
                    printf("Disc type DVD-DL 12cm\n");
                    task->command = MECHA_CMD_DISC_MODE_DVDDL_12;
                    status        = MECHA_ADJ_STATE_DVDDL;
                    return 0;
                case DISC_TYPE_UNKNOWN:
                    printf("Disc type unknown.\n");
                    task->id      = 0;
                    task->tag     = 0;
                    task->command = 0;
                    status        = MECHA_ADJ_STATE_NONE;
                    return 0;
                case DISC_TYPE_NO_DISC:
                    printf("No disc inserted.\n");
                    task->id      = 0;
                    task->tag     = 0;
                    task->command = 0;
                    status        = MECHA_ADJ_STATE_NONE;
                    return 0;
                default:
                    status = MECHA_ADJ_STATE_NONE;
                    printf("Unsupported disc type: %02x\n", DiscDetect);
                    return 1;
            }
        default:
            return 0;
    }
}

static int MechaAdjSaveDvdErrorData(const char *data, int len)
{
    if (len == 29 && data[0] == '0')
    {
        return (sscanf(data, "0%04hx%04hx%04hx%04hx%04hx%04hx%04hx", &DvdError.PICorrect, &DvdError.PINCorrect, &DvdError.PIMax,
                       &DvdError.POCorrect, &DvdError.PONCorrect, &DvdError.POMax,
                       &DvdError.jitter) != 7);
    }
    else
        return 1;
}

static int MechaAdjSaveCdErrorData(const char *data, int len)
{
    if (len == 9 && data[0] == '0')
    {
        return (sscanf(data, "0%04hx%04hx", &CdError.c1, &CdError.c2) != 2);
    }
    else
        return 1;
}

static int MechaAdjSaveDiscDetectData(const char *data, int len)
{
    unsigned short int DiscDetectValue;

    if (len == 3 && data[0] == '0')
    {
        if (sscanf(data, "0%02hx", &DiscDetectValue) == 1)
        {
            DiscDetect = (unsigned char)DiscDetectValue;
            return 0;
        }
        else
            return 1;
    }
    else
        return 1;
}

static int MechaAdjRxHandler(MechaTask_t *task, const char *result, short int len)
{
    switch (result[0])
    {
        case '0': // Rx-OK
            switch (task->tag)
            {
                case MECHA_CMD_TAG_MECHA_DVD_ERROR_RATE:
                    return MechaAdjSaveDvdErrorData(result, len);
                case MECHA_CMD_TAG_MECHA_CD_ERROR_RATE:
                    return MechaAdjSaveCdErrorData(result, len);
                case MECHA_CMD_TAG_MECHA_DISC_DETECT:
                    return MechaAdjSaveDiscDetectData(result, len);
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

static const struct MechaDiagCommand AdjCommands[] = {
    {"INIT", "INIT <mode>", "Performs initialization. Modes:\n"
                            "\tCD\t- CD test mode.\n"
                            "\tDVD-SL\t- DVD-SL test mode.\n"
                            "\tDVD-DL\t- DVD-DL test mode.\n"
                            "\tSKEW\t- Skew adjustment (DVD-SL) mode.",
     &MechaAdjInit},
    {"SLED", "SLED <mode>", "Controls the sled. Modes:\n"
                            "\tIN\t\t- Moves the sled to the inner-most position.\n"
                            "\tHOME\t\t- Moves the sled to the home position.\n"
                            "\tMID\t\t- Moves the sled to the middle position.\n"
                            "\tOUT\t\t- Moves the sled to the outward-most position.\n"
                            "\tIN-SW\t\t- Displays the status of the in-switch.\n"
                            "\tSTEP-M IN/OUT\t- Moves the sled in/out by a number of micro-steps.\n"
                            "\tSTEP-B IN/OUT\t- Moves the sled in/out by a number of biph-steps.\n"
                            "\tSTEP <amount>\t- Sets the number of steps to move for each step command.\n"
                            "\tTRACKING ON/OFF\t- Switches on/off tracking.",
     &MechaAdjSled},
    {"PLAY", "PLAY <mode>", "Starts play mode Modes:\n"
                            "\tCD:\n"
                            "\t\t1\t- 1x mode\n"
                            "\t\t2\t- 2x mode\n"
                            "\t\t3\t- 4x mode\n"
                            "\t\t4\t- 5-12x mode\n"
                            "\t\t5\t- 10-24x mode\n"
                            "\tDVD:\n"
                            "\t\t1\t- 1x mode\n"
                            "\t\t2\t- 2x mode\n"
                            "\t\t3\t- 1.6x-4x mode\n"
                            "\t\tFJ\t- Focus Jump (DVD-DL only)",
     &MechaAdjPlay},
    {"STOP", "STOP", "Stops play mode.", &MechaAdjStop},
    {"PAUSE", "PAUSE", "Pauses play mode.", &MechaAdjPause},
    {"TILT", "TILT <mode>", "Controls the auto-tilt motor. Modes:\n"
                            "\tINIT\t\t- Initializes auto-tilt\n"
                            "\tFWD\t\t- Step the auto-tilt motor forward\n"
                            "\tREV\t\t- Step the auto-tilt motor in reverse\n"
                            "\tADJ\t\t- Auto-adjust radial skew\n"
                            "\tWRITE\t\t- Write new parameters",
     &MechaAdjAutoTilt},
    {"TRAY", "TRAY <mode>", "Controls the tray. Modes:\n"
                            "\tOPEN\t-Ejects the tray\n"
                            "\tCLOSE\t-Retracts the tray\n"
                            "\tIN-SW\t-Displays the status of the IN-switch\n"
                            "\tOUT-SW\t-Displays the status of the OUT-switch\n"
                            "NOTE: When ejecting/retracting the tray, the sled must be in the HOME position!\n",
     &MechaAdjTray},
    {"JITTER", "JITTER <mode>", "Gets jitter measurement. Modes: 1, 16, 256", &MechaAdjJitter},
    {"ERROR", "ERROR", "Gets error rate measurement. Modes:\n"
                       "\tDVD\t- Gets DVD error rate measurement.\n"
                       "\tCD\t- Gets CD error rate measurement.\n",
     &MechaAdjGetError},
    {"HELP", "HELP", "Displays this help message.\nType HELP <command> to get help on specific commands.\n"
                     "Entering a blank line will cause the previous command entered to be executed.",
     &MechaAdjHelp},
    {"QUIT", "QUIT", "Quits adjustment", &MechaAdjQuit},
    {NULL, NULL, NULL}};

static const struct MechaDiagCommand TestCommands[] = {
    {"DISC", "DISC <type>", "Sets up the circuit/disc mode:\n"
                            "\tCD\t\t- CD 12cm mode\n"
                            "\tDVD-SL\t- DVD-SL mode\n"
                            "\tDVD-DL\t- DVD-SL mode\n"
                            "\tAUTO*\t- Automatically detect and set the disc type\n"
                            "*CAUTION: Do not stare into the laser beam!",
     &MechaTestDiscControl},
    {"LASER", "LASER <mode>", "Controls the laser:\n"
                              "\tON*\t- Switches on the laser\n"
                              "\tOFF\t- Switches off the laser\n"
                              "\tFOCUS <mode>\t- controls the focus operation:\n"
                              "\t\tUD START\t- Starts UP/DOWN focus movement\n"
                              "\t\tUD STOP\t- Stop UP/DOWN focus movement\n"
                              "\t\tAUTOFCS START\t- Starts automatic focus\n"
                              "\t\tAUTOFCS STOP\t- Stop automatic focus\n"
                              "*CAUTION: Do not stare into the laser beam!",
     &MechaTestLaserControl},
    {"SERVO", "SERVO <mode>", "Controls the servo circuit:\n"
                              "\tAUTO*\t- Automatically adjust the servo circuit\n"
                              "*NOTE: sled must be at the home position",
     &MechaTestServoControl},
    {"SLED", "SLED <mode>", "Controls the sled. Modes:\n"
                            "\tIN\t\t- Moves the sled to the inner-most position.\n"
                            "\tHOME\t\t- Moves the sled to the home position.\n"
                            "\tMID\t\t- Moves the sled to the middle position.\n"
                            "\tOUT\t\t- Moves the sled to the outward-most position.\n"
                            "\tIN-SW\t\t- Displays the status of the in-switch.\n"
                            "\tSTEP-M IN/OUT\t- Moves the sled in/out by a number of micro-steps.\n"
                            "\tSTEP-B IN/OUT\t- Moves the sled in/out by a number of biph-steps.\n"
                            "\tSTEP <amount>\t- Sets the number of steps to move for each step command.\n"
                            "\tTRACKING ON/OFF\t- Switches on/off tracking.",
     &MechaAdjSled},
    {"PLAY", "PLAY <mode>", "Starts play mode. Modes:\n"
                            "\t1x\t- 1x mode\n"
                            "\tFWD\t- Forward by 1 track\n"
                            "\tREV\t- Reverse by 1 track\n"
                            "\tFWDL\t- Long forward by 1 track\n"
                            "\tREVL\t- Long reverse by 1 track\n"
                            "\tSTOP\t- Stop playing\n"
                            "\tFJ\t- Focus Jump (DVD-DL only)",
     &MechaTestPlay},
    {"TRAY", "TRAY <mode>", "Controls the tray. Modes:\n"
                            "\tOPEN*\t- Ejects the tray\n"
                            "\tCLOSE*\t- Retracts the tray\n"
                            "\tIN-SW\t- Displays the status of the IN-switch\n"
                            "\tOUT-SW\t- Displays the status of the OUT-switch\n"
                            "*NOTE: sled must be at the HOME position!\n",
     &MechaAdjTray},
    {"SPIND", "SPIND <mode>", "Controls the spindle motor\n"
                              "\tKICK\t- Causes the spindle motor to \"kick\"\n"
                              "\tBRAKE\t- Brakes the spindle motor\n"
                              "\tSTOP\t- Stops the spindle motor\n"
                              "\tCLV-S\t- Enters CLV-S mode\n"
                              "\tCLV-A\t- Enters CLV-A mode",
     &MechaTestSpindle},
    {"HELP", "HELP", "Displays this help message.\nType HELP <command> to get help on specific commands.\n"
                     "Entering a blank line will cause the previous command entered to be executed.",
     &MechaTestHelp},
    {"QUIT", "QUIT", "Quits testing", &MechaAdjQuit},
    {NULL, NULL, NULL}};

static int MechaAdjInit(short int argc, char *argv[])
{
    unsigned char id;

    /*  Mechanism (slew) adjustment
        DVD-SL (10-15s TimeOut) initialization:
            1. Sled home position
            2. DVD-SL mode
            3. (B-chassis only) Initialize AUTO-TILT motor
            4. Autogain 1+2 (No EEP WRITE)
            5. STOP
        DVD-DL initialization:
            1. Sled home position
            2. DVD-DL mode
            3. Autogain 1+2 (No EEP WRITE)
            4. STOP
        CD-ROM initialization:
            1. Sled home position
            2. CD-ROM mode
            3. Autogain 1+2 (No EEP WRITE)
            4. STOP */

    switch (ConType)
    {
        case MECHA_TYPE_36:
        case MECHA_TYPE_38:
        case MECHA_TYPE_39:
        case MECHA_TYPE_F:
        case MECHA_TYPE_G:
        case MECHA_TYPE_G2:
        case MECHA_TYPE_40:
            break;
        default:
            printf("MechaAdjInit: Unsupported chassis.\n");
            return 0;
    }

    id = 1;
    if (argc == 2)
    {
        switch (status)
        {
            case MECHA_ADJ_STATE_CD_PAUSE:
            case MECHA_ADJ_STATE_CD_1:
            case MECHA_ADJ_STATE_CD_2:
            case MECHA_ADJ_STATE_CD_4:
            case MECHA_ADJ_STATE_CD_512:
            case MECHA_ADJ_STATE_CD_1024:
            case MECHA_ADJ_STATE_DVDSL_PAUSE:
            case MECHA_ADJ_STATE_DVDSL_1:
            case MECHA_ADJ_STATE_DVDSL_1p6:
            case MECHA_ADJ_STATE_DVDSL_1p64:
            case MECHA_ADJ_STATE_DVDDL_PAUSE:
            case MECHA_ADJ_STATE_DVDDL_1:
            case MECHA_ADJ_STATE_DVDDL_1p6:
            case MECHA_ADJ_STATE_DVDDL_1p64:
                printf("Please STOP the drive first! Currently in another PLAY mode.\n");
                return 0;
        }

        if (!pstricmp(argv[1], "CD"))
        {
            MechaCommandAdd(MECHA_CMD_SLED_POS_HOME, NULL, id++, 0, 3000, "SLED HOME");
            MechaCommandAdd(MECHA_CMD_DISC_MODE_CD_8, NULL, id++, MECHA_CMD_TAG_MECHA_CD_TYPE, 1000, "DISC MODE CD 8cm");
            switch (ConType)
            { // TCD-732RA
                case MECHA_TYPE_F:
                case MECHA_TYPE_G:
                case MECHA_TYPE_G2:
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_FIX_GAIN, "0003", id++, 0, 20000, "CD ADJUSTMENT (FIX GAIN)");
                    break;
                case MECHA_TYPE_40: // TCD-732RA
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_12, "00", id++, 0, 20000, "CD AUTO ADJUSTMENT (1+2)");
                    break;
                case MECHA_TYPE_36:
                case MECHA_TYPE_38:
                case MECHA_TYPE_39: // TCD-732RA
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_1, "00", id++, 0, 20000, "CD AUTO ADJUSTMENT (STAGE 1)");
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_2, "00", id++, 0, 20000, "CD AUTO ADJUSTMENT (STAGE 2)");
                    break;
            }
            MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "00", id++, 0, 3000, "CD STOP");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("CD initialization failed.\n");
            else
                status = MECHA_ADJ_STATE_CD;

            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "DVD-SL"))
        {
            MechaCommandAdd(MECHA_CMD_SLED_POS_HOME, NULL, id++, 0, 3000, "SLED HOME");
            MechaCommandAdd(MECHA_CMD_DISC_MODE_DVDSL_12, NULL, id++, 0, 1000, "DISC MODE DVD-SL 12cm");
            MechaCommandAdd(MECHA_CMD_INIT_AUTO_TILT, NULL, id++, MECHA_CMD_TAG_MECHA_AUTO_TILT, 5000, "AUTO TILT INIT");
            switch (ConType)
            { // TDR-832/TDV-520CSC
                case MECHA_TYPE_40:
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_12, "00", id++, 0, 20000, "DVD-SL AUTO ADJUSTMENT (1+2)");
                    break;
                case MECHA_TYPE_36:
                case MECHA_TYPE_38:
                case MECHA_TYPE_39:
                case MECHA_TYPE_F:
                case MECHA_TYPE_G:
                case MECHA_TYPE_G2: // TDR-832
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_1, "00", id++, 0, 20000, "DVD-SL AUTO ADJUSTMENT (STAGE 1)");
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_2, "00", id++, 0, 20000, "DVD-SL AUTO ADJUSTMENT (STAGE 2)");
            }
            MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "00", id++, 0, 3000, "DVD-SL STOP");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("DVD-SL initialization failed.\n");
            else
                status = MECHA_ADJ_STATE_DVDSL;

            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "DVD-DL"))
        {
            MechaCommandAdd(MECHA_CMD_SLED_POS_HOME, NULL, id++, 0, 3000, "SLED HOME");
            MechaCommandAdd(MECHA_CMD_DISC_MODE_DVDDL_12, NULL, id++, 0, 1000, "DISC MODE DVD-DL 12cm");
            switch (ConType)
            { // TDV-540CSC
                case MECHA_TYPE_40:
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_12, "00", id++, 0, 20000, "DVD-DL AUTO ADJUSTMENT (1+2)");
                    break;
                case MECHA_TYPE_36:
                case MECHA_TYPE_38:
                case MECHA_TYPE_39:
                case MECHA_TYPE_F:
                case MECHA_TYPE_G:
                case MECHA_TYPE_G2: // HLX-505
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_1, "00", id++, 0, 20000, "DVD-DL AUTO ADJUSTMENT (STAGE 1)");
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_2, "00", id++, 0, 20000, "DVD-DL AUTO ADJUSTMENT (STAGE 2)");
            }
            MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "00", id++, 0, 3000, "DVD-DL STOP");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("DVD-DL initialization failed.\n");
            else
                status = MECHA_ADJ_STATE_DVDDL;

            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "SKEW"))
        {
            MechaCommandAdd(MECHA_CMD_SLED_POS_HOME, NULL, id++, 0, 3000, "SLED HOME");
            MechaCommandAdd(MECHA_CMD_DISC_MODE_DVDSL_12, NULL, id++, 0, 1000, "DISC MODE DVD-SL 12cm");
            MechaCommandAdd(MECHA_CMD_INIT_AUTO_TILT, NULL, id++, MECHA_CMD_TAG_MECHA_AUTO_TILT, 5000, "AUTO TILT INIT");
            switch (ConType)
            { // Test DVD-SL GLD-DR01
                case MECHA_TYPE_F:
                case MECHA_TYPE_G:
                case MECHA_TYPE_G2:
                case MECHA_TYPE_40:
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_FIX_GAIN, "0005", id++, 0, 20000, "DVD-SL ADJUSTMENT (FIX GAIN)");
                    break;
                case MECHA_TYPE_36:
                case MECHA_TYPE_38:
                case MECHA_TYPE_39:
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_1, "00", id++, 0, 20000, "DVD-SL AUTO ADJUSTMENT (STAGE 1)");
                    MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_2MD, "00", id++, 0, 20000, "DVD-SL AUTO ADJUSTMENT (STAGE 2MD)");
            }
            MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "00", id++, 0, 3000, "DVD-SL STOP");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("DVD-SL initialization failed.\n");
            else
                status = MECHA_ADJ_STATE_DVDSL;

            SledIsAtHome = 0;
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjPlay(short int argc, char *argv[])
{
    char buffer[8];
    u16 command, timeout;
    int speed, result;

    if (argc == 2)
    {
        switch (status)
        {
            case MECHA_ADJ_STATE_NONE:
                printf("Please do initialization first!\n");
                break;
            case MECHA_ADJ_STATE_DVDDL_PAUSE:
            case MECHA_ADJ_STATE_DVDDL_1:
            case MECHA_ADJ_STATE_DVDDL_1p6:
            case MECHA_ADJ_STATE_DVDDL_1p64:
                if (!pstricmp(argv[1], "FJ"))
                {
                    if ((result = MechaCommandExecute(MECHA_CMD_FOCUS_JUMP, 2000, "0300", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    break;
                }
            case MECHA_ADJ_STATE_DVDSL_PAUSE:
            case MECHA_ADJ_STATE_DVDSL_1:
            case MECHA_ADJ_STATE_DVDSL_1p6:
            case MECHA_ADJ_STATE_DVDSL_1p64:
            case MECHA_ADJ_STATE_CD_PAUSE:
            case MECHA_ADJ_STATE_CD_1:
            case MECHA_ADJ_STATE_CD_2:
            case MECHA_ADJ_STATE_CD_4:
            case MECHA_ADJ_STATE_CD_512:
            case MECHA_ADJ_STATE_CD_1024:
                printf("Please STOP the drive. It is currently in PLAY mode.\n");
                break;
            case MECHA_ADJ_STATE_CD:
                speed = (int)strtol(argv[1], NULL, 0);

                switch (speed)
                {
                    case 1:
                        timeout = 3000;
                        command = MECHA_CMD_CD_PLAY_1;
                        break;
                    case 2:
                        timeout = 3000;
                        command = MECHA_CMD_CD_PLAY_2;
                        break;
                    case 3:
                        timeout = 3000;
                        command = MECHA_CMD_CD_PLAY_3;
                        break;
                    case 4:
                        timeout = 4000;
                        command = MECHA_CMD_CD_PLAY_4;
                        break;
                    case 5:
                        timeout = 3000;
                        command = MECHA_CMD_CD_PLAY_5;
                        break;
                    default:
                        printf("Unsupported speed.\n");
                        timeout = 0;
                        command = 0;
                }

                if (command != 0)
                {
                    if ((result = MechaCommandExecute(command, timeout, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    else
                        status += speed;

                    SledIsAtHome = 0;
                }
                break;
            case MECHA_ADJ_STATE_DVDSL:
            case MECHA_ADJ_STATE_DVDDL:
                speed = (int)strtol(argv[1], NULL, 0);

                switch (speed)
                {
                    case 1:
                        timeout = 5000;
                        command = MECHA_CMD_DVD_PLAY_1;
                        break;
                    case 2:
                        timeout = 5000;
                        command = MECHA_CMD_DVD_PLAY_2;
                        break;
                    case 3:
                        timeout = 5000;
                        command = MECHA_CMD_DVD_PLAY_3;
                        break;
                    default:
                        printf("Unsupported speed.\n");
                        timeout = 0;
                        command = 0;
                }

                if (command != 0)
                {
                    if ((result = MechaCommandExecute(command, timeout, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    else
                        status += speed;

                    SledIsAtHome = 0;
                }
                break;
        }
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjStop(short int argc, char *argv[])
{
    char buffer[8];
    int result;

    switch (status)
    {
        case MECHA_ADJ_STATE_CD_PAUSE:
        case MECHA_ADJ_STATE_CD_1:
        case MECHA_ADJ_STATE_CD_2:
        case MECHA_ADJ_STATE_CD_4:
        case MECHA_ADJ_STATE_CD_512:
        case MECHA_ADJ_STATE_CD_1024:
            if ((result = MechaCommandExecute(MECHA_CMD_CD_STOP, 4000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            status = MECHA_ADJ_STATE_CD;
            break;
        case MECHA_ADJ_STATE_DVDSL_PAUSE:
        case MECHA_ADJ_STATE_DVDSL_1:
        case MECHA_ADJ_STATE_DVDSL_1p6:
        case MECHA_ADJ_STATE_DVDSL_1p64:
        case MECHA_ADJ_STATE_DVDDL_PAUSE:
        case MECHA_ADJ_STATE_DVDDL_1:
        case MECHA_ADJ_STATE_DVDDL_1p6:
        case MECHA_ADJ_STATE_DVDDL_1p64:
            if ((result = MechaCommandExecute(MECHA_CMD_DVD_STOP, 5000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);

            switch (status)
            {
                case MECHA_ADJ_STATE_DVDSL_PAUSE:
                case MECHA_ADJ_STATE_DVDSL_1:
                case MECHA_ADJ_STATE_DVDSL_1p6:
                case MECHA_ADJ_STATE_DVDSL_1p64:
                    status = MECHA_ADJ_STATE_DVDSL;
                    break;
                case MECHA_ADJ_STATE_DVDDL_PAUSE:
                case MECHA_ADJ_STATE_DVDDL_1:
                case MECHA_ADJ_STATE_DVDDL_1p6:
                case MECHA_ADJ_STATE_DVDDL_1p64:
                    status = MECHA_ADJ_STATE_DVDDL;
                    break;
            }
            break;
        default:
            printf("Not in PLAY mode.\n");
    }

    return 0;
}

static int MechaAdjAutoTilt(short int argc, char *argv[])
{
    unsigned char id;

    if (!IsAutoTiltModel())
    {
        printf("This is not a B-chassis (non-auto-tilt motor model).\n");
        return 0;
    }

    if (argc == 2)
    {
        id = 1;
        if (!pstricmp(argv[1], "INIT"))
        {
            MechaCommandAdd(MECHA_CMD_INIT_AUTO_TILT, NULL, id++, 0, 5000, "AUTO TILT INIT");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "ADJ"))
        {
            MechaCommandAdd(MECHA_CMD_ADJ_AUTO_TILT, "00", id++, 0, 15000, "TILT ADJUST");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "WRITE"))
        {
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "00c0003e", id++, 0, 6000, "EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "00c11140", id++, 0, 6000, "EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "00c21167", id++, 0, 6000, "EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "00c3012c", id++, 0, 6000, "EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_EEPROM_WRITE, "00c42805", id++, 0, 6000, "EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, 3000, "EEPROM WRITE CHECKSUM");
            MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, 0, 3000, "EEPROM READ CHECKSUM");
            MechaCommandAdd(MECHA_CMD_UPLOAD_TO_RAM, "04", id++, 0, 6000, "EEPROM TO RAM (TILT)");
            MechaCommandAdd(MECHA_CMD_ADJ_AUTO_TILT, "01", id++, 0, 15000, "TILT ADJUST WITH EEPROM WRITE");
            MechaCommandAdd(MECHA_CMD_WRITE_CHECKSUM, "00", id++, 0, 3000, "EEPROM WRITE CHECKSUM");
            MechaCommandAdd(MECHA_CMD_READ_CHECKSUM, "00", id++, 0, 3000, "EEPROM READ CHECKSUM");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "REV"))
        {
            MechaCommandAdd(MECHA_CMD_MOV_AUTO_TILT, "000001", id++, 0, 5000, "TILT ADJUST REV");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "FWD"))
        {
            MechaCommandAdd(MECHA_CMD_MOV_AUTO_TILT, "010001", id++, 0, 5000, "TILT ADJUST FWD");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjPause(short int argc, char *argv[])
{
    char buffer[8];
    int result;

    switch (status)
    {
        case MECHA_ADJ_STATE_CD_PAUSE:
        case MECHA_ADJ_STATE_DVDSL_PAUSE:
        case MECHA_ADJ_STATE_DVDDL_PAUSE:
            printf("Already paused.\n");
            break;
        case MECHA_ADJ_STATE_CD_1:
        case MECHA_ADJ_STATE_CD_2:
        case MECHA_ADJ_STATE_CD_4:
        case MECHA_ADJ_STATE_CD_512:
        case MECHA_ADJ_STATE_CD_1024:
            if ((result = MechaCommandExecute(MECHA_CMD_CD_PAUSE, 3000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            break;
        case MECHA_ADJ_STATE_DVDSL_1:
        case MECHA_ADJ_STATE_DVDSL_1p6:
        case MECHA_ADJ_STATE_DVDSL_1p64:
        case MECHA_ADJ_STATE_DVDDL_1:
        case MECHA_ADJ_STATE_DVDDL_1p6:
        case MECHA_ADJ_STATE_DVDDL_1p64:
            if ((result = MechaCommandExecute(MECHA_CMD_DVD_PAUSE, 5000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);

            switch (status)
            {
                case MECHA_ADJ_STATE_DVDSL_1:
                case MECHA_ADJ_STATE_DVDSL_1p6:
                case MECHA_ADJ_STATE_DVDSL_1p64:
                    status = MECHA_ADJ_STATE_DVDSL_PAUSE;
                    break;
                case MECHA_ADJ_STATE_DVDDL_1:
                case MECHA_ADJ_STATE_DVDDL_1p6:
                case MECHA_ADJ_STATE_DVDDL_1p64:
                    status = MECHA_ADJ_STATE_DVDDL_PAUSE;
                    break;
            }
            break;
        default:
            printf("Not in PLAY mode.\n");
    }

    return 0;
}

static int MechaAdjTray(short int argc, char *argv[])
{
    char buffer[8];
    int result;

    if (argc == 2)
    {
        if (!pstricmp(argv[1], "CLOSE"))
        {
            if (!SledIsAtHome)
            {
                printf("Sled must be in home position!\n");
                return 0;
            }

            if ((result = MechaCommandExecute(MECHA_CMD_TRAY, 6000, "00", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
        }
        else if (!pstricmp(argv[1], "OPEN"))
        {
            if (!SledIsAtHome)
            {
                printf("Sled must be in home position!\n");
                return 0;
            }

            if ((result = MechaCommandExecute(MECHA_CMD_TRAY, 6000, "01", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
        }
        else if (!pstricmp(argv[1], "IN-SW"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_TRAY_SW, 3000, "00", buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
                printf("IN-SW: %s\n", buffer);
        }
        else if (!pstricmp(argv[1], "OUT-SW"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_TRAY_SW, 3000, "01", buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
                printf("OUT-SW: %s\n", buffer);
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjJitter(short int argc, char *argv[])
{
    int result;
    char buffer[8];

    if (argc == 2)
    {
        if (!pstricmp(argv[1], "1"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_JITTER, 1000, "00", buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
                printf("%s\n", &buffer[1]);
        }
        else if (!pstricmp(argv[1], "16"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_JITTER, 1000, "02", buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
                printf("%s\n", &buffer[1]);
        }
        else if (!pstricmp(argv[1], "256"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_JITTER, 2000, "01", buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
                printf("%s\n", &buffer[1]);
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjGetError(short int argc, char *argv[])
{
    unsigned char id;

    id = 1;
    if (!pstricmp(argv[1], "DVD"))
    {
        switch (status)
        {
            case MECHA_ADJ_STATE_DVDSL_1:
            case MECHA_ADJ_STATE_DVDSL_1p6:
            case MECHA_ADJ_STATE_DVDSL_1p64:
            case MECHA_ADJ_STATE_DVDDL_1:
            case MECHA_ADJ_STATE_DVDDL_1p6:
            case MECHA_ADJ_STATE_DVDDL_1p64:
                MechaCommandAdd(MECHA_CMD_DSP_ERROR_RATE, "00", id++, MECHA_CMD_TAG_MECHA_DVD_ERROR_RATE, 2000, "DVD GET DSP ERROR RATE");
                if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) == 0)
                {
                    printf("DVD Error Data:\n"
                           "PI Correct:\t\t%#04x\tPO Correct:\t\t%#04x\n"
                           "PI Non-correct:\t%#04x\tPO Non-correct:\t%#04x\n"
                           "PI Max:\t\t\t%#04x\tPO Max:\t\t\t%#04x\n"
                           "Jitter:\t\t\t%#04x\n",
                           DvdError.PICorrect, DvdError.PINCorrect, DvdError.PIMax,
                           DvdError.POCorrect, DvdError.PONCorrect, DvdError.POMax,
                           DvdError.jitter);
                }
                else
                    printf("Failed to execute.\n");
                break;
            default:
                printf("Not in a DVD PLAY mode.\n");
        }
    }
    else if (!pstricmp(argv[1], "CD"))
    {
        switch (status)
        {
            case MECHA_ADJ_STATE_CD_1:
            case MECHA_ADJ_STATE_CD_2:
            case MECHA_ADJ_STATE_CD_4:
            case MECHA_ADJ_STATE_CD_512:
            case MECHA_ADJ_STATE_CD_1024:
                MechaCommandAdd(MECHA_CMD_CD_ERROR, "00", id++, MECHA_CMD_TAG_MECHA_CD_ERROR_RATE, 2000, "CD GET DSP ERROR RATE");
                if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) == 0)
                {
                    printf("%04x:::%04x\n", CdError.c1, CdError.c2);
                }
                else
                    printf("Failed to execute.\n");
                break;
            default:
                printf("Not in a CD PLAY mode.\n");
        }
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaAdjSled(short int argc, char *argv[])
{
    int result;
    char buffer[8], args[8];

    if (argc >= 2)
    {
        if (!pstricmp(argv[1], "HOME"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_SLED_POS_HOME, 3000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            else
                SledIsAtHome = 1;
        }
        else if (!pstricmp(argv[1], "IN"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_POS, 2000, "00", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "OUT"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_POS, 3000, "02", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "MID"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_POS, 3000, "01", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            SledIsAtHome = 0;
        }
        else if (!pstricmp(argv[1], "STEP-M"))
        {
            if (argc == 3)
            {
                if (!pstricmp(argv[2], "IN"))
                { // Micro reverse
                    snprintf(args, 7, "00%04x", StepAmount);
                    if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_MICRO, 2000, args, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    SledIsAtHome = 0;
                }
                else if (!pstricmp(argv[2], "OUT"))
                { // Micro forward
                    snprintf(args, 7, "01%04x", StepAmount);
                    if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_MICRO, 2000, "010064", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    SledIsAtHome = 0;
                }
                else
                    return -EINVAL;
            }
            else
                return -EINVAL;
        }
        else if (!pstricmp(argv[1], "STEP-B"))
        {
            if (argc == 3)
            {
                if (!pstricmp(argv[2], "IN"))
                { // Biphs reverse
                    snprintf(args, 7, "00%04x", StepAmount);
                    if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_BIPHS, 2000, args, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    SledIsAtHome = 0;
                }
                else if (!pstricmp(argv[2], "OUT"))
                { // Biphs forward
                    snprintf(args, 7, "01%04x", StepAmount);
                    if ((result = MechaCommandExecute(MECHA_CMD_SLED_CTL_BIPHS, 2000, args, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                    SledIsAtHome = 0;
                }
                else
                    return -EINVAL;
            }
            else
                return -EINVAL;
        }
        else if (!pstricmp(argv[1], "TRACKING"))
        {
            if (argc == 3)
            {
                if (!pstricmp(argv[2], "ON"))
                {
                    if ((result = MechaCommandExecute(MECHA_CMD_TRACKING, 1000, "01", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                }
                else if (!pstricmp(argv[2], "OFF"))
                {
                    if ((result = MechaCommandExecute(MECHA_CMD_TRACKING, 1000, "00", buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                        printf("Error %d\n", result);
                }
                else
                    return -EINVAL;
            }
            else
                return -EINVAL;
        }
        else if (!pstricmp(argv[1], "IN-SW"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_SLED_IN_SW, 1000, NULL, buffer, sizeof(buffer))) < 0)
                printf("Error %d\n", result);
            else
            {
                result = (int)strtoul(&buffer[1], NULL, 16);
                printf("IN-SW: %02x\n", result);
            }
        }
        else if (!pstricmp(argv[1], "STEP"))
        {
            if (argc == 3)
            {
                StepAmount = (unsigned short int)strtoul(argv[2], NULL, 0);
                printf("STEP: %u\n", StepAmount);
            }
            else
                return -EINVAL;
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int DisplayHelp(const struct MechaDiagCommand *commands, short int argc, char *argv[])
{
    int i;
    const struct MechaDiagCommand *pCmd;

    if (argc == 1)
    {
        printf("To get help for a specific command, type HELP <command>\n"
               "Available commands:\n");
        for (pCmd = commands, i = 0; pCmd->command != NULL; pCmd++, i++)
        {
            printf("\t%s%c", pCmd->command, ((i != 0) && (i % 4) == 0) ? '\n' : ' ');
        }
        putchar('\n');
    }
    else if (argc == 2)
    {
        for (pCmd = commands; pCmd->command != NULL; pCmd++)
        {
            if (!pstricmp(pCmd->command, argv[1]))
            {
                printf("%s - %s\n", pCmd->syntax, pCmd->description);
                break;
            }
        }

        if (pCmd->command == NULL)
            printf("No such command.\n");
    }
    else
    {
        return -EINVAL;
    }

    return 0;
}

static int MechaAdjHelp(short int argc, char *argv[])
{
    return DisplayHelp(AdjCommands, argc, argv);
}

static int MechaTestHelp(short int argc, char *argv[])
{
    return DisplayHelp(TestCommands, argc, argv);
}

static int MechaAdjQuit(short int argc, char *argv[])
{
    return 1;
}

static int MechaTestDiscControl(short int argc, char *argv[])
{
    u8 id;
    int result;
    char buffer[8];

    if (argc == 2)
    {
        id = 1;
        if (!pstricmp(argv[1], "CD"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_DISC_MODE_CD_12, 1000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            else
            {
                status     = MECHA_ADJ_STATE_CD;
                DiscDetect = DISC_TYPE_CD12;
            }
        }
        else if (!pstricmp(argv[1], "DVD-SL"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_DISC_MODE_DVDSL_12, 1000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            else
            {
                status     = MECHA_ADJ_STATE_DVDSL;
                DiscDetect = DISC_TYPE_DVDS12;
            }
        }
        else if (!pstricmp(argv[1], "DVD-DL"))
        {
            if ((result = MechaCommandExecute(MECHA_CMD_DISC_MODE_DVDDL_12, 1000, NULL, buffer, sizeof(buffer))) < 0 || (result = strtoul(buffer, NULL, 16)) != 0)
                printf("Error %d\n", result);
            else
            {
                status     = MECHA_ADJ_STATE_DVDDL;
                DiscDetect = DISC_TYPE_DVDD12;
            }
        }
        else if (!pstricmp(argv[1], "AUTO"))
        {
            MechaCommandAdd(MECHA_CMD_DISC_DETECT, NULL, id++, MECHA_CMD_TAG_MECHA_DISC_DETECT, 3000, "DISC DETECT");
            MechaCommandAdd(MECHA_CMD_DISC_MODE_CD_8, NULL, id++, MECHA_CMD_TAG_MECHA_SET_DISC_TYPE, 1000, "DISC MODE CD 8cm");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaTestLaserControl(short int argc, char *argv[])
{
    unsigned char id;

    id = 1;
    if (argc == 2)
    {
        if (!pstricmp(argv[1], "ON"))
        {
            MechaCommandAdd(MECHA_CMD_LASER_DIODE, "01", id++, 0, 3000, "LD ON");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "OFF"))
        {
            MechaCommandAdd(MECHA_CMD_LASER_DIODE, "00", id++, 0, 3000, "LD OFF");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else
            return -EINVAL;
    }
    else if (argc >= 3)
    {
        if (!pstricmp(argv[1], "FOCUS"))
        {
            if (argc == 4)
            {
                if (!pstricmp(argv[2], "UD"))
                {
                    if (!pstricmp(argv[3], "START"))
                    {
                        MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "01", id++, 0, 3000, "FOCUS UP/DOWN START");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else if (!pstricmp(argv[3], "STOP"))
                    {
                        MechaCommandAdd(MECHA_CMD_FOCUS_UPDOWN, "00", id++, 0, 3000, "FOCUS UP/DOWN STOP");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        return -EINVAL;
                }
                else if (!pstricmp(argv[2], "AUTOFCS"))
                {
                    if (!pstricmp(argv[3], "START"))
                    {
                        MechaCommandAdd(MECHA_CMD_FOCUS_AUTO_START, NULL, id++, 0, 3000, "AUTO FOCUS START");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else if (!pstricmp(argv[3], "STOP"))
                    {
                        MechaCommandAdd(MECHA_CMD_FOCUS_AUTO_STOP, NULL, id++, 0, 3000, "AUTO FOCUS STOP");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        return -EINVAL;
                }
                else
                    return -EINVAL;
            }
            else
                return -EINVAL;
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaTestServoControl(short int argc, char *argv[])
{
    unsigned char id;

    if (DiscDetect == 0xFF)
    {
        printf("Disc type/circuit not set up!\n");
        return 0;
    }

    if (argc == 2)
    {
        id = 1;
        if (!pstricmp(argv[1], "AUTO"))
        {
            MechaCommandAdd(MECHA_CMD_SLED_POS_HOME, NULL, id++, 0, 3000, "SLED HOME");
            MechaCommandAdd(MECHA_CMD_AUTO_ADJ_ST_12, "00", id++, 0, 40000, "SERVO AUTO ADJ START");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaTestPlay(short int argc, char *argv[])
{
    unsigned char id;

    if (DiscDetect == 0xFF)
    {
        printf("Disc type/circuit not set up!\n");
        return 0;
    }

    if (argc == 2)
    {
        id = 1;
        if (!pstricmp(argv[1], "1x"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    MechaCommandAdd(MECHA_CMD_CD_PLAY_1, NULL, id++, 0, 3000, "PLAY CD 12cm");
                    if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                        printf("Failed to execute.\n");
                    else
                        status = MECHA_ADJ_STATE_CD_1;
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    MechaCommandAdd(MECHA_CMD_DVD_PLAY_1, NULL, id++, 0, 5000, "PLAY DVD 12cm");
                    if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                        printf("Failed to execute.\n");
                    else
                    {
                        switch (DiscDetect)
                        {
                            case DISC_TYPE_DVDS12:
                                status = MECHA_ADJ_STATE_DVDSL_1;
                                break;
                            case DISC_TYPE_DVDD12:
                                status = MECHA_ADJ_STATE_DVDDL_1;
                        }
                    }
                    break;
            }
        }
        else if (!pstricmp(argv[1], "FWD"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    if (status == MECHA_ADJ_STATE_CD_1)
                    {
                        MechaCommandAdd(MECHA_CMD_CD_TRACK_CTL, "01000A", id++, 0, 5000, "CD FWD 1 TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    if (status == MECHA_ADJ_STATE_DVDSL_1 || status == MECHA_ADJ_STATE_DVDDL_1)
                    {
                        MechaCommandAdd(MECHA_CMD_DVD_TRACK_CTL, "01000A", id++, 0, 5000, "DVD FWD 1 TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
            }
        }
        else if (!pstricmp(argv[1], "REV"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    if (status == MECHA_ADJ_STATE_CD_1)
                    {
                        MechaCommandAdd(MECHA_CMD_CD_TRACK_CTL, "00000A", id++, 0, 5000, "CD REV 1 TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    if (status == MECHA_ADJ_STATE_DVDSL_1 || status == MECHA_ADJ_STATE_DVDDL_1)
                    {
                        MechaCommandAdd(MECHA_CMD_DVD_TRACK_CTL, "00000A", id++, 0, 5000, "DVD REV 1 TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
            }
        }
        else if (!pstricmp(argv[1], "FWDL"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    if (status == MECHA_ADJ_STATE_CD_1)
                    {
                        MechaCommandAdd(MECHA_CMD_CD_TRACK_LONG_CTL, "010001", id++, 0, 10000, "CD FWD LONG TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    if (status == MECHA_ADJ_STATE_DVDSL_1 || status == MECHA_ADJ_STATE_DVDDL_1)
                    {
                        MechaCommandAdd(MECHA_CMD_DVD_TRACK_LONG_CTL, "010001", id++, 0, 10000, "DVD FWD LONG TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
            }
        }
        else if (!pstricmp(argv[1], "REVL"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    if (status == MECHA_ADJ_STATE_CD_1)
                    {
                        MechaCommandAdd(MECHA_CMD_CD_TRACK_LONG_CTL, "000001", id++, 0, 10000, "CD REV LONG TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    if (status == MECHA_ADJ_STATE_DVDSL_1 || status == MECHA_ADJ_STATE_DVDDL_1)
                    {
                        MechaCommandAdd(MECHA_CMD_DVD_TRACK_LONG_CTL, "000001", id++, 0, 10000, "DVD REV LONG TRACK");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");
                    }
                    else
                        printf("Not in PLAY mode.\n");
            }
        }
        else if (!pstricmp(argv[1], "STOP"))
        {
            switch (DiscDetect)
            {
                case DISC_TYPE_CD12:
                    if (status == MECHA_ADJ_STATE_CD_1)
                    {
                        MechaCommandAdd(MECHA_CMD_CD_STOP, NULL, id++, 0, 20000, "CD STOP");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");

                        status = MECHA_ADJ_STATE_CD;
                    }
                    else
                        printf("Not in PLAY mode.\n");
                    break;
                case DISC_TYPE_DVDS12:
                case DISC_TYPE_DVDD12:
                    if (status == MECHA_ADJ_STATE_DVDSL_1 || status == MECHA_ADJ_STATE_DVDDL_1)
                    {
                        MechaCommandAdd(MECHA_CMD_DVD_STOP, NULL, id++, 0, 20000, "DVD STOP");
                        if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                            printf("Failed to execute.\n");

                        switch (status)
                        {
                            case MECHA_ADJ_STATE_DVDSL_1:
                                status = MECHA_ADJ_STATE_DVDSL;
                                break;
                            case MECHA_ADJ_STATE_DVDDL_1:
                                status = MECHA_ADJ_STATE_DVDDL;
                        }
                    }
                    else
                        printf("Not in PLAY mode.\n");
            }
        }
        else if (!pstricmp(argv[1], "FJ"))
        {
            if (DiscDetect == DISC_TYPE_DVDD12)
            {
                if (status == MECHA_ADJ_STATE_DVDDL_1)
                {
                    MechaCommandAdd(MECHA_CMD_FOCUS_JUMP, "0205", id++, 0, 2000, "DVD-DL FOCUS JUMP");
                    if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                        printf("Failed to execute.\n");
                }
                else
                    printf("Not in PLAY mode.\n");
            }
            else
                printf("Not a DVD-DL.\n");
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static int MechaTestSpindle(short int argc, char *argv[])
{
    unsigned char id;

    if (argc == 2)
    {
        id = 1;
        if (!pstricmp(argv[1], "KICK"))
        {
            MechaCommandAdd(MECHA_CMD_SP_CTL, "01", id++, 0, 3000, "SP KICK");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "BRAKE"))
        {
            MechaCommandAdd(MECHA_CMD_SP_CTL, "00", id++, 0, 3000, "SP BRAKE");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "STOP"))
        {
            MechaCommandAdd(MECHA_CMD_SP_CTL, "02", id++, 0, 3000, "SP STOP");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "CLV-S"))
        {
            MechaCommandAdd(MECHA_CMD_SP_CLV_S, NULL, id++, 0, 3000, "SP CLV-S");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else if (!pstricmp(argv[1], "CLV-A"))
        {
            MechaCommandAdd(MECHA_CMD_SP_CLV_A, NULL, id++, 0, 3000, "SP CLV-A");
            if (MechaCommandExecuteList(&MechaAdjTxHandler, &MechaAdjRxHandler) != 0)
                printf("Failed to execute.\n");
        }
        else
            return -EINVAL;
    }
    else
        return -EINVAL;

    return 0;
}

static void MechaCommonMain(const struct MechaDiagCommand *commands, char prompt)
{
    int result;
    u8 tm, md;
    const struct MechaDiagCommand *pCmd;
    unsigned char done, argc;
    char input[128], previous[128], *argv[MECHA_ADJ_MAX_ARGS], *pTok;

    MechaGetMode(&tm, &md);
    status       = MECHA_ADJ_STATE_NONE;
    StepAmount   = 100;
    SledIsAtHome = 0;
    DiscDetect   = 0xFF;
    done         = 0;
    previous[0]  = '\0';
    do
    {
        printf("MD1.%d %c> ", md, prompt);
        if (fgets(input, sizeof(input), stdin))
        {
            input[strlen(input) - 1] = '\0';
            if (input[0] != '\0')
                strcpy(previous, input);
            else
                strcpy(input, previous);

            argv[0] = NULL;
            for (argc = 0, pTok = strtok(input, " "); pTok != NULL && argc < MECHA_ADJ_MAX_ARGS; argc++)
            {
                argv[argc] = pTok;
                pTok       = strtok(NULL, " ");
            }

            for (pCmd = commands; pCmd->command != NULL; pCmd++)
            {
                if (!pstricmp(pCmd->command, argv[0]))
                    break;
            }

            if (pCmd->command != NULL)
            {
                if ((result = pCmd->function(argc, argv)) < 0)
                {
                    if (result == -EINVAL)
                        printf(MECHA_ADJ_SYNTAX_ERR);
                }
                else
                {
                    done = result;
                }
            }
            else
                printf("Unrecognized command. For help, type HELP.\n");
        }
    } while (!done);
}

static void MechaAdjMain(void)
{
    MechaCommonMain(AdjCommands, 'A');
}

static void MechaTestMain(void)
{
    MechaCommonMain(TestCommands, 'T');
}

static int MechaAdjPromptT10K(void)
{
    char input;
    do
    {
        printf("Is this a DTL-T10000? [y,n] ");
        input = getchar();
        while (getchar() != '\n')
        {
        };
    } while (input != 'y' && input != 'n');

    return (input == 'y');
}

void MenuMECHA(void)
{
    int input;
    unsigned char done;
    char choice;

    if (MechaInitModel() != 0)
    {
        DisplayConnHelp();
        return;
    }
    if (IsOutdatedBCModel())
    {
        printf("B/C-chassis: EEPROM update required.\n");
        return;
    }

    do
    {
        printf("\nMechanics (skew) Adjustment\n"
               "This tool allows you to re-calibrate and test the mechanics (skew) of the CD/DVD drive.\n"
               "You need to do this if you:\n"
               "\t1. Change/remove the OPtical (OP) block\n"
               "\t2. Change/remove the spindle motor\n"
               "Warning! This process MAY damage the laser if the wrong type of disc is used!\n"
               "\nContinue with MECHA adjustment? [y/n]");

        choice = getchar();
        while (getchar() != '\n')
        {
        };
    } while (choice != 'y' && choice != 'n');

    if (choice == 'y')
    {
        ConIsT10K = IsChassisDexA() ? MechaAdjPromptT10K() : 0;

        done      = 0;
        do
        {
            do
            {
                printf("\nMechanics (skew) Adjustment\n"
                       "\t1. Adjust mechanics\n"
                       "\t2. Test mechanics\n"
                       "\t3. Quit\n"
                       "Your choice: ");
                input = 0;
                if (scanf("%d", &input) > 0)
                    while (getchar() != '\n')
                    {
                    };
            } while (input < 1 || input > 3);

            switch (input)
            {
                case 1:
                    MechaAdjMain();
                    break;
                case 2:
                    MechaTestMain();
                    break;
                default:
                    done = 1;
            }
        } while (!done);
    }
}
