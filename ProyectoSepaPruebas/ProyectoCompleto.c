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




int RELOJ;


#define SLEEP SysCtlSleepFake()


const int32_t REG_CAL[6]= {CAL_DEFAULTS};

volatile int Flag_ints=0;
int PeriodoPWM;
void IntTimer0(void);
void SysCtlSleepFake(void);

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


    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    // Loop principal
    while(1) {
        SLEEP;
        //InterfazMando();
        Lee_pantalla();
        if (POSX >= 20 && POSX <= 74 && POSY >= 10 && POSY <= 64)
        {
            UARTprintf("Has pulsado el boton de encender/apagar\n\n");
            ir_send_raw(SIGNAL_MANDO_POWER.data, SIGNAL_MANDO_POWER.length, RELOJ);
        }
        else if (POSX >= 20 && POSX <= 110 && POSY >= 70 && POSY <= 160)
        {
            UARTprintf("Has pulsado TVE\n\n");
            ir_send_raw(SIGNAL_MANDO_LA1.data, SIGNAL_MANDO_LA1.length, RELOJ);
        }

        else if (POSX >= 140 && POSX <= 230 && POSY >= 70 && POSY <= 160)
        {
            UARTprintf("Has pulsado La 2\n\n");
            ir_send_raw(SIGNAL_MANDO_LA2.data, SIGNAL_MANDO_LA2.length, RELOJ);
        }
        else if (POSX >= 260 && POSX <= 350 && POSY >= 70 && POSY <= 160)
        {
            UARTprintf("Has pulsado Antena 3\n\n");
            ir_send_raw(SIGNAL_MANDO_ANTENA3.data, SIGNAL_MANDO_ANTENA3.length, RELOJ);
        }
        else if (POSX >= 20 && POSX <= 110 && POSY >= 170 && POSY <= 260)
        {
            UARTprintf("Has pulsado Cuatro\n\n");
            ir_send_raw(SIGNAL_MANDO_LA4.data, SIGNAL_MANDO_LA4.length, RELOJ);
        }
        else if (POSX >= 140 && POSX <= 230 && POSY >= 170 && POSY <= 260)
        {
            UARTprintf("Has pulsado Telecinco\n\n");
            ir_send_raw(SIGNAL_MANDO_TELECINCO.data, SIGNAL_MANDO_TELECINCO.length, RELOJ);
        }
        else if (POSX >= 260 && POSX <= 350 && POSY >= 170 && POSY <= 260)
        {
            UARTprintf("Has pulsado La Sexta\n\n");
            ir_send_raw(SIGNAL_MANDO_LASEXTA.data, SIGNAL_MANDO_LASEXTA.length, RELOJ);
        }
        else if (POSX >= 400 && POSX <= 454 && POSY >= 30 && POSY <= 84)
        {
            UARTprintf("Has pulsado el boton de subir volumen\n\n");
            ir_send_raw(SIGNAL_MANDO_VOLUP.data, SIGNAL_MANDO_VOLUP.length, RELOJ);
        }
        else if (POSX >= 400 && POSX <= 454 && POSY >= 110 && POSY <= 164)
        {
            UARTprintf("Has pulsado el boton de bajar volumen\n\n");
            ir_send_raw(SIGNAL_MANDO_VOLDOWN.data, SIGNAL_MANDO_VOLDOWN.length,
                        RELOJ);
        }
        else if (POSX >= 400 && POSX <= 454 && POSY >= 190 && POSY <= 244)
        {
            UARTprintf("Has pulsado el boton de quitar volumen\n\n");
            ir_send_raw(SIGNAL_MANDO_MUTE.data, SIGNAL_MANDO_MUTE.length,RELOJ);}
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
