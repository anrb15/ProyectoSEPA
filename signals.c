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
static const uint16_t ir_signal_mando0[] = {
    40000, 9110, 4460, 590, 590, 570, 590, 570, 590, 570, 590, 570, 590, 570,
    590, 570, 590, 570, 580, 580, 1660, 570, 1660, 570, 1660, 570, 1660, 570,
    1660, 570, 1660, 580, 1660, 570, 1660, 570, 1660, 570, 590, 570, 590, 570,
    1660, 570, 1660, 570, 590, 570, 590, 570, 590, 560, 590, 570, 1660, 570,
    1660, 580, 580, 570, 590, 570, 1660, 570, 1670, 570, 1660, 570, 39230,
    9110, 2240, 570, 40440
};

const IRSignal SIGNAL_MANDO_0 = {
                                 .data = ir_signal_mando0,
                                 .length = 73,
                                 .carrier_khz = 38, // NEC usa 38 kHz, ajustar PWM
                                 .name  = "MANDO_0"
};






//Tabla opcional de señales (A FUTURO BORRAR SI NO SE USA)
//const IRSignal * const ALL_SIGNALS[] = {
//    &SIGNAL_MANDO_0
//};
//const size_t NUM_SIGNALS = sizeof(ALL_SIGNALS) / sizeof(ALL_SIGNALS[0]);


