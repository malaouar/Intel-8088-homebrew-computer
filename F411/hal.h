#include <stdint.h>

//===================  IWDG  ==================

typedef struct {
    volatile uint32_t KR;   // Key Register
    volatile uint32_t PR;   // Prescaler Register
    volatile uint32_t RLR;  // Reload Register
    volatile uint32_t SR;   // Status Register
} IWDG_Type;

#define IWDG ((IWDG_Type *)0x40003000UL)

//===================  SCB ==================

typedef struct {
    volatile uint32_t CPUID;   // CPUID Base Register
    volatile uint32_t ICSR;    // Interrupt Control State Register
    volatile uint32_t VTOR;    // Vector Table Offset Register
    volatile uint32_t AIRCR;   // Application Interrupt/Reset Control Register
    volatile uint32_t SCR;     // System Control Register
    volatile uint32_t CCR;     // Configuration Control Register
    volatile uint32_t SHPR[3]; // System Handler Priority Registers (SHPR1-3)
    volatile uint32_t SHCSR;   // System Handler Control/State Register   
    // Fault Registers
    volatile uint32_t CFSR;    // Configurable Fault Status Register
    volatile uint32_t HFSR;    // Hard Fault Status Register
    volatile uint32_t DFSR;    // Debug Fault Status Register
    volatile uint32_t MMFAR;   // MemManage Fault Address Register
    volatile uint32_t BFAR;    // Bus Fault Address Register
    volatile uint32_t AFSR;    // Auxiliary Fault Status Register
    // 0xE000ED40 - Reserved space
    uint32_t reserved1[18];    // Reserved 0x40-0x87   
    // 0xE000ED88
    volatile uint32_t CPACR;   // Coprocessor Access Control Register
} SCB_Type;

#define SCB ((SCB_Type *)0xE000ED00UL)

//==========================  USB  ==================
// Decice IN/OUT EPs registers
// OUT EPs have no TX fifo status register
struct endpoint{
										//  IN  / OUT
	volatile unsigned int DEPCTL;		// 0x900/0xB00 control
	    int pad0;
	volatile unsigned int DEPINT;		// 0x908/0xB08 interrupt
	    int pad1;
	volatile unsigned int DEPTSIZ;		// 0x910/0xB10 transfer size
		int pad2;
	volatile unsigned int DTXFSTS;		// 0x918  TX fifo status
	    int pad3;
};

