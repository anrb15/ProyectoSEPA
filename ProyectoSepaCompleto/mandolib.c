/*
 * mandolib.c
 *
 *  Created on: 19 nov. 2025
 *      Author: Patricio y Ana
 */

#include <stdbool.h>
#include <stdint.h>
//#include "inc/hw_memmap.h"
//#include "driverlib/gpio.h"
//#include "driverlib/pin_map.h"
//#include "driverlib/ssi.h"
//#include "driverlib/sysctl.h"
//#include "driverlib/uart.h"
#include "utils/uartstdio.h"
//
//
//
//#include "inc/hw_ints.h"
//#include "driverlib/debug.h"
//#include "driverlib/interrupt.h"
//#include "driverlib/pin_map.h"
//#include "driverlib/rom.h"
//#include "driverlib/rom_map.h"
#include "FT800_TIVA.h"
#include "MANDOLIB.h"
#include <logos.h>
#include <stdio.h>
#include <stdlib.h>


// Funcion para cargar imagenes en la memoria
void FT800_LoadAndInflateImage(const uint8_t *data, size_t imgSize, uint32_t destAddr)
{
    ComEsperaFin();
    HAL_SPI_CSLow();

    FT800_SPI_SendAddressWR(destAddr);
    size_t i;
    for (i = 0; i < imgSize; i++)
        HAL_SPI_ReadWrite(data[i]);

    HAL_SPI_CSHigh();
}

// Funcion para mostrar imagenes
void FT800_ShowBitmapSimple( const uint32_t addrs[],const uint16_t widths[],const uint16_t heights[],
                             const uint16_t strides[], uint8_t formats, const uint16_t xs[],
                             const uint16_t ys[], size_t count)
{
    size_t k;
    ComEsperaFin();
    EscribeRam32(CMD_DLSTART);
    EscribeRam32(CMD_CLEAR_COLOR_RGB(0,0,0));
    EscribeRam32(CMD_CLEAR(1,1,1));

    // Configurar bitmap
    EscribeRam32(CMD_BEGIN(BITMAPS));
    for (k = 0; k < count; ++k) {
        EscribeRam32(CMD_BITMAP_SOURCE(addrs[k]));
        if(k<6)
        {
            EscribeRam32(CMD_BITMAP_LAYOUT(formats, strides[0], heights[0]));
            EscribeRam32(CMD_BITMAP_SIZE(NEAREST, BORDER, BORDER, widths[0], heights[0]));
        }
        else
        {
            EscribeRam32(CMD_BITMAP_LAYOUT(formats, strides[1], heights[1]));
            EscribeRam32(CMD_BITMAP_SIZE(NEAREST, BORDER, BORDER, widths[1], heights[1]));
        }
        EscribeRam32(CMD_VERTEX2F(xs[k]*16, ys[k]*16));
    }

    EscribeRam32(CMD_END);
    EscribeRam32(CMD_DISPLAY);
    EscribeRam32(CMD_SWAP);
    Ejecuta_Lista();
    ComEsperaFin();
}


//Funcion que muestra la intefarza del mando
void InterfazMando(void)
{
    //Direcciones de memoria donde se empieza a grabar cada imagen
    uint32_t addr1 = RAM_G_ADDR1;
    uint32_t addr2 = addr1 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr3 = addr2 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr4 = addr3 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr5 = addr4 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr6 = addr5 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr7 = addr6 + ((IMG_SIZE + 3) & ~3U);
    uint32_t addr8 = addr7 + ((IMG_SIZE2 + 3) & ~3U);
    uint32_t addr9 = addr8 + ((IMG_SIZE2 + 3) & ~3U);
    uint32_t addr10 = addr9 + ((IMG_SIZE2 + 3) & ~3U);

    FT800_LoadAndInflateImage(tve_rgb565_data, IMG_SIZE, addr1); //Carga logo tve
    FT800_LoadAndInflateImage(la2_rgb565_data, IMG_SIZE, addr2); //Carga logo la2
    FT800_LoadAndInflateImage(antena3_rgb565_data, IMG_SIZE, addr3); //Carga logo antena3
    FT800_LoadAndInflateImage(cuatro_rgb565_data, IMG_SIZE, addr4); //Carga logo cuatro
    FT800_LoadAndInflateImage(telecinco_rgb565_data, IMG_SIZE, addr5); //Carga logo telecinco
    FT800_LoadAndInflateImage(lasexta_rgb565_data, IMG_SIZE, addr6); //Carga logo lasexta
    FT800_LoadAndInflateImage(encender_rgb565_data, IMG_SIZE2, addr7); //Carga imagen boton encender/apagar
    FT800_LoadAndInflateImage(subir_rgb565_data, IMG_SIZE2, addr8); //Carga imagen boton subir volumen
    FT800_LoadAndInflateImage(bajar_rgb565_data, IMG_SIZE2, addr9); //Carga imagen boton bajar volumen
    FT800_LoadAndInflateImage(quitar_rgb565_data, IMG_SIZE2, addr10); //Carga imagen quitar el volumen

    //Muestra todas las imagenes
    FT800_ShowBitmapSimple(
            (uint32_t[]){ addr1, addr2, addr3, addr4, addr5,addr6, addr7, addr8, addr9, addr10 },
            (uint16_t[]){IMG_WIDTH,IMG_WIDTH2},
            (uint16_t[]){IMG_HEIGHT,IMG_HEIGHT2},
            (uint16_t[]){IMG_STRIDE,IMG_STRIDE2},
            IMG_FORMAT,
            (uint16_t[]){ 20, 140, 260, 20, 140, 260, 20, 400, 400, 400 },   // x en pixeles de cada imagen
            (uint16_t[]){ 70, 70,  70,  170,170, 170, 10, 30,  110, 190},    // y en pixeles
            10
    );
}
