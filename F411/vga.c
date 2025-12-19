
#include "hal.h"
#include "vga.h" 

const u8 	font[];
uint8_t fb[VID_VSIZE][VID_HSIZE] __attribute__((aligned(32))) ;

static volatile unsigned int vline = 0;		// The current line being drawn
static volatile unsigned int vflag = 0;		// When 1, the SPI DMA request can draw on the screen

u16 con_x, con_y;  // console coordinates 

//***********************************************************************
// HSYNC (TIM3_CH1) and VSYNC_BACKPORCH (TIM3_CH2)
void TIM3_Config(){
		
	// 640x480 25.175 MHz
	uint16_t TimerPeriod = 3194; 		// Timer3 period 100.5*10^6/X = 31468.75 => 3193.6444
	uint16_t Channel1Pulse = 383;		// HSYNC pulse width Y*(1/100.5)= 3.81330us => 383.23665
	uint16_t Channel2Pulse = 574; 		// HSYNC + BACK PORCH  Z*(1/100.5)= 5.71995us => 574.854975
										// BACK PORCH 5.71995 - 3.81330 = 1.90665us

	// Set the Autoreload value
	TIM3->ARR = TimerPeriod ; // 
 
	// Generate an update event to reload the Prescaler and 
	// the Repetition counter values immediately
	TIM3->EGR = (uint16_t)0x0001; // EGR_UG (Bit0) = 1 PSC Reload Mode Immediate            

	//------------------------  Channel1 ----------------
	TIM3->CCMR[0] = 0x0070; // bits6:4 = 111 => PWM mode 2
	//TIM3->CCER = 0x0003; // Bit1 = 0 => OC1 active high, Bit0 = 1 => OC1 On (800x600)
	TIM3->CCER = 0x0001; // Bit1 = 1 => OC1 active low, Bit0 = 1 => OC1 On (640x480)
	// Set the Capture Compare Register value
	TIM3->CCR[0] = Channel1Pulse; // VSYNC pulse width
	//------------------------  Channel2 ----------------	
	// Bits14:12 = 010 => Set channel 2 to inactive level on match (OCMode_Inactive)
	TIM3->CCMR[0] |= 0x2000;  
	// Set the Capture Compare Register CCR2 value
	TIM3->CCR[1] = Channel2Pulse; // VSync pulse + Back porch
  
	//----------------  Configure TIMER1 as Master ----------------- 
	// Set the MSM Bit: synchronization timer and its slaves (through TRGO).
	TIM3->SMCR = (uint16_t)0x0080;  // MSM=1 => MasterSlaveMode Enable 
	// Select the TRGO source
	TIM3->CR2 |=  (uint16_t)0x0020 ; // bits6:4 = 010 => The update event is selected as trigger output (TRGO).

	//--------------------  Interrupt TIM4  -------------------------
	// set priority level lower than EXTI3  (CS)
	NVIC->IP[TIM3_IRQn] = 0x40; // 01.00 0000 => Priority level = 1 (low)	
    // Enable the Selected IRQ Channels 
    NVIC->ISER[TIM3_IRQn >> 5] = (uint32_t)(0x01 << (TIM3_IRQn & 0x1F));	 
    // Enable the channel 2 Interrupt: DIER_CC2IE (Bit2)
    TIM3->DIER |= (uint16_t)0x0004;  //  Bit2 = 1 => CC2 interrupt enabled   
	//-------------------------------------------

    // Enable the TIM3 Counter: CR1_CEN  (bit0) 
    TIM3->CR1 |= 0x0001; // bit0 = 1 => timer enable
}

