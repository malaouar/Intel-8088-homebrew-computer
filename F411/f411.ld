
MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS {
	.text :{
		*(.vectors*)
		*(.text)
		*(.rodata*)
	} >FLASH
    
    .data :{
        _sdata = .;
        *(.data*)
		 *(.ramfunc*)
        _edata = .;
    } > RAM AT> FLASH
    
    
    .bss : {
        _sbss = .;
        *(.bss*)
        _ebss = .;
    } > RAM
    
    _sidata = LOADADDR(.data);
}
