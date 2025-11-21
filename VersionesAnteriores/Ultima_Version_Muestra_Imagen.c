#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "tve.h"


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


#define RAM_G_ADDR 0x000000  // Direcciï¿½n en RAM_G donde se cargarï¿½ la imagen
#define IMG_WIDTH  31       // Ancho de la imagen
#define IMG_HEIGHT 29       // Alto de la imagen
#define IMG_STRIDE (IMG_WIDTH*2) // Para RGB565, stride = ancho * 2 bytes por pixel
#define IMG_SIZE   1798     // Tamaï¿½o del archivo .binh (en bytes)
#define IMG_FORMAT 7         // Formato RGB565

void FT800_LoadAndInflateImage(const uint8_t *data, size_t imgSize, uint32_t destAddr);
void FT800_ShowBitmapSimple(uint32_t addr, uint16_t width, uint16_t height, uint16_t stride, uint8_t format, uint16_t i, uint16_t j);
int RELOJ;



int main(void)
{


    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    // Initialize the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);

    // Note: Keep SPI below 11MHz here
    HAL_Init_SPI(1, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();

    SysCtlDelay(RELOJ/3);

    uint8_t id = Lee_Reg8(REG_ID); // usa tu función de lectura de registros
    char s[40];
    sprintf(s, "FT800 REG_ID = 0x%02X\n", id);
    UARTprintf(s);

    FT800_LoadAndInflateImage(tve_rgb565_data, tve_rgb565_size, RAM_G_ADDR);

    // DIAGNÓSTICO: volcado de primeros 64 bytes desde RAM_G
    UARTprintf("\n=== Primeros 64 bytes en RAM_G ===\n");
    HAL_SPI_CSLow();
    FT800_SPI_SendAddressRD(RAM_G_ADDR);  // Leer desde RAM_G

    int k;
    for (k = 0; k < 64; ++k) {
        uint8_t b = HAL_SPI_ReadWrite(0x00);
        char hex[4];
        sprintf(hex, "%02X ", b);
        UARTprintf(hex);
        if ((k & 15) == 15) UARTprintf("\n");
    }
    HAL_SPI_CSHigh();
    UARTprintf("\n=== Fin volcado ===\n");

    // Mostrar la imagen
    FT800_ShowBitmapSimple(RAM_G_ADDR, IMG_WIDTH, IMG_HEIGHT, IMG_STRIDE, IMG_FORMAT,0,0);



    //UARTprintf("Imagen mostrada correctamente.\n");

    // Loop principal
    while(1) {
        // Aquï¿½ puedes agregar mï¿½s lï¿½gica
    }

}

void FT800_LoadAndInflateImage(const uint8_t *data, size_t imgSize, uint32_t destAddr)
{
    // Mandamos el comando CMD_INFLATE primero
    // uint32_t cmd = CMD_INFLATE;
    //HAL_SPI_ReadWrite((cmd >> 0) & 0xFF);
    // HAL_SPI_ReadWrite((cmd >> 8) & 0xFF);
    // HAL_SPI_ReadWrite((cmd >> 16) & 0xFF);
    // HAL_SPI_ReadWrite((cmd >> 24) & 0xFF);

    UARTprintf("DEBUG: Copiando datos RGB565 a RAM_G...\n");

    // PASO 1: Espera a que FIFO esté listo
    UARTprintf("DEBUG: Esperando FIFO...\n");
    ComEsperaFin();
    UARTprintf("DEBUG: FIFO listo\n");

    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(destAddr);

        size_t i;
        for (i = 0; i < imgSize; i++) {
            HAL_SPI_ReadWrite(data[i]);
            if (i % 256 == 0) {
                char msg[40];
                sprintf(msg, "DEBUG: Bytes copiados: %u\n", (unsigned)i);
                UARTprintf(msg);
            }
        }

        HAL_SPI_CSHigh();

        char msg[80];
        sprintf(msg, "DEBUG: %u bytes copiados a RAM_G\n", (unsigned)imgSize);
        UARTprintf(msg);
}

// =====================================================
// Funciï¿½n: Mostrar bitmap en pantalla
// =====================================================
void FT800_ShowBitmapSimple(uint32_t addr, uint16_t width, uint16_t height, uint16_t stride, uint8_t format, uint16_t i, uint16_t j)
{
    ComEsperaFin();
    EscribeRam32(CMD_DLSTART);
    EscribeRam32(CMD_CLEAR_COLOR_RGB(0,0,0));
    EscribeRam32(CMD_CLEAR(1,1,1));

    // Configurar bitmap
    EscribeRam32(CMD_BEGIN(BITMAPS));
    EscribeRam32(CMD_BITMAP_SOURCE(addr));
    EscribeRam32(CMD_BITMAP_LAYOUT(format, stride, height));
    EscribeRam32(CMD_BITMAP_SIZE(NEAREST, BORDER, BORDER, width, height));

    // Dibujar en la esquina superior izquierda
    EscribeRam32(CMD_VERTEX2F(i*16, j*16));

    EscribeRam32(CMD_END);
    //Dibuja();  // el profesor ha dicho que no es necesario
    // Mostrar la display list y solicitar swap
    EscribeRam32(CMD_DISPLAY);
    EscribeRam32(CMD_SWAP);
    Ejecuta_Lista();
    ComEsperaFin();
}