//***********************************************************************
// VSYNC (TIM4_CH1) and VSYNC_BACKPORCH (TIM4_CH2)
void TIM4_Config(){
	// 640x480 25.175 MHz
	uint16_t TimerPeriod = 524;		// Vertical lines: 31468.75/X= 60 Hz => 524.479166
	uint16_t Channel1Pulse = 2;		// VSync pulse Y*(1/31468.75) = 63.55511us => 1.99999 
	uint16_t Channel2Pulse = 35;	// VSync pulse+Back porch Z*(1/31468.75) = 1112.2141us => 34.9999
									// Back porch = 1112.2141 - 63.55511 = 1048.659us
	
	// Select the Slave Mode:The counter clock is enabled when the trigger input (TRGI) is high.
	// Bits6:4 = 010 => Internal Trigger 2 (ITR2) selected => TIM3_TRGO
	TIM4->SMCR = 0x0025; // Bits2:0 = 101 => Gated Mode 
	
	// Set the Autoreload value
	TIM4->ARR = TimerPeriod ; // 
 
	// Generate an update event to reload the Prescaler and 
	// the Repetition counter values immediately
	TIM4->EGR = (uint16_t)0x0001; // EGR_UG (Bit0) = 1 PSC Reload Mode Immediate            

	//------------------------  Channel1 ----------------
	TIM4->CCMR[0] = 0x0070; // bits6:4 = 111 => PWM mode 2
	//TIM4->CCER = 0x0003; // Bit1 = 0 => OC1 active high, Bit0 = 1 => OC1 On (800x600)
	TIM4->CCER = 0x0001; // Bit1 = 1 => OC1 active low, Bit0 = 1 => OC1 On (640x480)
	// Set the Capture Compare Register value
	TIM4->CCR[0] = Channel1Pulse; // VSYNC pulse width
	//------------------------  Channel2 ----------------	
	// Bits14:12 = 010 => Set channel 2 to inactive level on match (OCMode_Inactive)
	TIM4->CCMR[0] |= 0x2000;
  
	// Set the Capture Compare Register CCR2 value
	TIM4->CCR[1] = Channel2Pulse; // VSync pulse + Back porch
	//--------------------  Interrupt TIM4  -------------------------
	// set priority level lower than EXTI3  (CS)
	NVIC->IP[TIM4_IRQn] = 0x40; // 01.00 0000 => Priority level = 1 (low)	
    // Enable the Selected IRQ Channels 
    NVIC->ISER[TIM4_IRQn >> 5] = (uint32_t)(0x01 << (TIM4_IRQn & 0x1F));	 
    // Enable the channel 2 Interrupt: DIER_CC2IE (Bit2)
    TIM4->DIER |= (uint16_t)0x0004;  //  Bit2 = 1 => CC2 interrupt enabled   
	//-------------------------------------------

    // Enable the TIM4 Counter: CR1_CEN  (bit0) 
    TIM4->CR1 |= 0x0001; // bit0 = 1 => timer enable
}

///************************************************************
void tims_init(){
	
	unsigned int ra;
	
	// Enable clocks for GPIOB, TIMER3 & TIMER4
	RCC->APB1ENR |= (1<<2) | (1<<1);     // enable timer3 and timer4 clocks
	RCC->AHB1ENR |= 1<<1;		// enable GPIOB clock

	//--------------------------------
    //Configure GPIOB mode reg
	ra = GPIOB->MODER;			// read GPIOB_MODER
    ra &= ~(3<<8); 				//clear PB4 mode bits
    ra |= 2<<8; 				//PB4 alternate function mode
	GPIOB->MODER = ra;
    // Configure GPIOB alternate function low register (GPIOB_AFRL)
    ra = GPIOB->AFRL; 			// Read GPIOB_AFRL
    ra &= ~(0xF<<16); 			//clear PB4 AF bits
    ra |=    2<<16;  			//change PB4 to AF 2 (TIM3 ch1)
    GPIOB->AFRL = ra;
	//--------------------------------
    //Configure GPIOB mode reg
	ra = GPIOB->MODER;			// read GPIOB_MODER
    ra &= ~(3<<12); 			//clear PB6 mode bits
    ra |= 2<<12; 				//PB6 alternate function mode
	GPIOB->MODER = ra;
    // Configure GPIOB alternate function low register (GPIOB_AFRL)
    ra = GPIOB->AFRL; 			// Read GPIOB_AFRL
    ra &= ~(0xF<<24); 			//clear PB6 AF bits
    ra |=    2<<24;  			//change PB6 to AF 2 (TIM4 ch1)
    GPIOB->AFRL = ra;

	// configure timers
	TIM3_Config();
	TIM4_Config();
}
	
