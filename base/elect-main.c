#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "platform.h"
#include "mecha.h"
#include "eeprom.h"
#include "elect.h"
#include "main.h"

extern unsigned char ElectConIsT10K;

static int ElectPromptT10K(void)
{
    char input;
    do
    {
        PlatShowMessage("DTL-T10000 (YEDS-18)? [y,n] ");
        input = getchar();
        while (getchar() != '\n')
        {
        };
    } while (input != 'y' && input != 'n');

    return (input == 'y');
}

void MenuELECT(void)
{
    char choice;

    if (MechaInitModel() != 0)
    {
        DisplayConnHelp();
        return;
    }
    ElectConIsT10K = IsChassisDexA() ? ElectPromptT10K() : 0;

    do
    {
        PlatShowMessage("\nElectric Circuit Adjustment\n"
               "This tool allows you to re-calibrate the electric circuit of the CD/DVD drive.\n"
               "You need to do this if you:\n"
               "\t1. Change/remove the OPtical (OP) block\n"
               "\t2. Change/remove the spindle motor\n"
               "\t3. Change the MECHACON\n"
               "Warning! This process MAY damage the laser if the wrong type of disc is used!\n"
               "\nContinue with automatic ELECT adjustment? [y/n]");

        choice = getchar();
        while (getchar() != '\n')
        {
        };
    } while (choice != 'y' && choice != 'n');

    if (choice == 'y')
        ElectAutoAdjust();
}
