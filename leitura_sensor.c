#include "leitura_sensor.h" 
#include <stdio.h> 

void inicializar_pinos() 
{
    adc_init();  // Inicializa o ADC
    // Não é necessário chamar adc_gpio_init() para o sensor de temperatura interno do RP2040.
    // Ele é acessado pelo canal 4 do ADC, que já é gerenciado internamente pela biblioteca.
}

float ler_temperatura() 
{
    adc_select_input(4);  // Seleciona o canal 4 do ADC para o sensor de temperatura interno do RP2040
    uint16_t valor_bruto = adc_read();  // Lê o valor do ADC 

    printf("SENSOR: ADC Bruto (Interno): %u\n", valor_bruto);

    // Converte o valor bruto do ADC para tensão (3.3V referência, 12 bits)
    float voltagem = (valor_bruto * 3.3f) / 4095.0f; 
    printf("SENSOR: Voltagem (Interno): %.4f V\n", voltagem);

    // Fórmula específica para o sensor de temperatura interno do chip RP2040.
    // Esta fórmula é baseada na documentação do Pico SDK.
    // 27.0f é a tensão de referência a 0.706V, e 0.001721f é a taxa de mudança de tensão por grau Celsius.
    float temperatura = 27.0f - ((voltagem - 0.706f) / 0.001721f); 
    
    printf("SENSOR: Temperatura (RP2040 Interno): %.2f C\n", temperatura);
    return temperatura;
}