//============================================
void spi_init(){
	unsigned int ra;
	RCC->AHB1ENR |= 1<<1;		// enable GPIOB clock

    //Configure PB5 as alternate function 5 high speed 
	// Configure GPIOB mode reg
	ra = GPIOB->MODER;			// read GPIOB_MODER
    ra &= ~(3<<10); 			//clear PB5 mode bits
    ra |= (2<<10); 				//PB5 alternate function mode
	GPIOB->MODER = ra;
	
    //Configure GPIOB output speed reg
    ra = GPIOB->OSPEEDR; 			// read GPIOB_OSPEEDR
    ra&=~(3<<10); 				//clear PB5 speed bits
    ra|= 2<<10; 				// PB5 = Fast speed
    GPIOB->OSPEEDR = ra;
	
    // Configure GPIOB alternate function low register (GPIOB_AFRL)
    ra = GPIOB->AFRL; 			// Read GPIOB_AFRL
    ra&=~(0xF<<20); 			//clear PB5 AF bits
    ra|=    5<<20;  			//change PB5 to AF 5 (SPI1 MOSI)
    GPIOB->AFRL = ra;
	

	// Configure SPI
    RCC->APB2ENR |= 1<<12;  //set bit12 => enable SPI1
	
	// MUST set bit2 (SSOE) in SPI_CR2 !!
	SPI1->CR2 = 1<<2 | 1<<1 ;  // set bit1 (TXDMAEN) TX dma and set bit2 SS output enable (SSOE)

	//now configure SPI control register 1 (SPI_CR1)
	// B15 (BIDIMODE) =1 1-line bidirectional data mode, b14 (BIDIOE) =1 Output enabled (transmit-only mode)
	// Bits 5:3 BR[2:0] = 001 ==> SPI clk = Fcpu/4 = (1005.5)/4 = 25.125 MHz
	SPI1->CR1 = 3 <<14 | 1<<6 | 1<<3 | 1<<2;  //  b2=1 => master, b6=1 enable SPI
}

//============================================
//Initialise the DMA2 Stream 3 Channel 3 for SPI1_TX DMA access
void dma_init(){
	uint32_t DMA2_stream3_IRQn = 59;
	
    RCC->AHB1ENR |= 1<<22;  //set bit22 ==> enable DMA2
  
	// configure s3->cr
	DMA2->S3[0] = 0x06020440; //0000 0110 0000 0010 0000 0100 0100 0000
	//configure memory address in s3->S3M0AR
	DMA2->S3[3] = (unsigned int)&fb[0][0];
	//configure periph address in S3->S3PAR
	DMA2->S3[2] = (unsigned int)&(SPI1->DR);
	//configure count (number of data) in S3->S3NDTR)
	DMA2->S3[1] = (unsigned int)VTOTAL; // 100 bytes = 1 ligne (800 pixels)
	DMA2->S3[5] = 0; //DISABLE FIFO

	// set priority level lower than EXTI3  (CS)
	NVIC->IP[DMA2_stream3_IRQn] = 0x40; // 01.00 0000 => Priority level = 1 (low)	
    // Enable DMA2 stream3 interrupt in NVIC
    NVIC->ISER[DMA2_stream3_IRQn >> 5] = (uint32_t) (0x01 << (DMA2_stream3_IRQn & 0x1F));
    // set  Bit4 (TCIE) DMA2 stream3  Transfer complete interrupt
    DMA2->S3[0] |= 1<<4; // Bit4 = 1 => Transfer complete interrupt enabled

	//enable stream
	DMA2->S3[0] |=  1; // Set bit0 (EN)	
}

