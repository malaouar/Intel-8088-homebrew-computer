; SD read/Write Soft INT13 OK, Keyboard Hard INT9 OK, Keyboard Soft INT16 OK, 
; Systick Hard INT8 OK, VGA Soft INT10 OK
;=========================================
    rom_size  = 0x200				; 512 bytes ROM 
    rom_start = 0x100000 - rom_size	; start address 
    rom_empty = 0xF4				; fill empty space with Halts (or NOPs or zeros or 0xFFs)

	BIOSSEG = 0x7FE0	     ; The BIOS copies itself to (512K-512b) = (0x80000 - 0x200) = 0x7FE00
	; we shift to right 4bits (segment)
;=========================================
; variables in RAM (BIOS vars area): 400h just after first 1024bytes used for INT vectors
ascii = 0x400	    ; ascii code from KB
tic = 0x401			; SYSTIC counter
;=========================================    
	use16

start:
	; copy BIOS from Flash to RAM
    mov  ax, cs
    mov  ds, ax
    xor  si, si 	; ds:si = source
    mov  ax, BIOSSEG
    mov  es, ax
    xor  di, di 	; es:di = Destination
    mov  cx, 512	; 512 bytes (the size of BIOS)
    rep 			; repeat    
	movsb			; copy from [ds:si] to [es:di] end decrement CX
	
	; Goto RAM
    jmp BIOSSEG:next	; CS=BIOSSEG, IP = next
next:
	; First do a dummy write
	; to switch F411 from ROM emulator to VGA_SD
	mov ax, 0xC000
	mov ds, ax			; DS = ROM base address
	xor si, si
	mov [si], al		; a dummy write
;=============================
	; init segments
	; DS = 0 in order to build the interrupts table at the beginning of RAM
	; and init the BIOS variables
	; ES = 0: SD root directory is copied in the first segment (overwritten by the kernel later)
	; SS = 0: use the first segment as a stack for BIOS and bootloader
	xor ax,ax		; AX=0000  
	mov ds,ax		; DS=0000
	mov es,ax
	mov ss,ax		
	mov ax,0x500	; stak bottom @ 500h => 256bytes from 400h
	mov sp,ax		; used only by BIOS and bootloader
					; The kernel uses it's stack as it wants
	
;============== HARDWARE INTERUPTS ==============
	; init interrupt vector type 8 (TIMER)
	mov word[0x8*4], SYSTIC 
	mov word[0x8*4+2], cs 
;--------------------------------       
	; init interrupt vector type 9 (KEYBOARD)
	mov word[0x9*4], KB
	mov word[0x9*4+2], cs	
	
;============== SOFTWARE INTERUPTS ==============      
	; init interrupt vector type 10 (VIDEO)
	mov word[0x10*4], INT10 
	mov word[0x10*4+2], cs 
;--------------------------------       
	; init interrupt vector type 13 (DISKETTE)
	mov word[0x13*4], INT13
	mov word[0x13*4+2], cs 
;--------------------------------       
	; init interrupt vector type 16 (KEYBOARD)
	mov word[0x16*4], INT16
	mov word[0x16*4+2], cs 
	
	;DON'T FORGET THIS !!!
	sti	; enable interrupts
			; or enable them in the kernel
	
;-------------------------
;main:
	; Init variables
	mov byte [tic], 10		; systic = 100ms so 10x100 = 1S
	mov byte [ascii], 0		; clear ascii var

	; Wait for F411 to switch to VGA/SD mode and to intialize it self
	call long_delay 	; MUST !!!
	; Print a greeting message
	mov si, msg	       ; Put string position into SI
	call print
	
	;------------------------ -------------      
	; Copy first sector (BOOTSECTOR) from SD to RAM at 0000:7C00h
	; INT13h copies to ES:DI (ES = 0 above)
	mov ah, 2		; function = Read
	mov al, 1		; Nbr of sectors to read (or write)
	mov cx, 1		; sector Nr 0 (1 - 1)
	mov bx, 0x7C00	; start of buffer in RAM where we copy the bootloader
	int 0x13		; go

	; Get the signature (the last two bytes in the boot sector) 
	mov si, 0x7C00
	mov ax, word [si + 0x1FE]	; 0x1FE is the offset of the signature
	cmp ax, 0xAA55				; BOOTABLE SD?
	jne error					; No
	; Yes, let's GO 
	jmp 0000:0x7C00 			;  execute BOOTSECTOR

