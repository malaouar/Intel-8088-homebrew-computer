#include "hal.h"
#include "vga.h" 

// Interrupt controller
// PB7 ==> INTR
// PB3 ==> INTA

extern volatile char INTn;		// Software Interrupt  Nr
extern volatile char key;		// key sent by the keyboard

// TEXT mode handler in cs.c
void TEXT_handler() __attribute__((section(".ramfunc")));

//=====================================
// config INTA (PB3) as EXTI3 
void EXTI3_enable(){
	// First Enable GPIOB and SYSCFG  clocks	
	RCC->APB2ENR |= 1<< 14;  // Enable SYSCFG clock
	RCC->AHB1ENR |= 1<< 1; 	// Enable GPIOB clock 
	
	// EXTI3 is configured using  SYSCFG->EXTICR1 register.
	SYSCFG->EXTICR1 |= 0x1000; // Tie PB3 to EXTI3
	
	// Next we choose either rising edge trigger (RTSR) or falling edge trigger (FTSR)
	EXTI->FTSR |= 1 << 3;   // TR3 = 1 => Enable falling edge trigger on EXTI3

	// Next  we unmask the used external interrupt number.
	EXTI->IMR |= 1 << 3;    // MR3 = 1 => unMask EXTI3
	
	// Finally we enable the interrupt in NVIC (EXTI3 Irq Nr is 9)
	NVIC->ISER[9 >> 5]  = 1 << (9&0x1F) ; // enable IRQn 9 --> External Line3 Interrupt
}

//===============================
// Keyboard  handler
void INT9_handler() __attribute__((section(".ramfunc")));
void INT9_handler(){
		
	while(GPIOB->IDR & (1 << 8));  // wait until PB8 (RD) = 0 
	GPIOA->ODR  = key<<8;
	// Switch PORTA_H to output
	GPIOA->MODER = 0x55550000; 	// PA8-15 outputs
	while(!(GPIOB->IDR & (1 << 8))); // wait until PB8 (RD =1)
	GPIOA->MODER = 0; 	// PA8-15 inputs
	key = 0;

	// change EXTI10_15 interrupt to default handler
	Change_EXTI10_15_Handler(TEXT_handler); // 
}

//===============================
// External interrupt3 (EXTI3) handler
void EXTI3_IRQHandler() __attribute__((section(".ramfunc")));
void EXTI3_IRQHandler(){
	// Pull INTR (PB7) LOW
	GPIOB->BSRR = 1 << (7+16); 		// INTR = LOW    
	while(!(GPIOB->IDR & (1<<3)));	// Wait for rising edge of INTA
	while(GPIOB->IDR & (1<<3)); 	// Wait for falling edge of INTA
	// put interrupt Nr on PA15-PA8
	GPIOA->ODR  = INTn << 8;		// 	
	// Switch PORTA_H to output
	GPIOA->MODER = 0x55550000; 	// PA8-15 outputs
	while(!(GPIOB->IDR & (1 << 3))); // wait until INTA  =1
	GPIOA->MODER = 0; 	// PA8-15 inputs
	
	// change EXTI10_15 interrupt handler
	if(INTn == 9) Change_EXTI10_15_Handler(INT9_handler); //	
	EXTI->PR = 1<<3;  // PR3 =1 => Remove pending state of interrupt exti3
}

