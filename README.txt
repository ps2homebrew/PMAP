PlayStation 2 Mechacon Adjustment Program (PMAP)	- 2015/12/22
====================================================================

The PlayStation 2 Mechacon Adjustment Program (PMAP) is a tool for maintaining the PlayStation 2 CD/DVD subsystem.
SONY has its own official tools that aid in the maintanence process of their consoles. This program is a clone of their tools.

It offers the following functionality:
1. EEPROM maintenance and updating
2. Electrical circuit adjustment
3. Mechanism (skew) adjustment

The electrical circuit and mechanism adjustment functions offer jitter measurement, the jitter measurement is only a rough value.
Only real jitter measuring equipment can give an accurate measurement of jitter.

Test points on the PlayStation 2 mainboard:
-------------------------------------------
A) RS232C:
	TXD (JL610)
	RXD (JL611)
	+3.3V (JL613)
	GND (JL609)
	TEST MODE (Connect JL617 to GND)
B) RF-AMP circuit for DVD jitter meter
	RFAC (JL601)
	AGND (JL602, AGND must be used with RFAC).
C) Other test points:
	RFDC		(JL608)
	FE		(JL605)
	TE		(JL607)
	VC_+1.7V	(JL604)
	+8.5V		(JL430, JL431)
	GND		(JL432, JL433)

About Optical Block (OP) types:
-------------------------------
There are two types of optical blocks: SONY and SANYO.
The SONY OP has SONY branding on it, while the SANYO OP has no branding.
Only consoles starting from the F-chassis can support the SANYO OP.

If the OP block is changed, the console must be reconfigured to support the new OP block.

About Object Lens types:
------------------------
There are two types of lenses for the SONY OP: T487 and T609K.
The T609K has a light yellow object lens protector (a ring around the object lens), while the T487 has a white protector.

For the D-chassis, the first lots of the KHS-400B with the T609K lens had a white object lens protector,
with a violet marking at the adjustment screw on the side of the base of the optical block.
Some lenses may have black, blue or green markings, but that does not mean that it is a new lens type.

There is no support for a SANYO OP with a T609K lens, so it's probably safe to assume that such a thing does not exist.

If the lens/OP block is swapped, the console must be reconfigured to support the new lens.

Real-Time Clock (RTC) IC:
-------------------------

There are two types of RTC ICs on the F-chassis:
IC405	RS5C384AE2
IC416	BU9861FV-WE2

Importance of the button battery:
---------------------------------

There is a new BGA-based MechaCon on the F-chassis mainboard (CXP-103049-xxx).
The battery must be present, or the DVD-ROM circuit may not function correctly.

If the battery is removed or has run out, the RTC-ECR data on the RTC IC and EEPROM will be erased to 0.
This may result in the i.Link interface not working properly. Therefore, install a good battery before adjustment and do not remove it.

EEPROM management
-----------------

The PlayStation 2 has a EEPROM chip that contains various configuration segments:
1. Disc detect
2. Servo
3. Tilt (unused on all consoles that do not have an auto-tilt motor)
4. PS2ID (model name + iLink ID + Console ID)
5. Tray
6. DVD player
7. EEGS
8. OSD

This tool allows the EEPROM to be backed up and restored (up to G-chassis only), erased and for the defaults to be loaded.
The defaults for the SANYO OP (F-chassis and later) can also be loaded, allowing the OP to be changed to a SANYO OP. The MECHACON defaults is for a SONY OP.

Updates to the EEPROM parameters are also provided.

Warning! although the functionality is provided, do not erasese the EEPROM or load the defaults for the ID region!
This tool does not provide functionality to restore the IDs of the PlayStation 2.

Electrical circuit adjustment
-----------------------------

Adjustment of the electical circuit is done automatically. Simply follow the on-screen instructions to complete the adjustment procedure.
You will need the following discs:
CD test disc 		- SCD-2700 (YEDS-18 for DTL-T10000)
DVD-SL test disc	- HX-504
DVD-DL test disc	- HX-505

If unavailable, regular discs (CD, DVD-SL and DVD-DL) can be used as substitute for these discs, but the correct type of disc must be inserted.
Failing which, irrepairable damage to the console's optical block may result!
If unsure, please use the corresponding PlayStation 2 discs. They have to be in a good condition.

You have to do this, if you:
1. Changed or removed the OP block.
2. Changed or removed the spindle motor.
3. Changed the MECHACON IC.
4. Changed, erased or loaded the defaults to the EEPROM IC.

Mechanism (skew) adjustment
---------------------------

The mechanism adjustment involves two parts:
1. Tangential skew
2. Radial skew (automatic for B-chassis)

You need to do this, if you:
1. Changed or removed the OP block
2. Changed or removed the spindle motor

