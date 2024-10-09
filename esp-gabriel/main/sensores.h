#ifndef SENSORES_H
#define SENSORES_H

#include <stdbool.h>

void configurar_leitura_analogica(void);
int ler_potenciometro(void);
bool ler_ldr(void);
void potenciometro_task(void *pvParameters);
void ldr_task(void *pvParameters);

#endif // SENSORES_H