error:
	; display an error message: "NOT A BOOTABLE SD !!"
	mov si, msg2		
	call print
hang:
	hlt
	jmp hang


;=====================================================
; Read sector delay
delay_r:
	push cx 			; we need sector Nr in multi sector read
	mov cx, 0xFFF
ld_r:
	loop ld_r
	pop cx
	ret
;------------------
;read one sector 
; Sector Nr in CX
; Buffer address where to save ES:DI
SD_RdSector:
	mov ax, 0xC000
	mov ds, ax			; DS = VGA base address
	mov si, cx			; SI = LOW byte of sector Nr
	mov ax, cx			; AH = HIGH part + command
	or ah, 0x80			; Function read sector
	mov [si], ah
	call delay_r 		; wait until VGA reads the sector

	; we copy the sector in ES:DI
	mov bx, 512			; 512 bytes
	xor si, si		
read_loop:
	mov al, [si] 	; Read byte from VGA [DS:SI]
	mov [es:di], al 	; AL --> buffer  [ES:DI]
	inc si
	inc di
	dec	bx				; 512 bytes received?
	jnz	read_loop		; No, then again
	; Yes
	ret		

;=================================================
; write one sector 
; Sector Nr in CX
; Buffer address from where to read DS:SI
SD_WrSector:
	mov ax, 0xC000
	mov es, ax			; DS = VGA base address
	mov di, cx			; DI = LOW byte of sector Nr
	mov ax, cx			; AX = HIGH part + command
	or ah, 0x40			; Function write sector
	mov [es:di], ah
	
	; send content of the buffer pointed to by DS:SI to SD 
	mov bx, 512			; Nr of bytes
	add si, 512			; we start reading from the end of the buffer  (see VGA code)         
write_loop:		      
	dec si
	mov al, [si]		; Read byte from buffer at buffer [DS:SI]  
	mov [es:di], al 	; and send it VGA [ES:DI]
	nop
	dec bx				; all 512 bytes read?
	jnz	write_loop		; No, then again
	; Yes
	ret	

;==================================
; VGA subroutines
;================================== 
; send command/ascii to VGA 
; Input: AL = command, ascii, X, Y  or graphic byte
; AH=0b0000 xxxx ==> LOW byte of DI = command or ascii
; AH=0b0010 xxxx ==> goto X   (LOW byte of DI=X)
; AH=0b0001 xxxx ==> goto Y   (LOW byte of DI=Y)
com_ascii:
	push es
	push di
	mov di, 0xC000
	mov es, di			; ES = VGA base address	
	mov di, ax
	mov [es:di], ah 	; send command/ascii to VGA
	pop di
	pop es
	ret
;---------------------
delay_vga:
	push cx
	mov cx, 100			; wait a moment
delv:
	loop delv
	pop cx
	ret

;-----------------------------------
print:
	cs lodsb		; Get char from string into AL, we use CS to load from CS:SI instead of DS:SI
	or al, al		; AL = 0?
	jz pdone		; yes,  char is zero (end of string) then return        
	; send AL to VGA
	mov ah, 9
	int 10h
	jmp print		; draw next character
pdone:
	ret

;===================================
;   INTERRUPTS ROUTINES
;============= HARD IRQs =========== 
; INT8
SYSTIC:
	;use this to implement a timer and/or for multitasking
	iret
;-------------------------------- 
; INT9       
KB:
	push ax
	push ds
	push si
	mov ax, 0xC000
	mov ds, ax		; DS = ROM/VGA base address
	xor si, si		; SI = 0
	mov al, [si] 	; Read byte from VGA [DS:SI]
	mov ds, si		; DS =0x0000
	mov byte [ascii], al	; save in  "ascii"
	pop si
	pop ds
	pop ax
	iret
		
