/*
 * MANDOLIB.h
 *
 *  Created on: 19 nov. 2025
 *      Author: Patricio y Ana
 */

#ifndef MANDOLIB_H_
#define MANDOLIB_H_

#include <stdlib.h>
#include "signals.h"

#define CMD_CLEAR_COLOR_RGB(r,g,b) ((2UL << 24) | ((r) << 16) | ((g) << 8) | (b))
#define CMD_CLEAR(c,s,t)   ((38UL << 24) | ((c) << 2) | ((s) << 1) | (t))
#define CMD_BEGIN(prim)    ((31UL << 24) | (prim))
#define CMD_VERTEX2F(x,y)  ((1UL << 30) | ((x) << 15) | (y))
#define CMD_BITMAP_SOURCE(a) ((1UL << 24) | ((a) & 0x3FFFFF))
#define CMD_BITMAP_LAYOUT(format, stride, height) ((7UL << 24) | ((format) << 19) | ((stride) << 9) | (height))
#define CMD_BITMAP_SIZE(filter, wrapx, wrapy, width, height) ((8UL << 24) | ((filter) << 20) | ((wrapx) << 19) | ((wrapy) << 18) | ((width) << 9) | (height))

// ===== Constantes auxiliares =====
#define BITMAPS    1  // Primitiva para BEGIN()
#define NEAREST    0
#define BORDER     0


#define RAM_G_SIZE  0x040000 //Tamaño memoria RAM


#define RAM_G_ADDR1 0x000000 //Direccion en la que empieza la primera foto

//LOGOS
#define IMG_WIDTH  90       // Ancho de la imagen
#define IMG_HEIGHT 90       // Alto de la imagen
#define IMG_STRIDE (IMG_WIDTH*2) // Para RGB565, stride = ancho * 2 bytes por pixel
#define IMG_SIZE   16200     // Tamaï¿½o del archivo .binh (en bytes)
#define IMG_FORMAT 7         // Formato RGB565

//BOTONES
#define IMG_WIDTH2  54       // Ancho de la imagen
#define IMG_HEIGHT2 54       // Alto de la imagen
#define IMG_STRIDE2 (IMG_WIDTH2*2) // Para RGB565, stride = ancho * 2 bytes por pixel
#define IMG_SIZE2   5832     // Tamaï¿½o del archivo .binh (en bytes)


void FT800_LoadImage(const uint8_t *data, size_t imgSize, uint32_t destAddr);
void FT800_ShowBitmapSimple( const uint32_t addrs[],const uint16_t widths[],const uint16_t heights[],
                             const uint16_t strides[], uint8_t formats, const uint16_t xs[],
                            const uint16_t ys[], size_t count);
void InterfazMando(void);

void FT800_LoadSound(const uint8_t *data,const size_t soundSize,const uint32_t destAddr);
void loadPlayAudio(uint32_t addr, const uint8_t *audio1, const uint8_t *audio2,const size_t tam[], uint8_t vol);
void FT800_PlaySound( const uint32_t addrs[],const size_t size[], uint8_t formats, uint8_t volume);
void Esc_Reg8(unsigned long dir, unsigned long valor);
void Esc_Reg16(unsigned long dir, unsigned long valor);

static inline void delay_us(uint32_t us, int RELOJ);
void ir_send_raw(const uint16_t *signal, size_t len, int RELOJ);

static inline void IR_MARK(void);
static inline void IR_SPACE(void);


#endif /* MANDOLIB_H_ */
