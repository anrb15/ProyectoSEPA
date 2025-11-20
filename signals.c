/*
 *
 * Librería para las señales a emitir por el mando
 *
 */

#include "signals.h"


// Señal capturada del archivo .csv
// Valores en microsegundos
// Alterna MARK (1), SPACE (0), MARK (1), SPACE (0)...

// Señal mando genérico, botón 0
const uint16_t ir_signal[] = { 0, 9100, 4450, 600, 550, 600, 600, 600, 550, 600,
                               550, 600, 550, 600, 550, 600, 600, 550, 600, 600,
                               1650, 550, 1700, 550, 1650, 600, 1650, 549, 1651,
                               600, 1650, 550, 1700, 550, 1650, 600, 1650, 550,
                               600, 600, 550, 600, 1650, 550, 1700, 550, 600,
                               550, 600, 550, 600, 600, 550, 600, 1650, 600,
                               1650, 550, 600, 550, 600, 600, 1650, 550, 1700,
                               550, 1650, 600, 39200 };

const IRSignal SIGNAL_MANDO_0 = {
                                 .data = ir_signal_mando0,
                                 .length = 69,
                                 .carrier_khz = 38, // NEC usa 38 kHz, ajustar PWM
                                 .name  = "MANDO_0"
};






//Tabla opcional de señales (A FUTURO BORRAR SI NO SE USA)
//const IRSignal * const ALL_SIGNALS[] = {
//    &SIGNAL_MANDO_0
//};
//const size_t NUM_SIGNALS = sizeof(ALL_SIGNALS) / sizeof(ALL_SIGNALS[0]);


