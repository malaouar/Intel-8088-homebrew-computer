
#include <stdbool.h>

#include "hal.h"

// PS2 CLK ==> PC15
// PS2 DATA ==> PC14

// PS2 scan code to ASCII mapping (partial)
const uint8_t ps2_to_ascii[128] = {    
0x00, 0x89, 0x87, 0x85, 0x83, 0x81, 0x82, 0x8C, 0x00, 0x8A, 0x88, 0x86, 0x84, 0x09, 0x60, 0x8F,    
0x03, 0xA0, 0x00, 0x00, 0x00, 'q', '1', 0x00, 0x00, 0x00, 'z', 's', 'a', 'w', '2', 0xA1,    
0x00, 'c', 'x', 'd', 'e', '4', '3', 0xA2, 0x00, ' ', 'v', 'f', 't', 'r', '5', 0xA3,    
0x00, 'n', 'b', 'h', 'g', 'y', '6', 0xA4, 0x00, 0x00, 'm', 'j', 'u', '7', '8', 0xA5,   
0x00, ',', 'k', 'i', 'o', '0', '9', 0x00, 0x00, '.', '/', 'l', ';', 'p', '-', 0x00,    
0x00, 0x00, 0x27, 0x00, '[', '=', 0x00, 0x00, 0x00, 0x00, 0x0D, ']', 0x00, 0x5C, 0xA6, 0x00,    
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, '1', 0x2F, '4', '7', 0x00, 0x00, 0x00,    
'0', '.', '2', '5', '6', '8', 0x1B, 0x00, 0x8B, '+', '3', '-', '*', '9', 0x8D, 0x00
};

// Global variables
volatile bool is_extended = false;
volatile bool is_released = false;

/**
* Read a single bit from the PS2 keyboard 
* Returns: bool - The bit value (true for 1, false for 0) 
*/
bool PS2_ReadBit() __attribute__((section(".ramfunc")));
bool PS2_ReadBit(void) {    
	// Wait for falling edge of clock   
	while(GPIOC->IDR & 1<<15);   //PC15 = CLK   
	// Read data bit    
	bool bit = (GPIOC->IDR & 1<<14) != 0;  //PC14 = DATA 
	// Wait for rising edge of clock    
	while(!(GPIOC->IDR & 1<<15));    
	return bit;
}

/** 
* Read a complete byte from the PS2 keyboard 
* Returns: uint8_t - The received byte
*/
uint8_t PS2_ReadByte() __attribute__((section(".ramfunc")));
uint8_t PS2_ReadByte(void){    
	uint8_t data = 0;    
	bool parity_bit;
	
	// Wait for falling edge of clock   
	while(GPIOC->IDR & (1<<15));    // CLK High
	// Start bit (should be 0)    
	PS2_ReadBit();    
	// 8 data bits, LSB first    
	for (int i = 0; i < 8; i++) {        
		if (PS2_ReadBit()) data |= (1 << i);
	}   
	// Parity bit (odd parity)    
	parity_bit = PS2_ReadBit();   
	// Stop bit (should be 1)    
	PS2_ReadBit();    
	return data;
}

/** 
* Get a character from the PS2 keyboard 
* Returns: char - ASCII character or 0 if no valid character 
*/
char PS2_GetChar() __attribute__((section(".ramfunc")));
char PS2_GetChar(void){    
	char result = 0;    
	// Read the scan code        
	uint8_t code = PS2_ReadByte();        
	// Process special codes       
	if(code == 0xE0){            
		// Extended key code follows            
		is_extended = true;           
		return 0;        
	}
	else if (code == 0xF0){            
		// Key release code follows            
		is_released = true;            
		return 0;        
	}       
	// If this is a regular key press (not release)        
	if (!is_released){            
		// Convert scan code to ASCII for regular (non-extended) keys
		if (!is_extended && code < 128){                
			result = ps2_to_ascii[code];            
		}            
		// For extended keys, we would need additional mapping        
	}
	// Reset flags
	is_extended = false;
	is_released = false;
	return result;
}