//=============================
// clear Screen 
void cls() __attribute__((section(".ramfunc")));
void cls(){
	uint16_t x, y;

	for (y = 0; y < VID_VSIZE; y++){
		for (x = 0; x < VTOTAL; x++) fb[y][x] = 0;
	}	
	con_x = 8; // skip 8 pixels in begining of each line 
	con_y = 4;  // skip 4 VGA lines at begining of frame	
}

//*****************************************************************************
//	Function bitBlt(i16 x, i16 y, i16 w, i16 h, pu8 bm)
//	Bit Block Transfer funcion. This function uses the STM32 Bit-Banding mode
//	to simplify the complex BitBlt functionality.
//	Parameters:
//		 the coordinates are the entire display area 	  ___x________________
//		x	 X start position in display			 	 |      |
//		y	 Y start position in display			 	 |..y..+---w----
//		w	 area width, in pixels					 	 |     | h     |
//		h	 area height, in pixels					 	 |     |_______|
//		bm	 Pointer to the bitmap to draw on screen	 |____________________
//**************************************************************************
void bitBlt(i16 x, i16 y, i16 w, i16 h, pu8 bm) __attribute__((section(".ramfunc")));
void bitBlt(i16 x, i16 y, i16 w, i16 h, pu8 bm){

	u16		i, xz, xb, xt;
	u32		wb;					// Width in bytes
	u32		r;					// Start X position in bits (relative to x)
	u32		k;				
	u32		d;
	u32		offs;
	u8		c;
	pu8		fbPtr;				// Pointer to the Frame Buffer Bit-Band area
	pu8		fbBak;
	u8		fb1;
	u32		fb2;
	u32		rp;
	pu8		bmPtr;				// Pointer to the bitmap bits

	//	Get total bitmap width in bytes
	wb = (u32) w >> 3; // wb = w/8
	if ((wb << 3) < (u32) w) ++wb;

	//	Get starting bit inside the first byte
	d = (u32) x >> 3; // d = x/8
	r  = ((u32) x - (d << 3));

	//	Clip X
	if ((x + w) >= VID_PIXELS_X ) xt =  VID_PIXELS_X - x; 
	else xt = w;

	//	Draw bits
	for (i = 0; i < h; i++) {
		//	Clip Y
		if ((i + y) > (VID_VSIZE - 1)) return;

		//	Get offset to frame buffer in bit-banding mode
		offs = (((u32) x >> 3)) + ((u32) (y + i)  * VID_HSIZE);
		k = (u32) (&fb - 0x20000000);
		k += offs;
		fbPtr = (pu8) (0x22000000 + (k * 32) + ((7 - r) * 4));
		fbBak = (pu8) (0x22000000 + (k * 32) + 28);

		//	Get offset to bitmap bits
		bmPtr = bm + ((u32) i * wb);
		xz = w;

		xb = 0;
		for (xz = 0; xz < xt; xz++) {
			fb1 = ((u32) fbPtr) & 0x000000E0;
			if (xb++ == 0) {
				c = *bmPtr;
				++bmPtr;
			}
			xb &= 0x07;
			(c & 0x80) ? (rp = 1) : (rp = 0);				
			*fbPtr = rp;
			fbPtr -= 4;
			fb2 = ((u32) fbPtr) & 0x000000E0;
			if (fb1 != fb2) {
				fbPtr = fbBak + 32;
				fbBak = fbPtr;
			}
			c <<= 1;
		}
	}
}

