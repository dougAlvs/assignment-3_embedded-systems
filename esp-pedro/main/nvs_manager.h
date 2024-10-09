#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <stdint.h>

// Funções para inicializar, salvar e carregar valores do NVS
void inicializar_nvs(void);
void salvar_modo_nvs(int modo);
uint8_t carregar_modo_nvs(void);

#endif // NVS_MANAGER_H
