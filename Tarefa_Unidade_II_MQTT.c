#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h" //Biblioteca para controle do display OLED
#include "lwip/netif.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/apps/lwiperf.h"//Bibliotecas de rede 

#include "pico/cyw43_arch.h"//Biblioteca da interface WIFI
#include "pico/time.h"
#include "mqtt_configuracao.h"//Configuracao MQTT
#include "leitura_sensor.h" // Leitura do sensor de temperatura

// --- DEFINIÇÕES DA REDE WIFI ---
#define SSID_WIFI "Rede" //Nome da rede wifi
#define SENHA_WIFI "Senha" //Senha da rede wifi

// Tópicos MQTT
#define TOPICO_LED_MQTT "pico_led_controle" // Topico onde sera publicada a mensagem
#define TOPICO_TEMP_MQTT "bitdoglab/temperature" // Topico onde sera publicada a temperatura

// Pinos GPIO para LED e I2C do display
#define PINO_LED 11 // Pino GPIO do LED verde
const uint I2C_SDA = 14;//Pino SDA do I2C (dados)
const uint I2C_SCL = 15;//Pino SCL do I2C (clock)

// Buffer e área de renderização para o display OLED
uint8_t buffer_display[ssd1306_buffer_length]; //Armazena o conteudo da imagem do display
struct render_area area_renderizacao = { 
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1,
};

// --- VARIÁVEL GLOBAL PARA A ÚLTIMA MENSAGEM RECEBIDA ---
char ultima_mensagem_mqtt_display[30] = "Nenhuma mensagem"; 

void callback_mqtt(char *topico, char *mensagem) { 
    printf("Monitor: Callback MQTT recebido - Topico: '%s', Mensagem: '%s'\n", topico, mensagem);
    snprintf(ultima_mensagem_mqtt_display, sizeof(ultima_mensagem_mqtt_display), "Msg: %s", mensagem);
    printf("DEBUG_CALLBACK: ultima_mensagem_mqtt_display atualizado para: '%s'\n", ultima_mensagem_mqtt_display);
    //Se a mensagem for "on" liga o LED, se for "off" desliga o LED
    if (strcmp(topico, TOPICO_LED_MQTT) == 0) {
        printf("Monitor: Mensagem para controle do LED.\n");
        if (strcmp(mensagem, "on") == 0 || strcmp(mensagem, "1") == 0) {
            gpio_put(PINO_LED, 1);
            printf("Monitor: LED LIGADO!\n");
        } else if (strcmp(mensagem, "off") == 0 || strcmp(mensagem, "0") == 0) {
            gpio_put(PINO_LED, 0);
            printf("Monitor: LED DESLIGADO!\n");
        } else {
            printf("Monitor: Mensagem desconhecida para o LED: '%s'\n", mensagem);
        }
    }
}

//----FUNCAO PRINCIPAL---
int main() {
    stdio_init_all();//Inicializa o sistema

    // --- Conexao WIFI ----
    printf("Monitor: Iniciando Wi-Fi...\n");
    if (cyw43_arch_init()) {
        printf("Monitor: Falha na inicializacao do Wi-Fi CYW43.\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Monitor: Conectando ao Wi-Fi %s...\n", SSID_WIFI);
    int tentativas = 0; 
    while (cyw43_arch_wifi_connect_timeout_ms(SSID_WIFI, SENHA_WIFI, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Monitor: Falha na conexao Wi-Fi. Tentando novamente...\n");
        tentativas++;
        if (tentativas > 5) {
            printf("Monitor: Nao foi possivel conectar ao Wi-Fi. Verifique as credenciais.\n");
            cyw43_arch_deinit();
            return 1;
        }
        sleep_ms(2000);
    }
    printf("Monitor: Wi-Fi conectado com sucesso!\n");

    //---- Inicializa LED e sensor ----
    gpio_init(PINO_LED);
    gpio_set_dir(PINO_LED, GPIO_OUT);
    inicializar_pinos(); //Habilita leitura do sensor (ADC)

    // --- Inicializacao do I2C e display OLED ---
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();//Inicializa o display
    calculate_render_area_buffer_length(&area_renderizacao);

    //---- Conexao com MQTT ----
    printf("Monitor: Iniciando conexao MQTT...\n");
    inicializar_mqtt(); 
    definir_callback_usuario_mqtt(callback_mqtt); 

    //--- Exibe status de conexao MQTT no display ---
    uint32_t tempo_inicio = to_ms_since_boot(get_absolute_time()); 
    while (!mqtt_esta_conectado && (to_ms_since_boot(get_absolute_time()) - tempo_inicio < 1000)) {
        memset(buffer_display, 0, ssd1306_buffer_length);
        ssd1306_draw_string(buffer_display, 0, 0, "Monitor Pico");
        ssd1306_draw_string(buffer_display, 0, 16, "Conectando...");
        render_on_display(buffer_display, &area_renderizacao);

        printf("Monitor: Aguardando conexao MQTT... (%lums de 1000ms)\n", (to_ms_since_boot(get_absolute_time()) - tempo_inicio));
        sleep_ms(1000);
    }

    if (mqtt_esta_conectado) {
        printf("Monitor: MQTT conectado! Tentando subscrever ao topico do LED.\n");
        subscrever_topico_mqtt(TOPICO_LED_MQTT); 

        memset(buffer_display, 0, ssd1306_buffer_length);
        ssd1306_draw_string(buffer_display, 0, 0, "Monitor Pico");
        ssd1306_draw_string(buffer_display, 0, 16, "Conectado");
        ssd1306_draw_string(buffer_display, 0, 32, ultima_mensagem_mqtt_display);
        render_on_display(buffer_display, &area_renderizacao);
        sleep_ms(200);
    } else {
        printf("Monitor: Falha na conexao MQTT apos timeout. Funcoes MQTT nao serao usadas.\n");
    }

    printf("Monitor: Entrando no loop principal...\n");

    //---- LOOP PRINCIPAL ----
    int contador_loop = 0; 
    while (true) {
        float temperatura_atual = ler_temperatura(); // Leitura da temperatura 
        char temp_str[10]; // Buffer para a string da temperatura
        sprintf(temp_str, "%.2f", temperatura_atual); // Formata para 2 casas decimais
        publicar_mensagem(TOPICO_TEMP_MQTT, temp_str); //  Publica a temperatura 

        memset(buffer_display, 0, ssd1306_buffer_length);

        ssd1306_draw_string(buffer_display, 0, 0, "Monitor Pico");

        if (mqtt_esta_conectado) {
            ssd1306_draw_string(buffer_display, 0, 16, "Conectado");
        } else {
            ssd1306_draw_string(buffer_display, 0, 16, "Desconectado");
        }

        ssd1306_draw_string(buffer_display, 0, 32, ultima_mensagem_mqtt_display);
        
        // Exibir a temperatura no display
        char temp_display_str[20];
        sprintf(temp_display_str, "Temp: %.2f C", temperatura_atual);
        ssd1306_draw_string(buffer_display, 0, 48, temp_display_str); // Exibe a temperatura no display

        bool estado_led = gpio_get_out_level(PINO_LED); 
        printf("DEBUG: Estado atual do LED_PIN: %d\n", estado_led);

        char string_contador[10]; 
        sprintf(string_contador, "C:%d", contador_loop++);
        ssd1306_draw_string(buffer_display, 80, 0, string_contador); 


        render_on_display(buffer_display, &area_renderizacao);

        sleep_ms(5000); // Publica/atualiza a cada 5 segundos
    }

    cyw43_arch_deinit();
    return 0;
}
