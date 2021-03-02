typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

int PlatOpenCOMPort(const char *device);
int PlatReadCOMPort(char *data, int n, unsigned short timeout);
int PlatWriteCOMPort(const char *data);
void PlatCloseCOMPort(void);
void PlatSleep(unsigned short int msec);
void PlatShowEMessage(const char *format, ...);
void PlatShowMessage(const char *format, ...);
void PlatShowMessageB(const char *format, ...);

void PlatDebugInit(void);
void PlatDebugDeinit(void);
void PlatDPrintf(const char *format, ...);

//If necessary, provide these functions, otherwise define them to their equivalents
int pstricmp(const char *s1, const char *s2);
int pstrincmp(const char *s1, const char *s2, int len);
