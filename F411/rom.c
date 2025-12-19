#include "hal.h"
#include "bios.h"


//=====================================================
// Function prototype placed in RAM section
void rom_emu() __attribute__((section(".ramfunc")));

void rom_emu(){
	register unsigned short data1;  // data sent by 8088
	register unsigned short data2;  //
	register unsigned short address;
	
	for(;;){	
		// wait until PB12 ROM_CS becomes LOW
		data2 =	GPIOB->IDR;
		if(!(data2 & (1 << 12))){ // ROM selected
			while((GPIOB->IDR & (1 << 9 | 1 << 8)) == (1 << 9 | 1 << 8)); // wait until PB8 (RD) or PB9 (WR) is LOW
			if(!(data2 & (1 << 8))){ //RD
				// get address
				data1 = GPIOA->IDR; // A0-A7
				data2 = GPIOB->IDR; // A8 
				address = (data2&0x0001)<<8 | data1&0x00FF;
				GPIOA->ODR  = (bios_bin[address])<<8;
				// Switch PORTA_H to output
				GPIOA->MODER = 0x55550000; 	// PA8-15 outputs
				while(!(GPIOB->IDR & (1 << 8))); // wait until PB8 (RD =1)
				GPIOA->MODER = 0; 	// PA8-15 inputs
			}
			// quit ROM emulation
			// Do a dummy write to ROM to exit ROM emulation mode
			else if(!(data2 & (1 << 9))) break; // if WR exit
		}
	}
}


