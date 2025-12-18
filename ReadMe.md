A minimal Intel 8088 based homebrew computer almost equivalent to the IBM PC 5150 from the early 1980's era.
Only 5 ICs (8088 + two 74LS373 + 74LS00 + 512kB RAM) and a BlakPill board. It's perhaps the simplest 8088 SBC you can DIY thanks to using a microcontroller (STM32F411CE) to acheive the most of hardware functions.
We can use a transistor as inverter and omit the 74LS00 so we need only 4 ICs.

The STM32F411 is used as:
	- Clock generator with variable frequency and duty.
	- RESET pulse geneartor with variable width.
	- ROM emulator  (512 bytes)
	- VGA   640x480  (text mode)
	- SD controller (V1, V2: SDC+SDHC) 
	- Interrupt controller
	- PS2 keyboard controller
	- Systick generator

This computer works almost as the IBM PC 5150 does. At boot it runs a minimal BIOS like program stored in the F411. This program copies it self from the F411 flash to the TOP of RAM and jumps there. It then switchs the F411 from ROM emlation mode to VGA/SD mode and does some initializations (vector table, variables ...) before loading the 1st sector of the SD in RAM at 0x0000:0x7C00 and executes it.
The bootsector searchs for a file named "kernel.bin", in the FAT16 file system of the SD, if it's found it loads it into RAM at 0x0050:0x0000 and jumps there. Your OS starts !!

The BIOS is really very basic! It implements only a few functions: 
* Hardware interrupts:
	- INT8h  systick 
	- INT9h  Keyboard
* Software interrupts:
	- INT10h  screen
	- INT13h  SD
	- INT16h  Keyboard
See the BIOS source for more.

The F411 firmware:
-----------------
At startup the F411 does some initializations (CLK, SD, 8088 CLK,...) then it generates the reset pulse and enters the ROM emulation mode. when the BIOS finishes copying itself to RAM the F411 switchs to VGA/SD mode where it receives ascii characters to draw on screen or commands like CLS, delete char, goto X, goto Y, read sector nn, write sector mm...
The VGA generation code is inspired from ARTEKIT project:
https://www.artekit.eu/vga-output-using-a-36-pin-stm32/

Hardware:
--------
The connections are shown in connections.jpg. 
the 8088 A0-A7 and A16-A19 are demultiplexed using two 74LS373 ICs.
You have 2 FREE pins: PB2 and PC13.

ATTENTION:
PA0 is NOT 5V tolerant. Use a Schottky diode, anode connected to the pin, and enable the internal pull-up.

- Command/ascii format:
----------------------

	FUNCTION		A15 A14 A13 A12, A11-A8, A7-A0  (GPIOA)
	-------------------------------------------------------------------
	read sector	 	 1   0   A13-A0 = sector Nr
	write sector	 0   1   A13-A0 = sector Nr
	goto column		 0   0   1  0    xxxx    A7-A0  = column Nr (X)
	goto row		 0   0   0  1    xxxx    A7-A0  = row Nr (Y)
	ascii/command    0   0   0  0    xxxx    A7-A0  = ascii char or command  

Address decoding :
------------------
The 8088's A19 line is used to select the RAM when LOW (1st 512KB) or the F411 (ROM/VGA) when HIGH after inverting it.

 A19    DEVICE
  0      RAM
  1      F411  (ROM/VGA)

RAM addressed with A0-A18 (512kB).
ROM addressed with A0-A8 (512 bytes ).

How to:
------
choose 0xC0000 or any address in the range 0x80000 to 0xFFFFF.

ASCII/COMMAND:

To send the command/ASCII.
		GPIOA A0-A7 = command/ASCII
		GPIOA A15-A12 = 0000
--------------------------------------------------------
GOTO X/Y:
To send the command GOTO_X/GOTO_Y + the position.
		GPIOA A0-A7 = position
		GPIOA A13-A12 = 10  GOTO_X
		GPIOA A13-A12 = 01  GOTO_Y
		GPIOA A15-A14 = 00

--------------------------------------------------------
SD:
To send the command to READ/WRITE + sector Nr.
		GPIOA A0-A13 = sector Nr
		GPIOA A15-A14 = 10  Read sector
		GPIOA A15-A14 = 01  write sector
		
Then read/write 512 bytes of data
Max sectors = 0x3FFF ==> 8M bytes

Building:
--------
I'm using FASM under WIN11 to generate the binary files of bootsector and BIOS and the yagarto ARM GNU toolchain to build the F411 firmware.
If you use the Weact bootloader, which I recommand strongly, change this line in f411.ld :
	FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
To: 
	FLASH (rx)  : ORIGIN = 0x08004000, LENGTH = 512K

Use the tool bin2h.exe to convert the BIOS binary to a header file.
Use the tool hddrawcopy.exe (or dd.exe) to copy the bootsector binary to the 1st sector of the SD.

Build a kernel from the given examples and copy it's binary into the root of the SD card and Have fun !!

If you have any suggestion or you've made any changes to this project please let me know.

malaouar67@gmail.com

