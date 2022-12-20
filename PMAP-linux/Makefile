VPATH = ./:../base/

ELF = pmap
CFLAGS = -O2 -I.
OBJS += eeprom-main.o eeprom.o elect.o elect-main.o mecha-main.o mecha.o updates.o platform-linux.o
OBJS += ../PMAP/main.o
# OBJS += eeprom-id.o id-main.o

$(ELF): $(OBJS)
	$(CC) -o $(ELF) $(OBJS)

clean:
	rm -f $(ELF) $(OBJS)
