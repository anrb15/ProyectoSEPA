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


#define RAM_G_SIZE  0x040000

//LOGO TVE
#define RAM_G_ADDR1 0x000000

#define IMG_WIDTH  31       // Ancho de la imagen
#define IMG_HEIGHT 29       // Alto de la imagen
#define IMG_STRIDE (IMG_WIDTH*2) // Para RGB565, stride = ancho * 2 bytes por pixel
#define IMG_SIZE   1798     // Tamaï¿½o del archivo .binh (en bytes)
#define IMG_FORMAT 7         // Formato RGB565

//LOGO ANTENA 3
#define IMG2_WIDTH   100
#define IMG2_HEIGHT  100
#define IMG2_STRIDE  (IMG2_WIDTH*2)
#define IMG2_SIZE    20000   // tamaño en bytes de antena3_rgb565 (ejemplo)
#define IMG2_FORMAT  7

void FT800_LoadAndInflateImage(const uint8_t *data, size_t imgSize, uint32_t destAddr);
void FT800_ShowBitmapSimple(const uint32_t addrs[], const uint16_t widths[], const uint16_t heights[],
                            const uint16_t strides[], const uint8_t formats[], const uint16_t xs[],
                            const uint16_t ys[], size_t count);
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

    uint32_t addr1 = RAM_G_ADDR1;
    uint32_t addr2 = addr1 + ((tve_rgb565_size + 3) & ~3U);
    // Verificar que ambas imágenes caben en RAM_G
    if ((addr2 + antena3_rgb565_size) > (RAM_G_ADDR1 + RAM_G_SIZE)) {
        UARTprintf("ERROR: No hay espacio en RAM_G para la segunda imagen. Ajusta direcciones/tamaï¿½os.\n");
        while (1) { SysCtlDelay(RELOJ); }
    }
    FT800_LoadAndInflateImage(tve_rgb565_data, tve_rgb565_size, addr1); //Carga imagen
    FT800_LoadAndInflateImage(antena3_rgb565_data, antena3_rgb565_size, addr2); //Carga imagen

    FT800_ShowBitmapSimple(
            (uint32_t[]){ addr1, addr2 },
            (uint16_t[]){ IMG_WIDTH, IMG2_WIDTH },
            (uint16_t[]){ IMG_HEIGHT, IMG2_HEIGHT },
            (uint16_t[]){ IMG_STRIDE, IMG2_STRIDE },
            (uint8_t[]){ IMG_FORMAT, IMG2_FORMAT },
            (uint16_t[]){ 0, 50 },   // x en pixeles de cada imagen
            (uint16_t[]){ 0, 0 },    // y en pixeles
            2
        ); //Muestra Imagen


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
void FT800_ShowBitmapSimple(const uint32_t addrs[], const uint16_t widths[], const uint16_t heights[],
                            const uint16_t strides[], const uint8_t formats[], const uint16_t xs[],
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
        EscribeRam32(CMD_BITMAP_LAYOUT(formats[k], strides[k], heights[k]));
        EscribeRam32(CMD_BITMAP_SIZE(NEAREST, BORDER, BORDER, widths[k], heights[k]));
        EscribeRam32(CMD_VERTEX2F(xs[k]*16, ys[k]*16)); // vertex recibe x*16,y*16
    }

    EscribeRam32(CMD_END);
    //Dibuja();  // el profesor ha dicho que no es necesario
    // Mostrar la display list y solicitar swap
    EscribeRam32(CMD_DISPLAY);
    EscribeRam32(CMD_SWAP);
    Ejecuta_Lista();
    ComEsperaFin();
}





