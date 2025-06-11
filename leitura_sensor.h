#ifndef LEITURA_SENSOR_H  
#define LEITURA_SENSOR_H  

#include "pico/stdlib.h"  // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital)

// --- DECLARAÇÃO DAS FUNÇÕES ---

// Inicializa os pinos necessários para leitura do sensor (configuração do ADC)
void inicializar_pinos();  

// Função para obter a temperatura medida pelo sensor
float ler_temperatura();  

#endif  // Finaliza 