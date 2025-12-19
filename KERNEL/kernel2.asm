; testing write sector function 
; write buffer to sector Nr 11 on SD

use16		; we are in 16 bit real mode

; At this point we have CS = 0x0050
main:
    mov  ax, cs
    mov  ds, ax
	mov  es, ax
	
	;cls
	xor ah, ah
	int 10h
	
	sti		; enable ints (if keyboard needed)
	
	mov si, st_msg	; starting message offset in SI
	call print
	mov si, wr_msg	; 
	call print

	; we have DS = CS current segment
	mov bx, sector	; BX points to the beginning of our buffer (offset in DS)
	mov cx, 12		; CX = sector Nr + 1 
	mov al, 1       ; Nr of sectors to write
	mov ah, 3		; function write sector
	int 13h 
	
	
hang:
	hlt
	jmp hang


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
wr_msg db 'Writing sector ..', 13, 10, 0

sector db 'S', 'E', 'C', 'T', 'O', 'R', '1', '1', 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '2', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '4', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '5', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '6', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '7', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D, 'L', '8', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,  'L', '1', '2', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '4', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '5', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '6', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '7', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '8', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '1', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '2', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '4', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '5', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '6', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '7', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '8', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '2', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '3', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '3', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D,'L', '3', '2', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x0D


