/*  Elect adjustment:
    1. CD (SCD-2700/YEDS-18)
        a. Tray close
        b. Sled home position
        c. Disc detect CD write.
        d. CD mode
        e. Autogain stage 1 write.
        f. Autogain stage 2 write.
        g. Check result of adjustment.
        h. Stop
        i. Tray open
    2. DVD-SL (HLX-504)
        a. Tray close.
        b. Sled home position.
        c. Disc detect DVD-SL write.
        d. Disc detect EEPROM write
        e. Disc detect RAM write
        f. DVD-SL mode
        g. Autogain stage 1 write.
        h. Autogain stage 2 write.
        i. Check result of adjustment 1.
        j. Check result of adjustment 2.
        k. Stop
        l. Tray open
    3. DVD-DL (HLX-505)
        a. Tray close.
        b. Sled home position.
        c. Disc detect DVD-DL write.
        d. Disc detect EEPROM write
        e. Disc detect RAM write
        f. DVD-DL mode
        g. Autogain stage 1 write.
        h. Autogain stage 2 write.
        i. Check result of adjustment.
        j. Stop
    4. Checksum adjustment
        a. Checksum EEPROM Write
        b. Checksum EEPROM Read
        c. Check result of adjustment.
        d. Tray open.

    Adjustment data:
        Model & serial number
        CD:
            a.    CD DET:                        SONY OP 600-1600, SANYO OP 750-1800 /  SONY OP: 660-1760, SANYO OP: 825-1980
            b.    FE Loop gain (K13):            0x10 - 0x60
            c.    TE Loop gain (K23):            0x10 - 0x60
        DVD-SL:
            a.    FE Loop gain (K13):            0x10 - 0x60
            b.    TE Loop gain (K23):            0x10 - 0x60
            c.    Jitter (256):                  0 - 0x1B00 (DEX: 0x1000, T10000: 0x1400) / G/H/I-chassis: 0x3E00 (DEX: 0x2970)
            d.    Error rate (PI+PO CC):         0 - 100
            e.    Error rate (PI NCC):           0
        DVD-DL:
            a.    DVD-DL DISC DETECT:            0x15 (DVD-DL)
            a.    (L0) FE Loop gain (K13):       0x10 - 0x60
            b.    (L0) TE Loop gain (K23):       0x10 - 0x60
            c.    (L0) Jitter (256):             0 - 0x2300 (DEX: 0x1200) / G/H/I-chassis: 0 - 0x4C00 (DEX: 0x2D00)
            d.    (L1) FE Loop gain (K13):       0x10 - 0x60
            e.    (L1) TE Loop gain (K23):       0x10 - 0x60
            f.    (L1) Jitter (256):             0 - 0x2300 (DEX: 0x1200) / G/H/I-chassis: 0 - 0x4C00 (DEX: 0x2D00)
        Disc Detect CD/DVD Ratio:  >= 1.80 / G/H/I-chassis: >=1.73
        EEPROM Checksum:                         0 */

int ElectAutoAdjust(void);
