/** 
* This program configures the SPI2 peripheral of STM32F411 for SD card communication in SPI mode.
* SPI2 Pin Configuration:
* - PB13: SPI2_SCK  (Serial Clock)
* - PB14: SPI2_MISO (Master In Slave Out) 
* - PB15: SPI2_MOSI (Master Out Slave In) 
* - PB12: SD Card CS (Chip Select) - GPIO output 
*/

#include "hal.h"

// GPIO pins
#define SCK        13
#define MISO       14
#define MOSI       15
#define CS         12

// RCC register bits
#define GPIOBEN (1 << 1)  // GPIOB enable
#define SPI2EN  (1 << 14) // SPI2 enable

// SPI register bits
#define CPHA        (1 << 0)  // Clock Phase
#define CPOL        (1 << 1)  // Clock Polarity
#define MSTR        (1 << 2)  // Master Selection
#define BR_MASK     (7 << 3)  // Baud Rate Control mask
#define BR_256      (7 << 3)  // Baud Rate = fPCLK/256 (slow for init)
#define BR_128      (6 << 3)  // Baud Rate = fPCLK/128
#define BR_64       (5 << 3)  // Baud Rate = fPCLK/64
#define BR_32       (4 << 3)  // Baud Rate = fPCLK/32
#define BR_16       (3 << 3)  // Baud Rate = fPCLK/16
#define SPE         (1 << 6)  // SPI Enable
#define SSI         (1 << 8)  // Internal slave select
#define SSM         (1 << 9)  // Software slave management
#define DFF         (1 << 11) // Data Frame Format (0: 8-bit, 1: 16-bit)

// SPI Status Register bits
#define SR_RXNE         (1 << 0)  // Receive buffer Not Empty
#define SR_TXE          (1 << 1)  // Transmit buffer Empty
#define SR_BSY          (1 << 7)  // Busy flag

// SD Card commands
#define CMD0             0     // GO_IDLE_STATE
#define CMD8             8     // SEND_IF_COND
#define CMD17            17    // Read 1 sector
#define CMD24            24    // Write 1 sector
#define CMD55            55    // APP_CMD
#define CMD58            58    // READ_OCR
#define ACMD41           41    // SEND_OP_COND

// Miscellaneous
#define DUMMY_BYTE       0xFF

// Function prototypes
void spi2_init(void);
void spi2_set_speed(uint32_t speed_setting);
void cs_low(void);
void cs_high(void);
uint8_t spi2_transfer(uint8_t data);
uint8_t send_command(uint8_t cmd, uint32_t arg);
uint8_t sd_init(void);
uint8_t write_sector(uint32_t sector, uint8_t* buffer);
uint8_t read_sector(uint32_t sector, uint8_t* buffer);

uint8_t sd_type = 0; 
uint8_t buffer[512];

/** 
* Initialize SPI2 for SD card communication 
*/
void spi2_init(void) {    
	// 1. Enable clocks for GPIOB and SPI2    
	RCC->AHB1ENR |= GPIOBEN;    
	RCC->APB1ENR |= SPI2EN;    
	// 2. Configure SPI2 pins (PB13, PB14, PB15)    
	// Set pin mode to alternate function (AF) mode (0b10)    
	GPIOB->MODER &= ~(3 << (SCK * 2) | 3 << (MISO * 2) | 3 << (MOSI * 2));  
	GPIOB->MODER |= 2 << (SCK * 2) | 2 << (MISO * 2) | 2 << (MOSI * 2);   
	// Set output type to push-pull (0) for all pins    
	GPIOB->OTYPER &= ~((1 << SCK) | (1 << MISO) | (1 << MOSI));        
	// Set output speed to high (0b10) for all pins    
	GPIOB->OSPEEDR &= ~(3 << (SCK * 2) | 3 << (MISO * 2) | 3 << (MOSI * 2));
	GPIOB->OSPEEDR |= 2 << (SCK * 2) | 2 << (MISO * 2) | 2 << (MOSI * 2);  
	
	// Configure alternate function registers for SPI2 (AF5 = 0b0101)    
	GPIOB->AFRH &= ~((0xF << ((SCK - 8) * 4)) | (0xF << ((MISO - 8) * 4)) | (0xF << ((MOSI - 8) * 4)));    
	GPIOB->AFRH |= 5 << ((SCK - 8) * 4) | 5 << ((MISO - 8) * 4) | 5 << ((MOSI - 8) * 4);
		
	// 4. Configure SPI2    
	// Disable SPI before configuration    
	SPI2->CR1 &= ~SPE;    
	// Configure SPI settings   
	SPI2->CR1 = 0; // Reset register    
	SPI2->CR1 |= MSTR;             // Master mode    
	SPI2->CR1 |= SSM | SSI; // Software slave management, internal slave select    
	SPI2->CR1 &= ~DFF;             // 8-bit data frame format   
	SPI2->CR1 &= ~CPOL;            // Clock polarity = 0, clock low when idle
	SPI2->CR1 &= ~CPHA;            // Clock phase = 0, first edge    
	// Set initial baud rate to lowest frequency (PCLK/256) for SD card initialization
    spi2_set_speed(BR_256);    
	// Enable SPI2    
	SPI2->CR1 |= SPE;
}

