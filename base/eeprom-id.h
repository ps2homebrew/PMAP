int EEPROMInitID(void);
int MechaInitMechacon(int model, int IsDex);
int EEPROMNTSCPALDefaults(int vmode);

void EEPROMGetiLinkID(u8 *id);
void EEPROMGetConsoleID(u8 *id);
int EEPROMSetiLinkID(const u8 *NewiLinkID);
int EEPROMSetConsoleID(const u8 *NewConID);
int EEPROMSetModelName(const char *ModelName);
