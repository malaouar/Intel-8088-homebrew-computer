
#include "hal.h"
#include "vga.h" 

extern uint8_t buffer[];  // VGA var

volatile char key=0;		// key sent by the keyboard  (used in intr.c)
volatile char INTn=0;		// Software Interrupt  Nr (used in intr.c)
// these vars used in cs.c
volatile uint16_t sector=0;  // sector Nr to read/write
volatile unsigned short index = 512; // sector writing index
volatile unsigned short flag=0;	// Write flag
volatile unsigned char col=0;
volatile unsigned char row=0;
volatile unsigned char byte=0;


// Function prototype placed in RAM section
void EXTI3_IRQHandler() __attribute__((section(".ramfunc")));
void EXTI10_15_IRQHandler() __attribute__((section(".ramfunc")));
void TIM3_IRQHandler() __attribute__((section(".ramfunc")));
void TIM4_IRQHandler() __attribute__((section(".ramfunc")));
void DMA2_Stream3_IRQHandler() __attribute__((section(".ramfunc")));
void TEXT_handler() __attribute__((section(".ramfunc")));
void systick_handler() __attribute__((section(".ramfunc")));
void forever() __attribute__((section(".ramfunc")));

// Interrupts vector table in RAM
volatile uint32_t irq_vect[ 16+86 ] __attribute__((aligned(0x200))) ; // allign to 512 bytes

void spin(); // in crto.s

void main(void);

void resetHandler(void){
    // Copy .data section including .ramfunc section
    extern uint32_t _sdata, _edata, _sidata;
    for(uint32_t *src=&_sidata, *dst=&_sdata; dst<&_edata; *dst++=*src++);
        
    // Zero .bss section
    extern uint32_t _sbss, _ebss;
    for(uint32_t *dst=&_sbss; dst<&_ebss; *dst++=0);

	// reset vector table to a default handler
	for(int i =0; i<(16+86); i++) irq_vect[i] = (uint32_t)spin;  // spin() in crt0.s
	// Set the entries for  interupts.
	irq_vect[15] = (uint32_t)systick_handler;
	irq_vect[15+9+1] = (uint32_t)EXTI3_IRQHandler;
	irq_vect[15+29+1] = (uint32_t)TIM3_IRQHandler;
	irq_vect[15+30+1] = (uint32_t)TIM4_IRQHandler;
	irq_vect[15+40+1] = (uint32_t)EXTI10_15_IRQHandler;
	irq_vect[15+59+1] = (uint32_t)DMA2_Stream3_IRQHandler;
	
	// Set the VTOR to the new RAM location
	SCB->VTOR = (uint32_t)irq_vect;

	// call the main function
	main();
	
}

//=============================================================
// Fcpu = 100.5 MHz
static void clock_init(void){
    unsigned int ra;
		
	// turn HSE oscillator ON
	RCC->CR |=(1<<16) ;			//set bit16 HSE_ON
    //wait for HSE clock ready 
	while(!(RCC->CR & (1<<17))); // wait until bit17 HSE_RDY set
	
	// Configure PLL, Reserved bits must be kept at reset value.
	// 25/25 feeds 1 Mhz to PLL; PLL yiels 201 Mhz; 201/2 = 100.5 Mhz to cpu
	ra = RCC->PLLCFGR & 0xF0BC8000; 	// save reserved bits and clear all others.
	ra |= (4<<24)  // bits27:24 (PLLQ) division factor for USB OTG FS and SDIO clocks. here devisor = 4
		| (1<<22)	// Bit22 PLLSRC: 1 source = HSE, 0 source = HSI
		| (0<<16)	// Bits17:16 (PLLP) division factor for main system clock. 0 => devisor = 2	
		| (201<<6)  // Bits14:6 (PLLN) multiplication factor for VCO. here multiplicator = 192
		| (25);	// Bits5:0 (PLLM) Division factor for the input clock before the VCO. here devisor = 25 
	RCC->PLLCFGR = ra;

	// Turn on PLL in RCC clock control register (RCC_CR)
	RCC->CR |= 1<<24; // set Bit24 (PLLON) to enable PLL
	while ( ! (RCC->CR & (1<<25)));// wait until Bit25 PLLRDY is set
	
	// for Fcpu >90 <= 100MHz add 4 wait states (latency) in Flash access control register (FLASH_ACR)
	ra = FLASH->ACR;
	ra &= ~0xf; // clear Bits3:0 LATENCY
	ra |= 4;	// 3 wait states
	FLASH->ACR = ra;

	// switch from HSI to PLL in RCC clock configuration register (RCC_CFGR)
	//Bits1-0 : System clock switch (SW)
	ra = RCC->CFGR;
	ra &= ~(3<<0); // clear bits1:0
	ra |= (2<<0);  // bits1:0 = 10 => PLL selected as system clock
	// low-speed APB1 MAX freq is 50 MHz.
	ra |= (4<<10); // Bits12:10 (PPRE1) APB Low speed prescaler (APB1) = b"100" => AHB clock divided by 2
	RCC->CFGR = ra;
	
	// Enable data cache, instruction cache, and prefetch
	//Bit 8 PRFTEN Prefetch enable, Bit 9 ICEN Instruction cache enable, Bit 10 DCEN Data cache enable
	FLASH->ACR |= (1<<10) | (1<<9) | (1<<8);
}

