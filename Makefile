
CC      = arm-none-linux-gnueabi-gcc
LD      = arm-none-linux-gnueabi-ld
AR      = arm-none-linux-gnueabi-ar
OBJCOPY = arm-none-linux-gnueabi-objcopy
OBJDUMP = arm-none-linux-gnueabi-objdump

CFLAGS 		:= -I -nostdlib -Wall -O2 -fno-builtin
CPPFLAGS   	:= -I$(INCLUDEDIR)

objs := start.o init.o main.o

boot.bin: $(objs)
	${LD} -Tboot.lds -o boot.elf $^
	${OBJCOPY} -O binary -S boot.elf $@
	${OBJDUMP} -D -m arm boot.elf > boot.dis

%.o:%.c
	${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o:%.S
	${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.bin *.elf *.dis
	

