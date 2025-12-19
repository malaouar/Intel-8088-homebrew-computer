
;**************** ********************************************************
;       A Simple FAT16 Bootloader  Compile it with FASM
; 2 FATs of 9 sectors and a 224 entries root directory
; cluster size is 1 sector
; to free space for Kernel, this code copies it self  
; 512bytes under the BIOS and copies FAT under it 
;*************************************************************************
 
use16		; we are in 16 bit real mode

    LOADSEG = 0x0050	     ; The kenel will be loaded here
    BIOSSEG = 0x07C0	     ; here the boot sector is loaded
	; The BIOS is copied to (512K-512B)= (0x80000 - 0x200) = 0x7FE00
	; we shift to right 4bits (segment)
    BOOTSEG = 0x7FC0	     ; here it will copy itself (512K-512B-512B)= (0x80000 - 0x200-0x200) = 0x7FC00
	FATSEG =  0x7EA0		 ; here we load FAT 9*512 = 4608bytes (512K-(4608 +512 +512)) = (0x8000 - (0x1200+0x200+0x200))= 0x7EA00
 
start:	
	jmp	main	; jump to start of bootloader
	nop			; BPB Begins 3 bytes from start.
				; If you use a short jump, add a "nop" after it to offset the 3rd byte.
				; or do a far jump, which is 3 bytes in size.

;*********************************************
;       BIOS Parameter Block  "BPB"
;*********************************************
OEM:				DB "My OS   "	; OEM identifier (Cannot exceed 8 bytes!)
BytesPerSector: 	DW 512			; sector size
SectorsPerCluster:	DB 1			;
ReservedSectors:	DW 1			; Only the boot sector
NumberOfFATs:		DB 2
RootEntries:		DW 224
TotalSectors:		DW 0
Media:				DB 0xf8  
SectorsPerFAT:		DW 9
SectorsPerTrack:	DW 18
HeadsPerCylinder:	DW 2
HiddenSectors:		DD 0
TotalSectorsBig:	DD 131070
DriveNumber:		DB 0
Unused: 			DB 0
ExtBootSignature:	DB 0x29
SerialNumber:		DD 0xa0a1a2a3
VolumeLabel:		DB "MOS FLOPPY "
FileSystem:			DB "FAT16   "

;**********************************************
; At this point we have CS=DS=ES=SS= 0000
main:
; copy bootsector to bootseg
    mov  ax, BIOSSEG
    mov  ds, ax
    xor  si, si 	; ds:si = source
    mov  ax, BOOTSEG
    mov  es, ax
    xor  di, di 	; es:di = Destination
    mov  cx, 256	; 256  words to move = 512 bytes
    rep 			; repeat    
	movsw			; copying

; start booting
    jmp BOOTSEG:next	; CS=bootseg, IP = next
next:
	mov si, ld_msg		; Put string position into SI
	call print

;load FAT into memory @ 0x7FC0:0000
	mov ax, FATSEG
	mov es, ax		; INT13 loads to ES:DI
	mov ah, 2		; function read
	mov cx, 2		; First copy of FAT  starts at sector Nbr 1 (2-1)
	mov al, 9		; it's length is 9 sectors
	xor bx, bx		; start loading from 0 (start of segment)  
	int 13h
;load root dir into memory @ 0000:0500h = 500h (will be overwriten by kernel)
	mov si, msg_dot ; Put string position into SI
	call print
	;load root dir, INT13 loads to ES:DI
	xor ax, ax
	mov es, ax		; ES=0  
	mov ah, 2		; function read
	mov cx, 20		; ROOT dir starts at sector 19 (20-1)
	mov al, 14		; and it is 14 sectors long
	mov bx, 500h	; we load ROOT Dir at 0000:0500h  [1280 = 1024 (INT VECs) +256 (BIOS Vars)]
	int 13h 
	mov si, msg_dot ; Put string position into SI
	call print