//------------------------------------------------
typedef struct {
	volatile unsigned int GOTGCTL; 		// 0x000 control and status register (OTG_FS_GOTGCTL)
	volatile unsigned int GOTGINT; 		// 0x004 interrupt register (OTG_FS_GOTGINT)
	volatile unsigned int GAHBCFG; 		// 0x008 AHB configuration register (OTG_FS_GAHBCFG)
	volatile unsigned int GUSBCFG; 		// 0x00C USB configuration register (OTG_FS_GUSBCFG)
	volatile unsigned int GRSTCTL; 		// 0x010 reset register (OTG_FS_GRSTCTL)
	volatile unsigned int GINTSTS; 		// 0x014 core interrupt register (OTG_FS_GINTSTS)
	volatile unsigned int GINTMSK; 		// 0x018 interrupt mask register (OTG_FS_GINTMSK)
	volatile unsigned int GRXSTSR; 		// 0x01C Receive status debug read/OTG status read and pop registers
	volatile unsigned int GRXSTSP; 		// 0x020 (OTG_FS_GRXSTSR/OTG_FS_GRXSTSP)
	volatile unsigned int GRXFSIZ; 		// 0x024 Receive FIFO size register (OTG_FS_GRXFSIZ)
	volatile unsigned int DIEPTXF0; 	// 0x028 Endpoint 0 Transmit FIFO size (OTG_FS_DIEPTXF0)
	volatile unsigned int HNPTXSTS; 	// 0x02C non-periodic transmit FIFO/queue status register (OTG_FS_HNPTXSTS)
	int pad0[2];
	volatile unsigned int GCCFG; 		// 0x038 general core configuration register (OTG_FS_GCCFG)
	volatile unsigned int CID; 			// 0x03C core ID register (OTG_FS_CID)
	int pad1[48];
	volatile unsigned int HPTXFSIZ; 	// 0x100 Host periodic transmit FIFO size register (OTG_FS_HPTXFSIZ)
	volatile unsigned int DIEPTXF[3];	// 0x104 device IN endpoint transmit FIFO size register (OTG_FS_DIEPTXFx)
	int pad2[188];
	
	// Host mode registers.
	int pad3[256];						// 0x400
	
	volatile unsigned int DCFG; 		// 0x800 device configuration register (OTG_FS_DCFG)
	volatile unsigned int DCTL; 		// 0x804 device control register (OTG_FS_DCTL)
	volatile unsigned int DSTS; 		// 0x808 device status register (OTG_FS_DSTS)
	int pad4;
	volatile unsigned int DIEPMSK; 		// 0x810 device IN endpoint common interrupt mask register (OTG_FS_DIEPMSK)
	volatile unsigned int DOEPMSK; 		// 0x814 device OUT endpoint common interrupt mask register (OTG_FS_DOEPMSK)
	volatile unsigned int DAINT; 		// 0x818 device all endpoints interrupt register (OTG_FS_DAINT)
	volatile unsigned int DAINTMSK; 	// 0x81C all endpoints interrupt mask register (OTG_FS_DAINTMSK)
	int pad5[2];
	volatile unsigned int DVBUSDIS; 	// 0x828 device VBUS discharge time register (OTG_FS_DVBUSDIS)
	volatile unsigned int DVBUSPULSE; 	// 0x82C device VBUS pulsing time register (OTG_FS_DVBUSPULSE)
	int pad6;
	volatile unsigned int DIEPEMPMSK; 	// 0x834 device IN endpoint FIFO empty interrupt mask register: (OTG_FS_DIEPEMPMSK)
	int pad7[50];
	
	// Device IN EPs registers
	struct endpoint DIEP[4]; 			// 0x900 device IN endpoints control registers (OTG_FS_DIEPCTLx)
	int pad8[128 - 4*8];
	
	// Device OUT EPs registers
	struct endpoint DOEP[4]; 			// 0xB00 device OUT endpoints control registers (OTG_FS_DOEPCTLx)
	int pad9[192 - 4*8];
	
	volatile unsigned int PCGCCTL; 		// 0xE00 Power and clock gating control and status registers
} _OTG_FS;

#define OTG_FS ((_OTG_FS *) 0x50000000) // USB BASE adress

//-----------------------------------------
// A struct to represent the 4k fifo of an EP:
struct EP_fifo {
	volatile unsigned int PPR;		// Push_Pop Registers 0x1000, 0x2000, ...
	int pad[1023];
};

//===================== USART  ==================
typedef struct {
  volatile uint32_t SR;   // 0x00 Status register
  volatile uint32_t DR;   // 0x04 Data register
  volatile uint32_t BRR;  // 0x08 Baud rate register
  volatile uint32_t CR1;  // 0x0C Control register 1
  volatile uint32_t CR2;  // 0x10 Control register 2
  volatile uint32_t CR3;  // 0x14 Control register 3
  volatile uint32_t GTPR;  // 0x18 Guard time and prescaler register
} USART ;

#define USART1 ((USART *) 0x40011000) // USART1 BASE adress outer () a MUST!!!??


//===================  SYSCFG ==================
typedef struct {
  volatile unsigned int MEMRMP;    	// 0x000  memory remap register
  volatile unsigned int PMC;		// 0x004  peripheral mode configuration register
  volatile unsigned int EXTICR1;    // 0x008  external interrupt configuration register 1
  volatile unsigned int EXTICR2;	// 0x00C  external interrupt configuration register 2
  volatile unsigned int EXTICR3; 	// 0x010  external interrupt configuration register 3   
  volatile unsigned int EXTICR4;	// 0x014  external interrupt configuration register 1
  unsigned int reserved[2];
  volatile unsigned int CMPCR;   	 // 0x020  Compensation cell control register  
} _SYSCFG ;