//=======================================
// Init systick timer 
void systick_init(){
    SYSTICK->CSR = 0;  // Bit0 = 0 ==> systick disabled
    SYSTICK->RVR = 1249999; // Reload value 
    SYSTICK->CVR = 0; // Reset current value and clear COUNTFLAG
    SYSTICK->CSR = 3; // Bit0 = 1 ==> systick enabled
                      // bit1 = 1 ==> interrupt enabled
                      // bit2 = 0 ==> systick clock source =  system clock/8
}

//================================
void led_init(int bit){
	RCC->AHB1ENR |= 1<<2;		// enable GPIOC clock 
	GPIOC->MODER &= ~(3<<(bit*2)); 	// clear mode bits
	GPIOC->MODER |= (1<<(bit*2)); 	// mode = 01 => General purpose output
	//GPIOC->OTYPER &= ~(1<<bit);	// bit = 0 => Output push-pull (default reset state)
	GPIOC->BSRR = 1 << (13+16); 	// led ON
}

//=====================================================
void config_pins(){
	RCC->AHB1ENR |= 1 << 2 | 1 << 1 | 1 << 0; 		// Enable GPIOA, B and C clocks
	GPIOA->MODER = 0; 	// all pins inputs
	//PA0 is not 5V tolerant so use a pull-up + Schottky Diode
	GPIOA->PUPDR = 1;  	// PA0 pull-up enabled, all others floating
	GPIOA->OSPEEDR = 0xFFFFFFFF; 	// High speed
	
	GPIOB->MODER = 0; 	// ALL inputs 
						// 4, 5 and 6 are configured for SPI, PB7 for INTR, PB1 for CLK, PB10 for RESET
						// 13, 14 and 15 are configured for SD
	// Configure PB7 (INTR) to output push-pull 
	GPIOB->MODER |= (1<<(7*2)); 	// mode = 01 => General purpose output    
	//GPIOB->OTYPER &= ~(1 << 7);   // bit = 0 => Output push-pull (default reset state)
	GPIOB->BSRR = 1 << (7+16); 		// INTR = LOW    						
	// Configure PB1 and PB10 as alternate function 
	//TIM3 CH4 on PB1 ==> CLK
	//TIM2 CH3 on PB10 ==> RESET 	
	//GPIOB->MODER &= ~((3 << (1 * 2)) | (3 << (10 * 2))); 	// Clear bits    
	GPIOB->MODER |= (2 << (1 * 2)) | (2 << (10 * 2)); 		// Set AF mode        
	// Configure alternate function: PB1 = AF1 (TIM1_CH3N)    
	GPIOB->AFRL &= ~(0xF << (1 * 4)); // Clear AF bits for PB1    
	GPIOB->AFRL |= (1 << (1 * 4));    // Set AF2 for PB1        
	// Configure alternate function: PB10 = AF1 (TIM2_CH3)    
	GPIOB->AFRH &= ~(0xF << ((10 - 8) * 4)); // Clear AF bits for PB10    
	GPIOB->AFRH |= (1 << ((10 - 8) * 4));    // Set AF1 for PB10
	GPIOB->OSPEEDR =	0xFFFFFFFF;	// High speed

	// Configure PC14 and PC15 as keyboard CLK and DATA
	GPIOC->PUPDR |= 1<<15*2 | 1<<14*2; //  Enable pull-ups for PC15 and PC14
	GPIOC->OSPEEDR =	0xFFFFFFFF;	// High speed
}

