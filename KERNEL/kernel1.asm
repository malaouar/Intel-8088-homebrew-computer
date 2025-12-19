; A program to read a key from the keyboard using INT16 and display it 
; This program is loaded as kernel.bin

use16		; we are in 16 bit real mode

; At this point we have CS = 0x0050
main:
    mov  ax, cs
    mov  ds, ax
	mov  es, ax
	
	sti		; enable ints (for keyboard)
	
	mov si, st_msg	; 
	call print

	mov si, ld_msg	; 
	call print
	
;=========================
; get a char from keyboard	
getc:
	int 16h			; call BIOS 
	cmp al, 0x1F	; ASCII?
	jg putc			; Yes print it
	jmp getc		; else try again
putc:
	mov	 ah, 9
	int 10h
	jmp getc

;--------------------------------	
hang:
	hlt
	jmp hang

;====================
delay:
	mov cx, 0xFFFF
del1:
    loop del1
del2:
    loop del2
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
st_msg db 'SOS starting ..', 13, 10, 0
ld_msg db 'Type characters...', 13, 10, 0	