#define SYSCFG	((_SYSCFG *)0x40013800) // SYSCFG base adress

//===================  EXTI ==================
typedef struct {
  volatile unsigned int IMR;    // 0x000  Interrupt mask register
  volatile unsigned int EMR;	// 0x004  Event mask register
  volatile unsigned int RTSR;   // 0x008  Rising trigger selection register
  volatile unsigned int FTSR;	// 0x00C Falling trigger selection register
  volatile unsigned int SWIER;  // 0x10 Software interrupt event register
  volatile unsigned int PR;     // 0x14 Pending register
} _EXTI ;

#define EXTI	((_EXTI *)0x40013C00) // EXTI base adress

//===================  NVIC ==================
typedef struct {
  volatile unsigned int ISER[8];    // 0x000  Interrupt Set Enable Register 
  unsigned int RESERVED0[24];
  volatile unsigned int ICER[8];    // 0x080  Interrupt Clear Enable Register 
  unsigned int RSERVED1[24];
  volatile unsigned int ISPR[8];    // 0x100  Interrupt Set Pending Register 
  unsigned int RESERVED2[24];
  volatile unsigned int ICPR[8];    // 0x180  Interrupt Clear Pending Register 
  unsigned int RESERVED3[24];
  volatile unsigned int IABR[8];    // 0x200  Interrupt Active bit Register 
  unsigned int RESERVED4[56];
  volatile unsigned char  IP[240];  // 0x300  Interrupt Priority Register (8Bit wide) 
  unsigned int RESERVED5[644];
  volatile  unsigned int STIR;      // 0xE00  Software Trigger Interrupt Register 
} STM32_NVIC ;

#define NVIC	((STM32_NVIC *) 0xe000e100) // NVIC base adress

//========================= SYSTICK ========================
typedef struct {
	volatile unsigned int CSR;
	volatile unsigned int RVR;
	volatile unsigned int CVR;
	volatile unsigned int CAL;
} _SYSTIC;

#define SYSTICK 	((_SYSTIC *) 0xE000E010)

//======================   TIMERS =========================
typedef struct {
	volatile unsigned int CR1;	  // 00  control register1 
	volatile unsigned int CR2;	  // 04  control register2 
	volatile unsigned int SMCR;	  // 08  slave mode control register 
	volatile unsigned int DIER;	  // 0c  DMA/Interrupt enable register 
	volatile unsigned int SR;	  // 10  status register 
	volatile unsigned int EGR;	  // 14  event generation register 
	volatile unsigned int CCMR[2];// 18  capture/compare mode registers 1 @ 2
	volatile unsigned int CCER;	  // 20  capture/compare enable register 
	volatile unsigned int CNT;	  // 24  counter 
	volatile unsigned int PSC;	  // 28  prescaler 
	volatile unsigned int ARR;	  // 2c  auto-reload register 
	volatile unsigned int RCR;    // 30  repetition counter register
	volatile unsigned int CCR[4]; // 34  capture/compare registers 1 TO 4 
	volatile unsigned int BDTR;   // 44  break and dead-time register
	volatile unsigned int DCR;	  // 48  DMA control register 
	volatile unsigned int DMAR;	  // 4c  DMA address for full transfer 
} TIMER;

#define TIM1	((TIMER *) 0x40010000)  // TIMER1 base address
#define TIM2	((TIMER *) 0x40000000)
#define TIM3	((TIMER *) 0x40000400)
#define TIM4	((TIMER *) 0x40000800)

#define TIMER1_CC_IRQ 27  
#define	TIMER2_IRQn	28   
#define	TIM3_IRQn	29
#define	TIM4_IRQn	30