; browse root directory for "KERNEL  BIN" our kernel.bin file
	mov cx, 224			; loop counter (Nbr of entries in root dir)
	xor ax, ax
	mov ds, ax			; DS = 0, rootdir is loaded at 0x0000:0x500
	mov si, 500h		; DS:SI points to first root dir entry
	mov ax, cs
    mov es, ax			; Let ES=0x7FE0 segment where bootsector is moved (where KernelName is)
Next_Entry:
	push cx 			; save entries counter
	mov cx, 11			; name characters counter: eleven characters by name
	mov di, KernelName	; ES:DI: points to kernel name to find 
	push si 			; save SI we use it in next instruction (comparaison of strings)
	repe cmpsb			; Repeat test for names match: compare DS:SI string to ES:DI string and decrement CX  
	je found			; file found
	pop si				; restore SI
	add si, 32			; (SI+32) = next directory entry
	pop cx				; restore entries counter
	loop Next_Entry 	; LOOP instrcution decrements CX, so if CX =/= 0  we continue searching
	jmp not_found		; CX=0 (all entries searched), file not found    
found:
	pop si				; pushed above
	; Now SI points the entry of our file in root dir, 
	; we add 26 to point first byte of file's first cluster Nbr
	mov ax, word [si + 26]	; get Nbr of first cluster (word), at offset 26 from dir entry
	; we load kernel @ 0050h:0000h 
	mov dx, 0x50
	mov es, dx
	xor bx, bx		; start of segment
	mov dx, FATSEG	; point to FAT in RAM
	mov ds, dx			
load_file:
	push ax 		; save current cluster number
	call CLUST_LBA	; convert cluster Nbr to LBA 
	mov ax, 0201h	; read 1 sector: ah=02 --> function read, al=01 --> 1 sector
	int 13h 		; load cluster (sector) into memory          
	; compute next cluster Nbr
	pop ax			; recover current cluster number
	; Get cluster's offset in FAT
	shl ax, 1		; multipy by 2  (each cluster occupies 2 bytes in FAT16)   
	mov si, ax		; save in index
	; Get Nbr of next cluster 
	mov ax, word [si]	; get word at DS:SI (DS points to adress of FAT in RAM)
	cmp ax, 0FF7h		; if > 0FF7h (end of file), so stop reading 
	ja DONE
	add bx, 512		; else point to address where to load next cluster into memory
	jmp load_file	; load next cluster
DONE:
	mov si, msg_dot   ; Put string position into SI
	call print
		
	; DS=SS=ES=CS   Tiny Model
	;mov ax, 050h       ; kernel's CRT0.s code does this job
	;mov ds, ax
	;mov es, ax
	;mov ss, ax
	jmp 0050h:0000	    ; far jump to start of kernel
	
not_found:
	mov ax, cs
	mov ds, ax
	mov si, err_msg 	; Put string position into SI
	call print
	
hang:
	hlt
	jmp hang

;---------------------------    
CLUST_LBA:  
	; In our case 1 cluster = 1 sector, so:
	; Nbr of (logical) sector =  cluster + 31  (first cluster (Nr 2) his first sector = 1+ 2x9 + 14 = 33) 
	; where: 1=bootsector,  2x9= 2Fats 9 sectors each , 14= root directory length = (224 entries X 32)/512
	; +1 ==> In VGA_SD the sector Nr starts from 1 not 0.	
	add ax, 32		; 31 + 1
	mov cx, ax
	ret

;==================================
; VGA subroutines
;================================== 

print:
	lodsb			; Get char from string into AL from DS:SI 
	or al, al		; AL= 0 ?
	je done 		; Yes, end of string so quit
	mov ah, 9
	int 10h
	jmp print		; draw next character
done:
	ret	
;----------------------------
KernelName db 'KERNEL  BIN'		; spaces needed cose 8.3 format (names in directory are upper case)
ld_msg db 'loading SOS .', 0	; SEB-SEB Operating System
msg_dot db '.', 0
err_msg db 'kernel NOT found !!',13, 0


    ; Fill free space until byte 509 with zeros (or NOPs 0x90 or HLTs 0xF4)
	TIMES 510-($-$$) DB 0x90
	DW 0xAA55				; bytes 510 and 511 = boot signature