//*****************************************************************************
void scroll_up() __attribute__((section(".ramfunc")));
void scroll_up(){

	u16 i, j;

	pu8 dp, sp; // source and distination pointers
	dp = &fb[8][0]; // destination pointer: first text line (remember we skip the first 8 VGA lines)
	sp = &fb[8 + FONT_HEIGHT][0]; // source pointer: second line
	
	// from the 73 lines we copy the first 72 (0 to 71)
	// we copy line+1 in line
	for(i =0; i<72; i++){  
		for(j=0; j< 8*100; j++) *dp++ = *sp++; // copy byte from source to destination
	}
	//clear last line
	sp = &fb[8+72*8][0];
	for(j=0; j< 8*100; j++)  *sp++ = 0; // clear byte	
}

//*****************************************************************************
void newl(u8 c) __attribute__((section(".ramfunc")));
void newl(u8 c){
	
	if(c == '\n') {// new line
		con_y += FONT_HEIGHT;
		if (con_y >= VID_PIXELS_Y - 8){
			con_y = VID_PIXELS_Y - 8;
			scroll_up();
			con_y -= FONT_HEIGHT; // return to line	72		
		}		
		return;
	}	
	if(c == '\r') {// feed back
		con_x = 16; 
		return;
	}				
}

//***************************************************************************
void draw_char(u8 c) __attribute__((section(".ramfunc")));
void draw_char(u8 c){

	u16  pos;
	pu8	 ptx; // pointer to char
	
	pos = (u16) (c - FONT_OFFSET) * FONT_HEIGHT;
	ptx = ((pu8) font) + pos;
	bitBlt(con_x, con_y, FONT_WIDTH, FONT_HEIGHT, ptx);
	con_x += FONT_WIDTH;  // next char pos
	if (con_x >= VID_PIXELS_X - 8){ // if end of line - 8 pixels
		con_x = 8; // skip 8bits at the begining of each line
		con_y += FONT_HEIGHT; //next line
	}
}


//*******************************
void print_str(char *msg) __attribute__((section(".ramfunc")));
void print_str(char *msg){

	//while(*msg) draw_char(*msg++);
	
	for(;*msg;msg++){ // while char different from zero
		//if(*msg == '\r')  con_x = 8; // if feedback char reset X counter
		//else if(*msg == '\n')	con_y += FONT_HEIGHT; // if next line char add font height to Y position
		
		if(*msg == '\n')	{con_x = 8; con_y += FONT_HEIGHT;}	
		else	draw_char(*msg); // else display char
	}
}

//*****************************************************************************
void goto_col(u8 col) __attribute__((section(".ramfunc")));
void goto_col(u8 col){
	col -= 1; 						 // if we start counting from 1
	con_x = (col * FONT_WIDTH) + 16; 	// we start each line from 16th pixel
}

//*****************************************************************************
void goto_row(u8 row) __attribute__((section(".ramfunc")));
void goto_row(u8 row){
	row -= 1;  						// if we start counting from 1
	con_y = (row * FONT_HEIGHT) + 8; // we skip first 8 VGA lines
}

//*****************************************************************************
// back space
void del_char() __attribute__((section(".ramfunc")));
void del_char(){
	u16  pos;
	pu8	 ptx; // pointer to char
	char c;

	con_x -= FONT_WIDTH; // return back one char
	if(con_x < 16){ // start of line
		con_y -= FONT_HEIGHT; // go back to previous line
		if(con_y == 8) return; // if first line quit
		con_x = ((128-1)*FONT_WIDTH); // else put cursor at last culumn of the row
	}
	c = ' ';   // delete char	
	pos = (u16) (c - FONT_OFFSET) * FONT_HEIGHT;
	ptx = ((pu8) font) + pos;
	bitBlt(con_x, con_y, FONT_WIDTH, FONT_HEIGHT, ptx);
} 

//============================
void vid_init(void){
	dma_init();
	spi_init();	
	tims_init();
}

//====================================================================
// convert a byte from binary to HEX ASCII and display it
void b2h(uint8_t byte){  
	uint8_t y;
   
	static char const hexchars[16] = "0123456789ABCDEF" ; 
	y=((byte&0xF0)>>4);  // get high nible and shift it to right 4 times
	y=hexchars[y % 16];
	draw_char(y);
	y=(byte&0x0F);  // get low nible
	y=hexchars[y % 16];
	draw_char(y);
	draw_char(' ');
}

