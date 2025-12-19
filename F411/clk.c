#include "hal.h"

//==================================
void tim1_pwm_config(uint32_t frequency, uint8_t duty_cycle){    
	uint32_t tim_clk = 100000000;  // Timer clock = 100MHz (APB1 * 2)
	uint32_t prescaler = 0;    
	uint32_t period = 0;        
	// Calculate prescaler and period values    
	if (frequency > 0){        
		// Find suitable prescaler and period values        
		for (prescaler = 0; prescaler < 65536; prescaler++){            
			period = tim_clk / ((prescaler + 1) * frequency);            
			if (period < 65536) 	break;        
		}                
		// Limit duty cycle to 0-100%        
		if (duty_cycle > 100)	duty_cycle = 100;                
		// Enable GPIOB and TIM1 clocks
		RCC->AHB1ENR |= (1 << 1);  // Enable GPIOB clock
		RCC->APB2ENR |= (1 << 0);   // Enable TIM1 clock 
		
		// Configure TIM1 period and duty
		TIM1->PSC = prescaler; 
		TIM1->ARR = period - 1;         // Auto-reload value for 1000 counts		
		// Calculate and set CCR3 for desired duty cycle        
		TIM1->CCR[2] = (period * duty_cycle) / 100;
		
		// Configure TIM1 CH3N for PWM mode 1
		TIM1->CCMR[1] |= (6 << 4) | (1 << 3); // PWM mode 1, preload enable
		TIM1->CCER = (1 << 10);        // Enable output for CH3N
		TIM1->BDTR = (1 << 15);        // Main output enable (MOE)

		// Generate an update event to load the prescaler and ARR
		TIM1->EGR |= (1 << 0); // UG bit

		// Enable TIM1 counter        
		TIM1->CR1 = (1 << 7) | (1 << 0); // ARPE enable, counter enable
	}
}

/** 
* Configure TIM2 CH3 in one-pulse mode with adjustable pulse length 
* @param pulse_length_us: Pulse length in microseconds 
*/
void tim2_one_pulse_config(uint32_t pulse_length_us){    
	//uint32_t tim_clk = 100000000;  // Timer clock = 100MHz (APB1 * 2)   
	uint32_t prescaler = 100 - 1;  // Prescale to get 1MHz (1µs resolution)
	uint32_t period = pulse_length_us; // Period in µs
		
	// Enable TIM2 clock    
	RCC->APB1ENR |= 1<<0;		//RCC_APB1ENR_TIM2EN        
	// Configure TIM2 for one-pulse mode    
	TIM2->CR1 = 1<<3;		//TIM_CR1_OPM  // One-pulse mode, counter stops after one cycle    
	TIM2->PSC = prescaler;    
	TIM2->ARR = period - 1; // Set ARR for desired pulse length 
		
	TIM2->CNT = 0;		// clear the counter
	// Set CCR3 for desired delay before the pulse  
	TIM2->CCR[2] = 1;	// CCR MUST be > CNT
	TIM2->CCER |= 1<<9; // Bit9 (CC3P) = 1 =>  OC3 active low
		
	// Configure CH3 for PWM mode 1    
	TIM2->CCMR[1] &= ~(7<<4);   // Clear output compare mode bits    
	TIM2->CCMR[1] |= (6<<4);    // PWM mode 1
	
	// Enable CH3 output    
	TIM2->CCER |= 1<<8; //TIM_CCER_CC3E;        
	// Generate update event to load registers    
	TIM2->EGR |= 0x01;  // UG=1 (Update Generation)        
	// Note: TIM2 counter will be enabled in main() when a pulse is needed
}