
.section .vectors
.cpu cortex-m4
.thumb

.word   0x20020000  // stack pointer => top of RAM 
.word   resetHandler      // 1 Reset 
.word   spin        // 2 NMI 
.word   spin        // 3 Hard Fault 
.word   spin        // 4 MM Fault 
.word   spin        // 5 Bus Fault 
.word   spin        // 6 Usage Fault 
.word   spin        // 7 RESERVED 
.word   spin        // 8 RESERVED 
.word   spin        // 9 RESERVED
.word   spin        // 10 RESERVED 
.word   spin        // 11 SV call 
.word   spin        // 12 Debug reserved 
.word   spin        // 13 RESERVED 
.word   spin        // 14 PendSV 
.word 	spin   		// 15 SysTick 

@ and now 68 IRQ vectors
.word   spin           // IRQ  0
.word   spin           // IRQ  1
.word   spin           // IRQ  2
.word   spin           // IRQ  3 -- RTC
.word   spin           // IRQ  4
.word   spin           // IRQ  5
.word   spin           // IRQ  6
.word   spin           // IRQ  7
.word   spin           // IRQ  8
.word   spin           // IRQ  9 -- EXTI3 (EXTI3_IRQHandler)
.word   spin           // IRQ 10
.word   spin           // IRQ 11
.word   spin           // IRQ 12
.word   spin           // IRQ 13
.word   spin           // IRQ 14
.word   spin           // IRQ 15
.word   spin           // IRQ 16
.word   spin           // IRQ 17
.word   spin           // IRQ 18
.word   spin           // IRQ 19
.word   spin           // IRQ 20
.word	spin		// IRQ 21
.word	spin		// IRQ 22
.word	spin		// IRQ 23 -- exti9_5
.word	spin		// IRQ 24 -- Timer 1 break
.word	spin		// IRQ 25 -- Timer 1 update
.word	spin		// IRQ 26 -- Timer 1 trig
.word	spin		// IRQ 27 -- Timer 1 cc
.word	spin		// IRQ 28 -- Timer 2
.word	spin		// IRQ 29 -- Timer 3
.word	spin		// IRQ 30 -- Timer 4
.word	spin		// IRQ 31
.word	spin		// IRQ 32
.word	spin		// IRQ 33
.word	spin		// IRQ 34
.word	spin		// IRQ 35
.word	spin		// IRQ 36
.word	spin		// IRQ 37 -- UART 1
.word	spin		// IRQ 38 -- UART 2
.word	spin		// IRQ 39 -- UART 3
.word	spin		// IRQ 40 -- exti15_10
.word	spin		// IRQ 41
.word	spin		// IRQ 42
.word	spin		// IRQ 43
.word	spin		// IRQ 44
.word	spin		// IRQ 45
.word	spin		// IRQ 46
.word	spin		// IRQ 47
.word	spin		// IRQ 48
.word	spin		// IRQ 49
.word	spin		// IRQ 50
.word	spin		// IRQ 51
.word	spin		// IRQ 52
.word	spin		// IRQ 53
.word	spin		// IRQ 54
.word	spin		// IRQ 55
.word	spin		// IRQ 56
.word	spin		// IRQ 57
.word	spin		// IRQ 58
.word	spin		// IRQ 59 - DMA2 Stream3
.word	spin		// IRQ 60
.word	spin		// IRQ 61
.word	spin		// IRQ 62
.word	spin		// IRQ 63
.word	spin		// IRQ 64
.word	spin		// IRQ 65
.word	spin		// IRQ 66
.word	spin		// IRQ 67

.section .text	
.thumb_func
.global spin    // export to C
spin:
	wfi
	b spin
