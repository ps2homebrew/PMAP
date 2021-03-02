ELF = pmap
CFLAGS = -O2 -I.
OBJS = eeprom-id.o eeprom-main.o eeprom.o elect-main.o id-main.o main.o mecha-main.o mecha.o platform-linux.o updates.o

$(ELF): $(OBJS)
	$(CC) -o $(ELF) $(OBJS)

clean:
	rm -f $(ELF) $(OBJS)
