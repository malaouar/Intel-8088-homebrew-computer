
#ifndef	__GDI_H
#define	__GDI_H

#include <stdint.h>

//	Types
typedef	unsigned int	u32;
typedef	int				i32;
typedef	unsigned short	u16;
typedef	short			i16;
typedef	unsigned char	u8;
typedef	char			i8;

typedef	u32				*pu32;
typedef	i32				*pi32;
typedef u16				*pu16;
typedef i16				*pi16;
typedef	u8				*pu8;
typedef i8				*pi8;

#define	NULL			0x0

//	System font
#define	FONT_WIDTH		6		// Width in pixels
#define	FONT_HEIGHT		8		// Height in pixels
#define	FONT_OFFSET		0x20

extern u8 curflag; 			// cursor flag, if 0 don't draw cursor
extern  u16 con_x, con_y;  	// console coordinates

#define	VID_HSIZE		80		// Horizontal resolution (in bytes)= 640/8
#define	VID_VSIZE		480		// Vertical resolution (in lines)
#define VTOTAL	VID_HSIZE		// Total bytes to send through SPI for one horizontal line

#define	VID_PIXELS_X	(VID_HSIZE * 8)
#define	VID_PIXELS_Y	VID_VSIZE

extern uint8_t fb[VID_VSIZE][VID_HSIZE];
extern uint8_t mode;

// Typedef for interrupt handler function pointer
typedef void (*InterruptHandler)(void);

// Volatile global pointer to store the current interrupt handler
extern volatile InterruptHandler currentInterruptHandler;

//	Function definitions
void	vid_init(void);
void 	cls(void);
void	bitBlt(i16 x, i16 y, i16 w, i16 h, pu8 bm);
void 	draw_char(u8 c);
void 	print_str(char *str);

#endif