/** 
* Set SPI2 speed 
* @param speed_setting: Speed setting bits (BR_xx) 
*/
// Function prototype placed in RAM section
void spi2_set_speed(uint32_t speed_setting) __attribute__((section(".ramfunc")));
void spi2_set_speed(uint32_t speed_setting) {    
	// Disable SPI   
	SPI2->CR1 &= ~SPE;    
	// Update speed    
	SPI2->CR1 &= ~BR_MASK;  // Clear BR bits    
	SPI2->CR1 |= speed_setting;     // Set new BR bits    
	// Re-enable SPI    
	SPI2->CR1 |= SPE;
}


// Transfer one byte via SPI2 
//param  Byte to send 
//return: Received byte 
// Function prototype placed in RAM section
uint8_t spi2_transfer(uint8_t data) __attribute__((section(".ramfunc")));
uint8_t spi2_transfer(uint8_t data) {    
	// Wait until transmit buffer is empty   
	while (!(SPI2->SR & SR_TXE));    
	// Send data    
	SPI2->DR = data;    
	// Wait until receive buffer is not empty    
	while (!(SPI2->SR & SR_RXNE));   
	// Return received data    
	return (uint8_t)SPI2->DR;
}


/** 
* Send a command to the SD card 
* @param cmd: Command index 
* @param arg: Command argument 
* @return: Response byte 
*/
// Function prototype placed in RAM section
uint8_t send_command(uint8_t cmd, uint32_t arg) __attribute__((section(".ramfunc")));
uint8_t send_command(uint8_t cmd, uint32_t arg) {    
	uint8_t response, retry = 0;    
	
	// Send dummy byte for sync    
	spi2_transfer(DUMMY_BYTE);  
	// Send command    
	spi2_transfer(0x40 | cmd);        // Command (bit 6 always set)    
	spi2_transfer((arg >> 24) & 0xFF); // Argument [31:24]    
	spi2_transfer((arg >> 16) & 0xFF); // Argument [23:16]    
	spi2_transfer((arg >> 8) & 0xFF);  // Argument [15:8]   
	spi2_transfer(arg & 0xFF);         // Argument [7:0]    
	// Send CRC   
	if (cmd == CMD0) {        
		spi2_transfer(0x95);  // Valid CRC for CMD0    
	} else if (cmd == CMD8) {       
		spi2_transfer(0x87);  // Valid CRC for CMD8    
	} else {        
		spi2_transfer(0x01);  // Dummy CRC + Stop bit    
	}  
	// Wait for response (skip stuff byte for stop read)    
	if (cmd == 12) {        
		spi2_transfer(DUMMY_BYTE); // Skip one byte    
	}   
	// Receive response    
	retry = 10; // Wait for up to 10 bytes    
	do {       
		response = spi2_transfer(DUMMY_BYTE);        
		retry--;    
	} while ((response & 0x80) && retry);   
	return response;
}

/** 
* Initialize SD card in SPI mode 
* @return: 0 if successful, non-zero if error 
*/
uint8_t sd_init(void) {   
	uint8_t response;    
	uint16_t retry;   
	uint32_t ocr_value;    
	uint8_t buffer[4];  
	  
	// Power-up wait (at least 80 clock cycles with CS high)    	
	for (retry = 0; retry < 10; retry++) {      
		spi2_transfer(DUMMY_BYTE);    
	}    
	// CMD0: Go to idle state    
	retry = 100;    
	do {        
		response = send_command(CMD0, 0);        
		retry--;    
	} while ((response != 0x01) && retry);   
	if (response != 0x01) {        
        
		return 1; // Error: Card did not enter idle state    
	}    
	// CMD8: Send Interface Condition (check if SD v2)    
	response = send_command(CMD8, 0x000001AA);   
	if (response == 0x01) {       
	// SD V2 Card       
	// Get the rest of R7 response        
	for (retry = 0; retry < 4; retry++) {           
		buffer[retry] = spi2_transfer(DUMMY_BYTE);        
	}       
	// Check voltage range (should be 0x01) and echo back pattern (0xAA)        
	if (buffer[2] == 0x01 && buffer[3] == 0xAA) {            
		// Valid SD V2 card           
		sd_type = 2;        
	} else {            
		
		return 2; // Error: Incompatible voltage        
	}   
	} else {        
		// SD V1 Card or MMC       
		sd_type = 1;    
	}    
	// Send ACMD41 until card is ready    
	retry = 1000;    
	do {        
		// Send CMD55 first (to indicate next command is application command)       
		response = send_command(CMD55, 0);        
		if (response <= 0x01) {          
			// SD card recognized the CMD55            
			// Send ACMD41 with HCS bit set for SD V2 cards            
			response = send_command(ACMD41, sd_type == 2 ? 0x40000000 : 0);        
		} else {           
			// Failed to send CMD55, might be MMC            
			//cs_high();
            
			return 3; // Error: Not an SD card        
		}       
		if (retry-- == 0) {            
			
			return 4; // Error: Card did not initialize        
		}    
	} while (response != 0x00);   
	// If SD V2, check for high capacity card    
	if (sd_type == 2) {       
		// Send CMD58 to read OCR        
		response = send_command(CMD58, 0);        
		if (response == 0x00) {            
			// Read OCR register            
				for (retry = 0; retry < 4; retry++) {                
				buffer[retry] = spi2_transfer(DUMMY_BYTE);            
			}          
			// Check CCS bit (bit 30)            
				if (buffer[0] & 0x40) {                
				// High capacity card (SDHC)               
				sd_type = 3;            
			}        
		}  
	} 
	
	// Send dummy byte    
	spi2_transfer(DUMMY_BYTE);    
	// Now we can increase SPI speed
	// 16, 32, 64, 128 don't work in writing BUT works in reading !!    
	spi2_set_speed(BR_128);    
	return 0; // Success
}

