
#include "hal.h"
#include "vga.h" 

/*
	1 pass to send ascii or command
	Format:
					A15 A14 A13 A12 A11-A8   A7-A0
	read sector	 	 1   0   A13-A0 = sector Nr
	write sector	 0   1   A13-A0 = sector Nr
	goto column		 0   0   1  x   xxxx     A7-A0  = column Nr
	goto row		 0   0   x  1   xxxx     A7-A0  = row Nr
	ascci/command    0   0   0  0   xxxx     A7-A0  = ascii/command 

*/

extern volatile uint8_t buffer[];  // in vga.h
// these vars in main.c
extern volatile unsigned short index;	// Index for sector writing
extern volatile unsigned short flag;	// Write flag
extern volatile uint16_t sector;		// sector Nr to read/write
extern volatile unsigned char col;
extern volatile unsigned char row;
extern volatile unsigned char byte;

//=====================================
// config CS (PB12) as EXTI10_15 
void EXTI10_15_enable(){
	// First Enable GPIOB and SYSCFG  clocks	
	RCC->APB2ENR |= 1<< 14;  // Enable SYSCFG clock
	RCC->AHB1ENR |= 1<< 1; 	// Enable GPIOB clock 
		
	// EXTI12 is configured using  SYSCFG->EXTICR4 register.
	SYSCFG->EXTICR4 |= 0x0001; // Tie PB12 to EXTI12
	
	// Next we choose either rising edge trigger (RTSR) or falling edge trigger (FTSR)
	EXTI->FTSR |= 1 << 12;   // TR12 = 1 => Enable falling edge trigger on EXTI12

	// Next  we unmask the used external interrupt number.
	EXTI->IMR |= 1 << 12;    // MR12 = 1 => unMask EXTI12
	 
	// Finally we enable the interrupt in NVIC (EXTI10_15 Irq Nr is 40)
	NVIC->ISER[40 >> 5]  = 1 << (40&0x1F) ; // enable IRQn 40 --> External Line10_15 Interrupt
}

//===============================
// Function prototype placed in RAM section
void EXTI10_15_IRQHandler() __attribute__((section(".ramfunc")));
void EXTI10_15_IRQHandler(){
	currentInterruptHandler();
	EXTI->PR = (1<<12);  // PR12 =1 => Remove pending state of interrupt exti12
}

//=================== TEXT mode handler ======================
// Function prototype placed in RAM section
void TEXT_handler() __attribute__((section(".ramfunc")));
void TEXT_handler(){
	register unsigned short data;
	register unsigned short data2;
	
	
	// RD/WR mask = 1 << 9 | 1 << 8; 
	while((data2=(GPIOB->IDR)&(1 << 9 | 1 << 8)) == (1 << 9 | 1 << 8));
	// get data
	data = GPIOA->IDR; // read GPIOA
	
	if(!(data2 & (1 << 9))){// PB9 (WR) = 0
		if(!flag){// flag = 0
			// process data
			if(data & 1<<15){ // Fonction read sector (copy from SD to RAM)
				sector = data&0x3FFF;
				// the buffer is read from SD in forever() in main.c
			}
			else if(data & 1<<14){ // Fonction write sector)
				sector = data&0x3FFF;
				flag = 1;
			}			
			else if(!(data&0x0F<<12)){ // a command or ascii char
				byte = data&0x00FF;				
			}
			else if(data&1<<13) col = data&0x00FF; 
			else if(data&1<<12) row = data&0x00FF;
		}
		else{// flag = 1
			//data = GPIOA->IDR;  // read GPIOA for data (already done)
			buffer[--index] = data>>8; // save byte in buffer
			// the buffer is written to SD in forever() in main.c
		}
	}
	else { // PB8 (RD) = 0  (read sector)
		// get index
		//data = (GPIOA->IDR); // A0-A7  (already done)
		data2 = GPIOB->IDR; // get A8 
		GPIOA->ODR  = (buffer[(data2&0x0001)<<8 | data&0x00FF])<<8;
		// Switch PORTA_H to output
		GPIOA->MODER = 0x55550000; 	// PA8-15 outputs
		while(!(GPIOB->IDR & (1 << 8))); // wait until PB8 (RD =1)
		GPIOA->MODER = 0; 	// PA8-15 inputs
	}
}

