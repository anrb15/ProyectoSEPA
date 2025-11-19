/*
 * main.c
 */



#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include <stdio.h>
#include "signals.h"


// =======================================================================
// Function Declarations
// =======================================================================
#define byte char

// Para cambiar pin IR (elegido: PN0)
#define IR_PORT GPIO_PORTN_BASE
#define IR_PIN  GPIO_PIN_0

#define IR_ON()     GPIOPinWrite(IR_PORT, IR_PIN, IR_PIN)
#define IR_OFF()    GPIOPinWrite(IR_PORT, IR_PIN, 0)

// Para botones
#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

volatile int Flag_ints=0;

// Funciones
static inline void delay_us(uint32_t us);
void ir_send_raw(const uint16_t *signal, size_t len);
void SysCtlSleepFake(void);
void IntTimer(void);

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

int RELOJ;
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

// Tiempo de espera
int t_esp = 0;
int i = 0;

int main(void)
 {

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // Botones
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Apagar todos los LEDs al inicio
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, 0);
    // Led 1: N1 | Led 2: N0 | Led 3: F4 | Led 4: F0

    // Pin IR
    GPIOPinTypeGPIOOutput(IR_PORT, IR_PIN);
    IR_OFF();

    // Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0

    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1); // FREC 20 = 50ms
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);

    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);

    SysCtlPeripheralClockGating(true);


    while(1){
        SLEEP;

        // Si se pulsa B1, emitir señal
        if (B1_ON)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4); // LED indicador

            ir_send_raw(SIGNAL_MANDO_0.data, SIGNAL_MANDO_0.length);

            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
        }

    }
}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Delay en microsegundos (SysCtlDelay tarda 3 ciclos por iteración)
static inline void delay_us(uint32_t us)
{
    SysCtlDelay((RELOJ / 3000000) * us);
}

// Enviar señal (raw: señal medida en duraciones MARK/SPACE directamente)
void ir_send_raw(const uint16_t *signal, size_t len)
{
    bool mark = true;  // El array empieza por MARK

    for (i = 0; i < len; i++)
    {
        if (mark) {
            IR_ON();       // nivel alto
        } else {
            IR_OFF();      // nivel bajo
        }

        delay_us(signal[i]);  // mantener MARK o SPACE

        mark = !mark;  // alternar
    }

    IR_OFF();  // asegurarse de apagar al terminar
}

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}
