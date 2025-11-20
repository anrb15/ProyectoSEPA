/*
 * main.c
 */

/*
 * TODO: Cambiar de delay_us() con SysCtlDelay a con Timer Hardware
 * TODO: Conectar Led IR correctamente (transistor):
 *      - GND + Led- a E
 *      - PWM0 + resistor a B
 *      - Resistor Rled + Led+ + Vcc 3.3V
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include <stdio.h>
#include "signals.h"


// =======================================================================
// Function Declarations
// =======================================================================

// Para botones
#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

// Encender/apagar la salida PWM (marca = portadora ON)
static inline void IR_MARK(void)  { PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);  }
static inline void IR_SPACE(void) { PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false); }

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
int pulsado=0;

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

    static int debounce = 0;
    static bool pulsado = false;
    static bool led = false;

    while(1){
        SLEEP;

        // Si el botón está pulsado
        if (B1_ON)
        {
            if (debounce < 4)
                debounce++;  // 4 ticks × 20 ms = 80 ms estable
        }
        else
        {
            debounce = 0;
            pulsado = false;       // Podrá volver a disparar
        }

        // Si ha estado estable 80 ms y aún no se procesó esta pulsación
        if (debounce >= 4 && !pulsado)
        {

            pulsado = true;   // Para no repetir hasta que se suelte
            debounce = 4;
            led = true;

            //------------------------------------------------------------------
            // ACCIÓN: ENVIAR SEÑAL IR
            //------------------------------------------------------------------
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4); // LED indicador ON
            t_esp = 0;

            ir_send_raw(SIGNAL_MANDO_0.data, SIGNAL_MANDO_0.length);
        }

        // Apagar LED después de un tiempo)
        if (led)
        {
            t_esp++;
            if (t_esp > 4)
            {           // 4 ticks × 20 ms = 80 ms de LED encendido
                t_esp = 0;
                led = false;
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
            }
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
            IR_MARK();       // PWM ON (portadora)
        } else {
            IR_SPACE();      // PWM OFF (silencio)
        }

        delay_us(signal[i]);  // mantener MARK o SPACE

        mark = !mark;  // alternar
    }

    IR_SPACE();  // asegurarse de apagar al terminar
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