Procedure for mechanism (skew) adjustment:
1. Remove tray and move the tray mechanism to the close position.
2. Put a disc (GLD-DR01, or a DVD-SL disc) on the spindle motor with a chuck (clamp).
3. Do initialization (INIT SKEW).
4. Move the sled out (SLED OUT). Warning! the laser is now at the outer part of the disc!
5. Enter play mode (PLAY 1x).
6. Read jitter (JITTER 16 or JITTER 256) as you make adjustments.
	You will have to repeat both radial and tangential skew adjustments, until jitter is minimal (i.e. the sweet spot is found).
6a. Radial skew:
	i) If the console is a B-chassis, use the AUTO TILT motor adjustment function:
		*Automatically adjust radial skew with the ADJ command (TILT ADJ).
		*Stop play mode (PLAY STOP).
		*Move the sled back to the HOME position (SLED HOME) and enter PLAY mode again (PLAY 1x).
		*Write the new radial-skew settings (TILT WRITE)
	ii) If the console is a non-auto-tilt motor model:
		*Adjust the radial skew adjustment screw until jitter is minimal.
6b. Tangential skew:
	*Move the sled out (SLED OUT), if it was moved back to home position.
	*Adjust the tangential skew adjustment screw until jitter is minimal.
7. Stop play mode (PLAY STOP).
8. Move the sled back to the home position (SLED HOM).
9. Take the disc off and open the tray (TRAY OPEN).
10. Put the tray back on, and check that it can eject and retract properly.

Adjustment thresholds/targets:
------------------------------
CD:
	a.	CD DET:				SONY OP 600-1600, SANYO OP 750-1800 /  SONY OP: 660-1760, SANYO OP: 825-1980
	b.	FE Loop gain (K13):		0x10 - 0x60
	c.	TE Loop gain (K23):		0x10 - 0x60
DVD-SL:
	a.	FE Loop gain (K13):		0x10 - 0x60
	b.	TE Loop gain (K23):		0x10 - 0x60
	c.	Jitter (256):			0 - 0x1B00 (DEX: 0x1000, T10000: 0x1400) / G/H/I-chassis: 0x3E00 (DEX: 0x2970)
	d.	Error rate (PI+PO CC):		0 - 100
	e.	Error rate (PI NCC):		0
DVD-DL:
	a.	DVD-DL DISC DETECT:		0x15 (DVD-DL)
	a.	(L0) FE Loop gain (K13):	0x10 - 0x60
	b.	(L0) TE Loop gain (K23):	0x10 - 0x60
	c.	(L0) Jitter (256):		0 - 0x2300 (DEX: 0x1200) / G/H/I-chassis: 0 - 0x4C00 (DEX: 0x2D00)
	d.	(L1) FE Loop gain (K13):	0x10 - 0x60
	e.	(L1) TE Loop gain (K23):	0x10 - 0x60
	f.	(L1) Jitter (256):		0 - 0x2300 (DEX: 0x1200) / G/H/I-chassis: 0 - 0x4C00 (DEX: 0x2D00)
Disc Detect CD/DVD Ratio:			>= 1.80 / G/H/I-chassis: >=1.73
EEPROM Checksum:				0

Supported chassis models:
-------------------------
A-chassis:	SCPH-10000/SCPH-15000
B-chassis:	SCPH-30001
C-chassis:	SCPH-30001/2/3/4
D-chassis:	SCPH-30000/1/2/3/4, SCPH-35000/1/2/3/4, SCPH-30001/1/2/3/4R
F-chassis:	SCPH-30000, SCPH-30001/1/2/3/4/5/6/7R
G-chassis:	SCPH-39000/1/2/3/4/5/6/7/8,SCPH-37000L,SCPH-37000B
H/I-chassis:	SCPH-50000/1/2/3/4/5/6/7/8/9/10
A-chassis DEX:	DTL-H10000
A-chassis TOOL:	DTL-T10000(H)
B-chassis DEX:	DTL-H30001/2
D-chassis DEX:	DTL-H30000
H-chassis DEX:	DTL-H50000/1/2/6/8/9

Supported MD versions:
MD1.36 testmode.193 for CXP101064-605R, testmode.194 for CXP101064-602R
MD1.38 testmode.6 for CXP102064-003R
MD1.38 testmode.19 for CXP102064-005R
MD1.39 (CXP103049-xxx F/G-chassis)
MD1.40 (CXR706080-xxx H/I-chassis)

Known bugs and limitations:
---------------------------
1. There is currently no way to enter a new i.Link or console ID.
2. There is currently no way to enter a new model name.

There are no plans for adding support for ID management, as their only use is to evade DNAS.