/* 
* Read a single 512-byte sector from the SD card
* @param sector: Sector number to read 
* @param buffer: Pointer to buffer to store the read data (must be at least 512 bytes) 
* @return: 0 if successful, non-zero if error
*/
// Function prototype placed in RAM section
uint8_t read_sector(uint32_t sector, uint8_t* buffer) __attribute__((section(".ramfunc")));
uint8_t read_sector(uint32_t sector, uint8_t* buffer) {    
	uint8_t response;    
	uint16_t i;   
	uint32_t timeout;    
	// For non-SDHC cards, convert sector number to byte address (sector * 512)    
	if (sd_type != 3) {        
		sector = sector << 9;  // Multiply by 512    
	}   
	// Send CMD17 (READ_SINGLE_BLOCK)    
	response = send_command(CMD17, sector);    
	if (response != 0x00) {        
		//cs_high();
		
		return 1;  // Error: Command rejected    
	}    
	// Wait for data token (0xFE)   
	timeout = 200000;  // Timeout value    
	do {        
		response = spi2_transfer(DUMMY_BYTE);        
		timeout--;    
	} while (response == 0xFF && timeout);   
	if (response != 0xFE) {        
		//cs_high(); 
		
		return 2;  // Error: Data token not received    
	}    
	// Read 512 bytes of data    
	for (i = 0; i < 512; i++) {        
		buffer[i] = spi2_transfer(DUMMY_BYTE);   
	}    
	// Read and discard CRC (2 bytes)   
	spi2_transfer(DUMMY_BYTE);    
	spi2_transfer(DUMMY_BYTE);   
	// Deselect card    
	//cs_high(); 
	
	// Send dummy byte    
	spi2_transfer(DUMMY_BYTE);    
	return 0;  // Success
}

/* 
* Write a single 512-byte sector to the SD card 
* @param sector: Sector number to write
* @param buffer: Pointer to data to write (must be 512 bytes) 
* @return: 0 if successful, non-zero if error 
*/
// Function prototype placed in RAM section
uint8_t write_sector(uint32_t sector, uint8_t* buffer) __attribute__((section(".ramfunc")));
uint8_t write_sector(uint32_t sector, uint8_t* buffer) {    
	uint8_t response;    
	uint16_t i;   
	uint32_t timeout; 
	// speed down
	spi2_set_speed(BR_256);
	
	// For non-SDHC cards, convert sector number to byte address (sector * 512)\n    
	if (sd_type != 3) {        
		sector = sector << 9;  
		// Multiply by 512    
	}   
	// Send CMD24 (WRITE_BLOCK)    
	response = send_command(CMD24, sector);    
	if (response != 0x00) {       
		spi2_set_speed(BR_128);	// speed up
		return 1;  	// Error: Command rejected    
	}
    // Wait for card readiness (0xFF)
    while(spi2_transfer(DUMMY_BYTE) != 0xFF);
	
	// Send data token (0xFE)    
	spi2_transfer(0xFE);    
	// Write 512 bytes of data    
	for (i = 0; i < 512; i++) spi2_transfer(buffer[i]);
	// Send dummy CRC (2 bytes)   
	spi2_transfer(DUMMY_BYTE);   
	spi2_transfer(DUMMY_BYTE); 
	
	// Check data response token 
	timeout = 1000000;  // Timeout value
	do {		
		response = spi2_transfer(DUMMY_BYTE);	
		timeout--;
	} while ((response& 0x1F) != 0x05 && timeout);    
	if (timeout == 0) {        
		spi2_set_speed(BR_128);	// speed up
		return 2;  // Error: Write timeout    
	}

	// Wait for write to complete   
	timeout = 1000000;  // Timeout value    
	do {        
		response = spi2_transfer(DUMMY_BYTE);        
		timeout--;    
	} while (response == 0x00 && timeout);    
	if (timeout == 0) {        
		spi2_set_speed(BR_128);	// speed up
		return 3;  // Error: Write timeout    
	}
	
	// Send dummy byte    
	spi2_transfer(DUMMY_BYTE);
	// speed up
	spi2_set_speed(BR_128);	
	return 0;  // Success
}