//=============================================
void h2a(unsigned int n){
    unsigned int a;
    unsigned int b;
	
	if(n < 0x100) a =8;
	else if(n < 0x10000) a =16;
    else a=32;
	
	while(a){
        a -= 4;
        b = (n>>a)&0xF;
        if(b>9) b += 0x37; else b += 0x30; // 0x30 = '0', 0x37 = 'A' - 10
        draw_char(b);
    }
    draw_char(0x20);  	// 0x20 = ' ' space
}


// Function prototype placed in RAM section
void TIM3_IRQHandler() __attribute__((section(".ramfunc")));
void TIM4_IRQHandler() __attribute__((section(".ramfunc")));
void DMA2_Stream3_IRQHandler() __attribute__((section(".ramfunc")));


//*****************************************************************************
//	This irq is generated at the end of the horizontal back porch.
//	Test if inside a valid vertical start frame (vflag variable), 
//	and start the DMA to output a single frame buffer line through the SPI device.
//*****************************************************************************
void TIM3_IRQHandler(void){
	TIM3->SR = 0xFFFB; //reset TIMER3 CC2  interrupt flag 
	if (vflag) DMA2->S3[0] |=  1; // Set bit0 (EN) : enable DMA2 stream3
}

//*****************************************************************************
//	This irq is generated at the end of the vertical back porch.
//	Sets the 'vflag' variable to 1 (valid vertical frame).
//*****************************************************************************
void TIM4_IRQHandler(void){
	TIM4->SR = 0xFFFB; //reset TIMER4 CC2  interrupt flag
	vflag = 1;	
}

//*****************************************************************************
//	This interrupt is generated at the end of every line.
//	It will increment the line number and set the corresponding line pointer
//	in the DMA2 stream3 register.
// Upon exit, the stream is set up but disabled.
//*****************************************************************************
void DMA2_Stream3_IRQHandler(void){	
	DMA2->LIFCR = 1 << 27; 		// clear DMA2 stream3 transfer complete bit 
	DMA2->S3[0] &=  ~(1<<0); 	// clear bit0 (EN) in s3->cr to disable the stream
	DMA2->S3[1] = VTOTAL; 		// reload the number of bytes to send

	vline++;		
	if (vline == VID_VSIZE){
		vline = vflag = 0;
		DMA2->S3[3] = (unsigned int)&fb[0][0];			
	} 
	else DMA2->S3[3] += VTOTAL;
}