//----------------------- EXTI10_15 STUFF --------------
// global function pointer to store the current EXTI10_15 interrupt handler
volatile InterruptHandler currentInterruptHandler = NULL;

//============================ MUST =========================
void Change_EXTI10_15_Handler(InterruptHandler new_handler) __attribute__((section(".ramfunc")));
void Change_EXTI10_15_Handler(InterruptHandler new_handler) {
	// Disable global interrupts
	asm("cpsid i");
	// Update the interrupt handler    
	currentInterruptHandler = new_handler;
	asm("cpsie i"); 	// Re-enable global interrupts
}
//----------------------- End EXTI10_15 STUFF --------------

//================================
// Runs from RAM
void forever(){
	uint8_t err;

	for(;;){
		if(!flag){
			if(sector){
				read_sector(sector-1, buffer);
				sector = 0;
			}
		}
		if(!index){
			err = write_sector(sector-1, buffer);
			if(err){ 
				print_str("SD write ERROR: ");
			}
			index = 512;
			sector = 0;
			flag = 0;
		}

		if(byte){
			if(byte>0x1F && byte<0x80) draw_char(byte); // an ASCII code
			else if(byte == '\r' || byte == '\n') newl(byte); // line feed or carriage return
			else if(byte == 8) del_char(); // Back space: delete the char before cursor position
			else if(byte == 5) cls();
			byte = 0;
		}
		if(col){ 
			goto_col(col);
			col = 0;	
		}
		if(row){ 
			goto_row(row);
			row = 0;	
		}	
		
		// Poll for keyboard data        
		if(!(GPIOC->IDR & 1<<15)){ 
			key = PS2_GetChar();        
			// Send an interrupt request to the 8088
			INTn = 9;	// keyboard
			GPIOB->BSRR = 1 << 7; 		// INTR = HIGH
		}
	}	
}
	
//================================
void main(){
	uint16_t i, sd; 	// sd for SD initialization result
	
	char *fbp = fb;
		
	clock_init();
	config_pins(); 		// GPIOA/B	
	led_init(13);  		// PC13 init
	
	systick_init();		// 100ms TIC

	// Initialize SPI2 for SD card communication   
	spi2_init();
	sd = sd_init();

	// Clear screen
	cls();  
	// default EXTI10_15 Handler
	Change_EXTI10_15_Handler(TEXT_handler); // TEXT mode handler

	// Configure TIM2 CH3 on PB10 (RESET) as One Pulse Mode    
	tim2_one_pulse_config(50);    // 50us pulse    
	// MUST Trigger the pulse before starting the clock !!        
	TIM2->CR1 |= 1<<0;		//TIM_CR1_CEN                

	// Configure TIM1 CH3N on PB1(CLK) as PWM    
	tim1_pwm_config(2000000, 33);	// 2 MHz, 33% duty cycle

	//Start ROM emulation
	rom_emu();

	// Now we are in VGA/SD mode
	vid_init();  // Init VGA
				
	if(sd){ 
		print_str("SD Init ERROR!!");
		while(1) asm ("wfi");		// hang
	}	
	
	// enable CS (PB12) interrupt to handle the 8088 requests
	EXTI10_15_enable();
	
	// enable INTA (PB3) interrupt to handle the 8088 INTR ack (INTA)
	EXTI3_enable();

	forever();
}

//======================
void systick_handler(){
	// Send an interrupt request to the 8088
	INTn = 8;	// systick
	GPIOB->BSRR = 1 << 7; 		// INTR = HIGH	
}

