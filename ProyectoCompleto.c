/*
 * ProyectoCompleto.c
 *
 *  Created on: 19 nov. 2025
 *      Author: Patricio y Ana
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "MANDOLIB.h"
#include "signals.h"
#include "audios.h"


int RELOJ;


#define SLEEP SysCtlSleepFake()

extern uint32_t last_addr;
const int32_t REG_CAL[6]= {CAL_DEFAULTS};

volatile int Flag_ints=0;
int PeriodoPWM;
void IntTimer0(void);
void SysCtlSleepFake(void);

void FT800_LoadSound(const uint8_t *data, size_t soundSize, uint32_t destAddr);
void loadPlayAudio(uint32_t addr, const size_t size);
void FT800_PlaySound( const uint32_t addrs[],const size_t size[], uint8_t formats, uint8_t volume, size_t count );
void Esc_Reg8(unsigned long dir, unsigned long valor);
void Esc_Reg16(unsigned long dir, unsigned long valor);

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

    //PWM
    // Pin IR (PWM)
    // Habilitar periféricos
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    // Configurar reloj del módulo PWM
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_64);// al PWM le llega un reloj de 1.875MHz
    // Configurar pin PK5 para función M0PWM7 y tipo PWM
    GPIOPinConfigure(GPIO_PK5_M0PWM7); //Configurar el pin a PWM (ver datasheet)
    GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_5); //Configurar el pin a PWM (ver datasheet)
    // Calcular periodo para la frecuencia deseada (38000 Hz)
    const uint32_t desired_hz = 38000U;
    // PWM_clk (Hz) = reloj_sys / 64
    const uint32_t pwm_clk = RELOJ / 64U;
    // period cycles = pwm_clk / desired_hz
    uint32_t periodo_cycles = pwm_clk / desired_hz;
    if (periodo_cycles < 2)
        periodo_cycles = 2; // seguridad
    // Como tu usabas PeriodoPWM = cycles - 1, para mantenerse consistente:
    uint32_t periodo_pwm = periodo_cycles - 1U;
    // Configurar generador y periodo
    PWMGenConfigure(PWM0_BASE, PWM_GEN_3,
                    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, periodo_pwm); //frec:50Hz
    // Duty
    //    pulse = periodo_cycles * duty_fraction
    const uint32_t duty_num = 1;  // numerador para 1/3
    const uint32_t duty_den = 3;  // denominador
    uint32_t pulse = (periodo_cycles * duty_num) / duty_den;
    if (pulse == 0)
        pulse = 1;
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, pulse);
    // Enable generator pero salida inicialmente apagada
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);     //Habilita el generador 3
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false); //Habilita la salida 7 (apagada)


    //TIMER
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/20) -1);  //50ms
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer0);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);

    // Note: Keep SPI below 11MHz here
    HAL_Init_SPI(1, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();

    SysCtlDelay(RELOJ/3);
    InterfazMando();

    loadPlayAudio(last_addr,haspulsadoSize);

    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    // Loop principal
    while(1) {
        SLEEP;
        //InterfazMando();
        Lee_pantalla();
        if(POSX>=20 && POSX<=74 && POSY>=10 && POSY<=64)
        {
            UARTprintf("Has pulsado el boton de encender/apagar\n\n");
            ir_send_raw(SIGNAL_MANDO_0.data,SIGNAL_MANDO_0.length,RELOJ);
        }
        else if(POSX>=20 && POSX<=110 && POSY>=70 && POSY<=160)
            UARTprintf("Has pulsado TVE\n\n");
        else if(POSX>=140 && POSX<=230 && POSY>=70 && POSY<=160)
            UARTprintf("Has pulsado La 2\n\n");
        else if(POSX>=260 && POSX<=350 && POSY>=70 && POSY<=160)
            UARTprintf("Has pulsado Antena 3\n\n");
        else if(POSX>=20 && POSX<=110 && POSY>=170 && POSY<=260)
            UARTprintf("Has pulsado Cuatro\n\n");
        else if(POSX>=140 && POSX<=230 && POSY>=170 && POSY<=260)
            UARTprintf("Has pulsado Telecinco\n\n");
        else if(POSX>=260 && POSX<=350 && POSY>=170 && POSY<=260)
            UARTprintf("Has pulsado La Sexta\n\n");
        else if(POSX>=400 && POSX<=454 && POSY>=30 && POSY<=84)
            UARTprintf("Has pulsado el boton de subir volumen\n\n");
        else if(POSX>=400 && POSX<=454 && POSY>=110 && POSY<=164)
            UARTprintf("Has pulsado el boton de bajar volumen\n\n");
        else if(POSX>=400 && POSX<=454 && POSY>=190 && POSY<=244)
            UARTprintf("Has pulsado el boton de quitar volumen\n\n");
    }

}

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

void IntTimer0(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}

void FT800_LoadSound(const uint8_t *data, size_t soundSize, uint32_t destAddr)
{
    ComEsperaFin();
    HAL_SPI_CSLow();

    FT800_SPI_SendAddressWR(destAddr);

    // Procesar en bloques de 2 bytes porque es RGB565
    size_t i;
    for (i = 0; i < soundSize; i++)
//        HAL_SPI_ReadWrite(data[i]);
        FT800_SPI_Write8(data[i]);

    HAL_SPI_CSHigh();
}


// Funcion para mostrar imagenes
void FT800_PlaySound( const uint32_t addrs[],const size_t size[], uint8_t formats, uint8_t volume, size_t count )
{
    size_t k;
    ComEsperaFin();
    for (k = 0; k < count; ++k) {
        Esc_Reg(REG_PLAYBACK_START,addrs[k]);
        Esc_Reg(REG_PLAYBACK_LENGTH,size[k]);
        Esc_Reg16(REG_PLAYBACK_FREQ,48000);
        Esc_Reg8(REG_PLAYBACK_FORMAT,formats);
        Esc_Reg8(REG_VOL_PB,volume);
        Esc_Reg8(REG_PLAYBACK_LOOP,0);
        Esc_Reg8(REG_PLAYBACK_PLAY,1);
    }

    ComEsperaFin();
}


//Funcion que muestra la intefarza del mando
void loadPlayAudio(uint32_t addr, const size_t size)
{
    //Direcciones de memoria donde se empieza a grabar cada imagen
    uint32_t addr1 = addr;


    FT800_LoadSound(haspulsado_data, haspulsadoSize, addr1); //Carga logo tve

    // Arrays de posiciones y tamaños
    uint32_t addrs[] = {addr1 };
    size_t sizeAudio[] =  {haspulsadoSize} ;

    FT800_PlaySound(addrs,sizeAudio,1,255,1);

}

void Esc_Reg8(unsigned long dir, unsigned long valor)
{
    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(dir);
    FT800_SPI_Write8(valor);
    HAL_SPI_CSHigh();
}

void Esc_Reg16(unsigned long dir, unsigned long valor)
{
    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(dir);
    FT800_SPI_Write16(valor);
    HAL_SPI_CSHigh();
}
