VPATH = ./:../base/

ELF = pmap
CFLAGS ?= -O2
CPPFLAGS = -I.
OBJS += eeprom-main.o eeprom.o elect.o elect-main.o mecha-main.o mecha.o updates.o platform-unix.o
OBJS += main.o
# Add -DID_MANAGEMENT when ID_MANAGEMENT is defined
ifdef ID_MANAGEMENT
CPPFLAGS += -DID_MANAGEMENT
OBJS += eeprom-id.o id-main.o
endif

$(ELF): $(OBJS)
	$(CC) -o $(ELF) $(OBJS)

clean:
	rm -f $(ELF) $(OBJS) eeprom-id.o id-main.o
