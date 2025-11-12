#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"



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


#define RAM_G_ADDR 0x000000  // Dirección en RAM_G donde se cargará la imagen
#define IMG_WIDTH  31       // Ancho de la imagen
#define IMG_HEIGHT 29       // Alto de la imagen
#define IMG_STRIDE (IMG_WIDTH*2) // Para RGB565, stride = ancho * 2 bytes por pixel
#define IMG_SIZE   1798     // Tamaño del archivo .binh (en bytes)
#define IMG_FORMAT 7         // Formato RGB565

void FT800_LoadAndInflateImage(const char *binhFile, uint32_t destAddr, size_t imgSize);
void FT800_ShowBitmapSimple(uint32_t addr, uint16_t width, uint16_t height, uint16_t stride, uint8_t format);
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
       // Cargar y decodificar imagen comprimida
       FT800_LoadAndInflateImage("tve.binh", RAM_G_ADDR, (unsigned long)(IMG_WIDTH*IMG_HEIGHT*2));

       // Mostrar la imagen
       FT800_ShowBitmapSimple(RAM_G_ADDR, IMG_WIDTH, IMG_HEIGHT, IMG_STRIDE, IMG_FORMAT);

       UARTprintf("Imagen mostrada correctamente.\n");
    // Loop principal
    while(1) {
        // Aquí puedes agregar más lógica
    }

}

void FT800_LoadAndInflateImage(const char *binhFile, uint32_t destAddr, size_t imgSize)
{
    char filebin[50];

    FILE *fp = fopen(binhFile, "rb");
    if (!fp)
    {
        sprintf(filebin,"Error: no se pudo abrir el archivo %s\n",binhFile);
        UARTprintf(filebin);
        return;
    }

    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(destAddr);

    // Mandamos el comando CMD_INFLATE primero
    HAL_SPI_ReadWrite(CMD_INFLATE);

    uint8_t buffer[512];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        size_t i;
        for ( i = 0; i < bytesRead; i++)
            HAL_SPI_ReadWrite(buffer[i]);
    }

    HAL_SPI_CSHigh();
    fclose(fp);
    /*sprintf(addrDest,"Imagen comprimida cargada y decodificada en RAM @0x%08X\n", destAddr);
    UARTprintf(char(addrDest));*/
}

// =====================================================
// Función: Mostrar bitmap en pantalla
// =====================================================
void FT800_ShowBitmapSimple(uint32_t addr, uint16_t width, uint16_t height, uint16_t stride, uint8_t format)
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
    EscribeRam32(CMD_VERTEX2F(0*16,0*16));
    EscribeRam32(CMD_VERTEX2F(width*16, height*16));

    EscribeRam32(CMD_END);
    Dibuja();
}





