/*
 * mandolib.c
 *
 *  Created on: 19 nov. 2025
 *      Author: Patricio y Ana
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "driverlib2.h"
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h"

#include "FT800_TIVA.h"
#include <logos.h>
#include "signals.h"
#include "MANDOLIB.h"

uint32_t last_addr;

void FT800_LoadImage(const uint8_t *data, size_t imgSize, uint32_t destAddr)
{
    ComEsperaFin();
    HAL_SPI_CSLow();

    FT800_SPI_SendAddressWR(destAddr);

    // Procesar en bloques de 2 bytes porque es RGB565
    size_t i;
    for (i = 0; i < imgSize; i += 2)
    {
        // Leer el pixel como 16 bits
        uint16_t pixel = data[i] | (data[i + 1] << 8);

        // Convertir RGB565 -> BGR565
        uint16_t converted =
            ((pixel & 0xF800) >> 11) |     // R -> B
            ( pixel & 0x07E0 )        |    // G igual
            ((pixel & 0x001F) << 11);     // B -> R

        // Enviar pixel convertido, LSB primero (formato del FT800)
        HAL_SPI_ReadWrite(converted & 0xFF);
        HAL_SPI_ReadWrite(converted >> 8);
    }

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

//
////Funcion que muestra la intefarza del mando
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

    last_addr=addr10 + ((IMG_SIZE2 + 3) & ~3U);

    FT800_LoadImage(tve_rgb565_data, IMG_SIZE, addr1); //Carga logo tve
    FT800_LoadImage(la2_rgb565_data, IMG_SIZE, addr2); //Carga logo la2
    FT800_LoadImage(antena3_rgb565_data, IMG_SIZE, addr3); //Carga logo antena3
    FT800_LoadImage(cuatro_rgb565_data, IMG_SIZE, addr4); //Carga logo cuatro
    FT800_LoadImage(telecinco_rgb565_data, IMG_SIZE, addr5); //Carga logo telecinco
    FT800_LoadImage(lasexta_rgb565_data, IMG_SIZE, addr6); //Carga logo lasexta
    FT800_LoadImage(encender_rgb565_data, IMG_SIZE2, addr7); //Carga imagen boton encender/apagar
    FT800_LoadImage(subir_rgb565_data, IMG_SIZE2, addr8); //Carga imagen boton subir volumen
    FT800_LoadImage(bajar_rgb565_data, IMG_SIZE2, addr9); //Carga imagen boton bajar volumen
    FT800_LoadImage(quitar_rgb565_data, IMG_SIZE2, addr10); //Carga imagen quitar el volumen

    // Arrays de posiciones y tamaños
    uint32_t addrs[] = { addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10 };
    uint16_t widths[] = { IMG_WIDTH, IMG_WIDTH2 };
    uint16_t heights[] = { IMG_HEIGHT, IMG_HEIGHT2 };
    uint16_t strides[] = { IMG_STRIDE, IMG_STRIDE2 };
    uint16_t xs[] = { 20, 140, 260, 20, 140, 260, 20, 400, 400, 400 };
    uint16_t ys[] = { 70, 70,  70, 170,170, 170, 10, 30, 110, 190 };

    // Muestra todas las imagenes
    FT800_ShowBitmapSimple(addrs, widths, heights, strides, IMG_FORMAT, xs, ys, 10);

}



// Encender/apagar la salida PWM (marca = portadora ON)
static inline void IR_MARK(void)  { PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);  }
static inline void IR_SPACE(void) { PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false); }

// Delay en microsegundos (SysCtlDelay tarda 3 ciclos por iteración)
static inline void delay_us(uint32_t us, int RELOJ)
{
    // Timer a 120 MHz -> 120 ticks = 1 microsegundo
    uint32_t ticks = (RELOJ / 1000000) * us;

    TimerLoadSet(TIMER2_BASE, TIMER_A, ticks - 1);

    // Limpia bandera por si acaso
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

    // Arranca el temporizador
    TimerEnable(TIMER2_BASE, TIMER_A);

    // Espera activa a que finalice
    while (!(TimerIntStatus(TIMER2_BASE, false) & TIMER_TIMA_TIMEOUT))
        ;

    // Limpia bandera de finalizaci�n
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
}

// Enviar señal (raw: señal medida en duraciones MARK/SPACE directamente)
void ir_send_raw(const uint16_t *signal, size_t len, int RELOJ)
{
    bool mark = true;  // El array empieza por MARK
    int i;
    for (i = 0; i < len; i++)
    {
        if (mark) {
            IR_MARK();       // PWM ON (portadora)
        } else {
            IR_SPACE();      // PWM OFF (silencio)
        }

        delay_us(signal[i],RELOJ);  // mantener MARK o SPACE

        mark = !mark;  // alternar
    }

    IR_SPACE();  // asegurarse de apagar al terminar
}
