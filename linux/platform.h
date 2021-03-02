typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

int PlatformOpenCOMPort(const char *device);
int PlatformReadCOMPort(char *data, int n, unsigned short timeout);
int PlatformWriteCOMPort(const char *data);
void PlatformCloseCOMPort(void);
void PlatformSleep(unsigned short int msec);
void PlatformShowEMessage(const char *format, ...);
//Block until the user acknowledges.
void PlatformShowMessage(const char *format, ...);

//If necessary, provide these functions, otherwise define them to their equivalents
int pstricmp(const char *s1, const char *s2);
int pstrincmp(const char *s1, const char *s2, int len);