//======================  RCC ==============================
typedef struct {
	volatile unsigned int CR;		//0 - control reg 
	volatile unsigned int PLLCFGR;	//4 - pll config 
	volatile unsigned int CFGR;		//8 - clock config 
	volatile unsigned int CIR;		//c - clock interrupt 
	volatile unsigned int AHB1RSTR;	//10 - AHB1 peripheral reset 
	volatile unsigned int AHB2RSTR;	//14 - AHB2 peripheral reset 
	int __pad1[2];
	volatile unsigned int APB1RSTR;	//20 - APB1 peripheral reset 
	volatile unsigned int APB2RSTR;	//24 - APB2 peripheral reset 
	int __pad2[2];
	volatile unsigned int AHB1ENR;	//30 - AHB1 peripheral enable 
	volatile unsigned int AHB2ENR;	//34 - AHB2 peripheral enable 
	int __pad3[2];
	volatile unsigned int APB1ENR;	//40 - APB1 peripheral enable 
	volatile unsigned int APB2ENR;	//44 - APB2 peripheral enable 
	int __pad4[2];
	volatile unsigned int AHB1LPENR;//50 - AHB1 peripheral enable in low power 
	volatile unsigned int AHB2LPENR;//54 - AHB2 peripheral enable in low power 
	int __pad5[2];
	volatile unsigned int APB1LPENR;//60 - APB1 peripheral enable in low power 
	volatile unsigned int APB2LPENR;//64 - APB2 peripheral enable in low power 
	int __pad6[2];
	volatile unsigned int BDCR;		//70 
	volatile unsigned int CSR;		//74 
	int __pad7[2];
	volatile unsigned int SSCGR;	//80 
	volatile unsigned int PLLI2SCFGR;//84 
	int __pad8;
	volatile unsigned int DCKCFGR;	//8c 
} _RCC;

#define RCC	((_RCC *)0x40023800)  

//======================= GPIO ===================
typedef struct {
	volatile unsigned int MODER;	// 0x00
	volatile unsigned int OTYPER;	// 0x04
	volatile unsigned int OSPEEDR;	// 0x08
	volatile unsigned int PUPDR;	// 0x0c
	volatile unsigned int IDR;		// 0x10
	volatile unsigned int ODR;		// 0x14
	volatile unsigned int BSRR;		// 0x18
	volatile unsigned int LCKR;		// 0x1c
	volatile unsigned int AFRL;		// 0x20
	volatile unsigned int AFRH;		// 0x24
} GPIO;

#define GPIOA ((GPIO *)0x40020000)
#define GPIOB ((GPIO *)0x40020400)
#define GPIOC ((GPIO *)0x40020800)


//======================= SPI/I2S =================
typedef struct {
	volatile unsigned int CR1;	// 0x00
	volatile unsigned int CR2;	// 0x04
	volatile unsigned int SR;	// 0x08
	volatile unsigned int DR;	// 0x0c
	volatile unsigned int CRCPR;	// 0x10
	volatile unsigned int RXCRCR;	// 0x14
	volatile unsigned int TXCRCR;	// 0x18
	volatile unsigned int I2SCFGR;	// 0x1c
	volatile unsigned int I2SPR;	// 0x20
} SPI;

#define SPI1 ((SPI *)0x40013000)
#define SPI2 ((SPI *)0x40003800)

//======================= DMA ==============
typedef struct {
	volatile unsigned int LISR;		// 0x00
	volatile unsigned int HISR;		// 0x04
	volatile unsigned int LIFCR;	// 0x08
	volatile unsigned int HIFCR;	// 0x0c
	volatile unsigned int S0[6];	// 0x10
	volatile unsigned int S1[6];	// 0x14
	volatile unsigned int S2[6];	// 0x18
	volatile unsigned int S3[6];	// 0x1c
	volatile unsigned int S4[6];	// 0x10
	volatile unsigned int S5[6];	// 0x14
	volatile unsigned int S6[6];	// 0x18
	volatile unsigned int S7[6];	// 0x1c
} DMA;

#define DMA1 ((DMA *)0x40026000)
#define DMA2 ((DMA *)0x40026400)
// DMA2_Stream3 interrupt No 59
//======================= FLASH ============
typedef struct {
	volatile unsigned int ACR;
	volatile unsigned int KEYR;
	volatile unsigned int OPTKEYR;
	volatile unsigned int SR;
	volatile unsigned int CR;
	volatile unsigned int OPTCR;
} _FLASH;

#define FLASH ((_FLASH *)0x40023c00)