//**************************************************************
// font 8x8 (used 6x8), each char = 8 bytes
const unsigned char font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 		// 0x20 ' '
	0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20, 0x00, 		// 0x21 '!'
	0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 		// 0x22 '"'
	0x00, 0x50, 0xF8, 0x50, 0xF8, 0x50, 0x00, 0x00, 		// 0x23 '#'
	0x20, 0x78, 0xA0, 0x70, 0x28, 0xF0, 0x20, 0x00, 		// 0x24 '$'
	0xC8, 0xD0, 0x20, 0x20, 0x20, 0x58, 0x98, 0x00, 		// 0x25 '%'
	0x20, 0x50, 0x20, 0x40, 0xA8, 0x98, 0x70, 0x00, 		// 0x26 '&'
	0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 		// 0x27 '''
	0x20, 0x40, 0x80, 0x80, 0x80, 0x40, 0x20, 0x00, 		// 0x28 '('
	0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00, 		// 0x29 ')'
	0x00, 0xa8, 0x70, 0xf8, 0x70, 0xa8, 0x00, 0x00, 		// 0x2A '*'
	0x00, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x00, 0x00, 		// 0x2B '+'
	0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x40, 0x00, 		// 0x2C ','
	0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 		// 0x2D '-'
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 		// 0x2E '.'
	0x10, 0x10, 0x20, 0x20, 0x20, 0x40, 0x40, 0x00, 		// 0x2F '/'
	0x70, 0x88, 0x88, 0xA8, 0x88, 0x88, 0x70, 0x00, 		// 0x30 '0'
	0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 		// 0x31 '1'
	0x70, 0x88, 0x08, 0x70, 0x80, 0x80, 0xF8, 0x00, 		// 0x32 '2'
	0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70, 0x00, 		// 0x33 '3'
	0x10, 0x30, 0x50, 0x90, 0xF8, 0x10, 0x10, 0x00, 		// 0x34 '4'
	0xf0, 0x80, 0x80, 0xf0, 0x08, 0x88, 0x70, 0x00, 		// 0x35 '5'
	0x30, 0x40, 0x80, 0xB0, 0xC8, 0x88, 0x70, 0x00, 		// 0x36 '6'
	0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x00, 		// 0x37 '7'
	0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70, 0x00, 		// 0x38 '8'
	0x70, 0x88, 0x88, 0x78, 0x08, 0x88, 0x70, 0x00, 		// 0x39 '9'
	0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x00, 0x00, 		// 0x3A ':'
	0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x60, 0x00, 		// 0x3B ';'
	0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x00, 		// 0x3C '<'
	0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 		// 0x3D '='
	0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40, 0x00, 		// 0x3E '>'
	0x70, 0x88, 0x08, 0x10, 0x20, 0x00, 0x20, 0x00, 		// 0x3F '?'
	0x70, 0x88, 0xb8, 0xa8, 0xb8, 0x80, 0x78, 0x00, 		// 0x40 '@'
	0x70, 0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x00, 		// 0x41 'A'
	0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0, 0x00, 		// 0x42 'B'
	0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x00, 		// 0x43 'C'
	0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x00, 		// 0x44 'D'
	0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0xF8, 0x00, 		// 0x45 'E'
	0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80, 0x00, 		// 0x46 'F'
	0x70, 0x88, 0x80, 0xB8, 0x88, 0x88, 0x70, 0x00, 		// 0x47 'G'
	0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88, 0x00, 		// 0x48 'H'
	0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 		// 0x49 'I'
	0x08, 0x08, 0x08, 0x08, 0x88, 0x88, 0x70, 0x00, 		// 0x4A 'J'
	0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88, 0x00, 		// 0x4B 'K'
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x00, 		// 0x4C 'L'
	0x88, 0xD8, 0xA8, 0x88, 0x88, 0x88, 0x88, 0x00, 		// 0x4D 'M'
	0x88, 0xC8, 0xA8, 0x98, 0x88, 0x88, 0x88, 0x00, 		// 0x4E 'N'
	0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 		// 0x4F 'O'
	0xf0, 0x88, 0x88, 0xf0, 0x80, 0x80, 0x80, 0x00, 		// 0x50 'P'
	0x70, 0x88, 0x88, 0x88, 0xa8, 0x90, 0x68, 0x00, 		// 0x51 'Q'
	0xf0, 0x88, 0x88, 0xf0, 0xa0, 0x90, 0x88, 0x00, 		// 0x52 'R'
	0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70, 0x00, 		// 0x53 'S'
	0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 		// 0x54 'T'
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 		// 0x55 'U'
	0x88, 0x88, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00, 		// 0x56 'V'
	0x88, 0x88, 0x88, 0x88, 0xa8, 0xa8, 0x70, 0x00, 		// 0x57 'W'
	0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88, 0x00, 		// 0x58 'X'
	0x88, 0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x00, 		// 0x59 'Y'
	0xf8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xf8, 0x00, 		// 0x5A 'Z'
	0x70, 0x40, 0x40, 0x40, 0x40, 0x40, 0x70, 0x00, 		// 0x5B '['
	0x40, 0x40, 0x20, 0x20, 0x20, 0x10, 0x10, 0x00, 		// 0x5C '\'
	0x70, 0x10, 0x10, 0x10, 0x10, 0x10, 0x70, 0x00, 		// 0x5D ']'
	0x00, 0x20, 0x50, 0x88, 0x00, 0x00, 0x00, 0x00, 		// 0x5E '^'
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 		// 0x5F '_'
	0x20, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 		// 0x60 '`'
	0x00, 0x00, 0x70, 0x08, 0x78, 0x98, 0x68, 0x00, 		// 0x61 'a'
	0x00, 0x80, 0x80, 0xF0, 0x88, 0x88, 0xF0, 0x00, 		// 0x62 'b'
	0x00, 0x00, 0x78, 0x80, 0x80, 0x80, 0x78, 0x00, 		// 0x63 'c'
	0x00, 0x08, 0x08, 0x78, 0x88, 0x88, 0x78, 0x00, 		// 0x64 'd'
	0x00, 0x00, 0x70, 0x88, 0xF0, 0x80, 0x70, 0x00, 		// 0x65 'e'
	0x30, 0x48, 0x40, 0xE0, 0x40, 0x40, 0x40, 0x00,  		// 0x66 'f'
	0x00, 0x00, 0x78, 0x88, 0x78, 0x08, 0x70, 0x00, 		// 0x67 'g'
	0x00, 0x80, 0x80, 0xB0, 0xC8, 0x88, 0x88, 0x00, 		// 0x68 'h'
	0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00,  		// 0x69 'i'
	0x00, 0x10, 0x00, 0x10, 0x10, 0x90, 0x60, 0x00,  		// 0x6A 'j'
	0x00, 0x80, 0x90, 0xA0, 0xE0, 0x90, 0x90, 0x00, 		// 0x6B 'k'
	0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 		// 0x6C 'l'
	0x00, 0x00, 0xD0, 0xA8, 0xA8, 0x88, 0x88, 0x00,  		// 0x6D 'm'
	0x00, 0x00, 0xB0, 0xC8, 0x88, 0x88, 0x88, 0x00,  		// 0x6E 'n'
	0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70, 0x00, 		// 0x6F 'o'
	0x00, 0x00, 0xF0, 0x88, 0xF0, 0x80, 0x80, 0x00, 		// 0x70 'p'
	0x00, 0x00, 0x68, 0x98, 0x78, 0x08, 0x08, 0x00, 		// 0x71 'q'
	0x00, 0x00, 0xB0, 0xC8, 0x80, 0x80, 0x80, 0x00, 		// 0x72 'r'
	0x00, 0x00, 0x70, 0x80, 0x70, 0x08, 0xF0, 0x00, 		// 0x73 's'
	0x40, 0xE0, 0x40, 0x40, 0x40, 0x48, 0x30, 0x00, 		// 0x74 't'
	0x00, 0x00, 0x88, 0x88, 0x88, 0x98, 0x68, 0x00, 		// 0x75 'u'
	0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00, 		// 0x76 'v'
	0x00, 0x00, 0x88, 0x88, 0x88, 0xA8, 0x50, 0x00, 		// 0x77 'w'
	0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88, 0x00, 		// 0x78 'x'
	0x00, 0x00, 0x88, 0x88, 0x78, 0x08, 0x70, 0x00, 		// 0x79 'y'
	0x00, 0x00, 0xF8, 0x10, 0x20, 0x40, 0xF8, 0x00, 		// 0x7A 'z'
	0x20, 0x40, 0x40, 0x80, 0x40, 0x40, 0x20, 0x00, 		// 0x7B '{'
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 		// 0x7C '|'
	0x80, 0x40, 0x40, 0x20, 0x40, 0x40, 0x80, 0x00, 		// 0x7D '}'
	0x00, 0x00, 0x48, 0xA8, 0x90, 0x00, 0x00, 0x00, 		// 0x7E '~'
	0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00, 		// 0x7F  full square
};

