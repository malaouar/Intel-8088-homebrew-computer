
TOOLS = arm-none-eabi

AS = $(TOOLS)-as
CC = $(TOOLS)-gcc -mcpu=cortex-m4 -mthumb -Ofast -std=gnu99
LD = $(TOOLS)-ld

OBJS =  crt0.o main.o sd.o vga.o rom.o cs.o clk.o ps2.o intr.o

all: vga.bin

vga.bin: vga.elf
	rm -f *.bin
	$(TOOLS)-objcopy vga.elf vga.bin -O binary
	rm -f *.elf

vga.elf: $(OBJS)
	$(LD) -T f411.ld -o vga.elf $(OBJS)
	rm -f *.o

crt0.o:	crt0.s
	$(AS) crt0.s -o crt0.o

%.o: %.c
	$(CC) -o $@ -c $<

clean:
	rm -f *.o vga.elf *.bin