;===============  SOFT IRQs ===========
; VIDEO
INT10:	
	or ah, ah		; AH= 0? 
	jz CLS			; yes, then clear screen
	cmp ah, 0x09	; print caracter AL= Code ascii , BL= attribute
	jz  V1
	cmp ah, 0x0E	; print caracter AL= Code ascii , BL= attribute
	jz  V1
	cmp ah, 0x20	; Function: goto X
	jz	g_x
	cmp ah, 0x10	; Function: goto Y
	jz	g_y
	jmp ret10
CLS:
	mov al, 0x05		; CLS command
V1:   ; ascii or command in  AL
	xor ah, ah			; AH = 0b0000 xxxx  
	call com_ascii		; send to VGA
	call delay_vga		; wait
	call delay_vga		; wait
	iret
g_x:
g_y:
	; AL = X or Y
	call com_ascii		; AH = 0b0010 xxxx or 0b0001 xxxx
	call delay_vga		; wait
ret10:
	iret

;---------------------------------------
INT13:	; DISK operations
    ;AL = number of sectors to read/write (must be nonzero)
    ;Cx = Sector to start read/write from  (in LBA form not CHS) 
    ;BX = RAM buffer pointer.
	PUSH	SI
	PUSH	DI
	PUSH	DS
	PUSH	ES
	PUSH	BX		; used as byte counter
	PUSH	DX		; used as sector counter
	; Wich function ?
	or ah, ah		; AH= 0 (INIT)?
	jz ret13		; SD already initialized
	dec ah			; skip ah =1
	dec ah			; AH =2 Read
	jz Rd_Sect		; Yes
	dec ah			; AH = 3  (Write)
	jz Wr_Sect		; Yes
	jmp ret13		; Else return
	
;---------------------------------
; Sector Nr in CX, count in AL
; Buffer address where to save ES:BX
Rd_Sect:
	mov di, bx	  ; Change BX with DI as offset where we save data 
	mov dl, al	  ; save Nr of sectors to read in DL 
RS:
	call SD_RdSector
	inc cx		    ; next sector
	dec dl
	jnz RS		    ; done? No, again
	jmp ret13		; Yes,  return
;-------------------------------
Wr_Sect:
	mov si, bx		; Change BX by SI as offset from where we read
	mov dl, al		; save Nr of sectors to write in DL 
WS:
	push si			; save offset (SI is decremented in SD_WrSector)
	call SD_WrSector	;
	inc cx		    ; next sctor
	pop si			; restor offset
	add si, 512		; point next sector in buffer
	dec dl
	jnz WS		       ; done? No, again         
ret13:		; RESTORE STATE
	POP	DX
	POP BX
	POP	ES
	POP	DS
	POP	DI
	POP	SI
	iret			

;--------------------------------------------
; KEYBOARD
INT16:
	PUSH DS 
	xor ax, ax
	mov DS, ax				; DS=0 (we need ascii variable)
	mov al, byte [ascii]	; AL = ascii code  
	mov byte [ascii], 0		; reset
	POP	DS
	iret					; return AH=0,  AL= ASCII code OR 0

;====================
long_delay:
	push cx
	mov cx, 0xFFFF
del1:
    loop del1
del2:
    loop del2
	pop cx
    ret
		
;===============================================
msg db '...SEB-SEB COMPUTERS ...', 10, 13, 0 
msg2 db 'NOT A BOOTABLE SD!!', 0 

;===============================================
	
   times rom_size - 16 - $ db rom_empty 	; Fill free space with F4h until last 16 bytes
;-----------------------------------------
; reset vector: FFFF0
; Far jump to start address of BIOS
    jmp (rom_start shr 4):start 			; put start address of code in the begin of last 16 bytes
;-----------------------------------------
    times rom_size - $ db rom_empty			; Fill the remaining of 16 bytes
	