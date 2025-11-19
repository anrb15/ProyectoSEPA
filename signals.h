/*
 *
 * Librería para las señales a emitir por el mando
 *
 */

#ifndef LOGOS_H_
#define LOGOS_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint16_t *data;   // Array con duraciones MARK/SPACE
    size_t length;          // Elementos del array
    uint16_t carrier_khz;   // Frecuencia de portadora en kHz (opcional)
    const char *name;       // Nombre simbólico (para debug)
} IRSignal;

// Declaraciones de señales disponibles
extern const IRSignal SIGNAL_MANDO_0;

// Tabla para recorrer todas las señales (A FUTURO BORRAR SI NO SE USA)
//extern const IRSignal * const ALL_SIGNALS[];
//extern const size_t NUM_SIGNALS;

#endif
