#include "mqtt_configuracao.h" 
#include <stdio.h>
#include <string.h>
#include "lwip/init.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dns.h" 
#include <stdbool.h>
#include "pico/stdlib.h"

//Instancia do cliente MQTT
mqtt_client_t *cliente_mqtt_instancia; 

// Ponteiro para a função de callback do usuário, chamando quando ha uma nova mensagem MQTT
static void (*_callback_usuario_mqtt)(char *topico, char *mensagem) = NULL; 

static char topico_entrada_atual[100]; // Buffer para armazenar o tópico da mensagem recebida 

bool mqtt_esta_conectado = false; // Flag para o status da conexão MQTT 

// Callback de conexão MQTT
void callback_conexao_mqtt(mqtt_client_t *cliente, void *arg, mqtt_connection_status_t status) 
{
    if (status == MQTT_CONNECT_ACCEPTED)//Conectado
    {
        printf("MQTT: Conectado ao servidor!\n");
        mqtt_esta_conectado = true;
    }
    else//Falha na conexao
    {
        printf("MQTT: Erro na conexao: %d (status: %d)\n", status, status);
        mqtt_esta_conectado = false;
    }
}

// Callback para quando uma nova publicação chega
void callback_publicacao_chegando_mqtt(void *arg, const char *topico, u32_t comprimento_total) { 
    printf("MQTT: Publicacao recebida no topico: %s (comprimento total: %u)\n", topico, (unsigned int)comprimento_total);
    // Armazena o topico para ser usado no callback de dados
    strncpy(topico_entrada_atual, topico, sizeof(topico_entrada_atual) - 1);
    topico_entrada_atual[sizeof(topico_entrada_atual) - 1] = '\0'; // Garante terminação nula
}

// Callback para os dados da publicação recebida
void callback_dados_chegando_mqtt(void *arg, const u8_t *dados, u16_t comprimento, u8_t flags) { 
    char mensagem[256]; // Buffer para a mensagem recebida 
    // Copia os dados para o buffer e garante terminação nula
    strncpy(mensagem, (const char *)dados, comprimento);
    mensagem[comprimento] = '\0';

    printf("MQTT: Dados recebidos: '%s' (len: %u)\n", mensagem, comprimento);

    // Se houver um callback de usuario registrado, ele será chamado com os dados da mensagem
    if (_callback_usuario_mqtt) {
        _callback_usuario_mqtt(topico_entrada_atual, mensagem);
    }
}

// Callback para quando uma publicação é concluída
void callback_publicacao_concluida_mqtt(void *arg, err_t erro) { 
    printf("MQTT: Publicacao concluida com status: %d\n", erro);
}

// Callback para quando uma solicitação de subscrição é concluída
void callback_solicitacao_sub_mqtt(void *arg, err_t erro) { 
    const char *topico = (const char *)arg; // O tópico é passado como argumento
    if (erro == ERR_OK) {
        printf("MQTT: Subscricao ao topico '%s' bem-sucedida!\n", topico);
    } else {
        printf("MQTT: Falha na subscricao ao topico '%s': %d\n", topico, erro);
    }
}

// Inicializa o cliente MQTT e tenta conectar ao broker
void inicializar_mqtt() 
{
    cliente_mqtt_instancia = mqtt_client_new();
    if (cliente_mqtt_instancia == NULL)
    {
        printf("MQTT_INIT: ERRO FATAL: Falha ao criar cliente MQTT!\n");
        return;
    }
    printf("MQTT_INIT: Cliente MQTT criado.\n");

    // Configura as informações do cliente MQTT
    struct mqtt_connect_client_info_t info_cliente = {.client_id = "bitdoglab_cliente_picow", .keep_alive = 60}; 

    // Usa o IP LOCAL para conexão (mais simples para debug inicial)
    
    const ip_addr_t ip_broker = IPADDR4_INIT_BYTES(192, xxx, 0, xxx); // <<< Use o IP do seu broker local
                                                                    

    printf("MQTT_INIT: Tentando conectar ao broker IP: %s\n", ipaddr_ntoa(&ip_broker));

    err_t erro = mqtt_client_connect(cliente_mqtt_instancia, &ip_broker, MQTT_BROKER_PORTA, callback_conexao_mqtt, NULL, &info_cliente); 
    if (erro != ERR_OK)
    {
        printf("MQTT_INIT: Erro ao iniciar conexao ao broker (assincrona): %d\n", erro);
    } else {
        printf("MQTT_INIT: Conexao MQTT iniciada (assincrona). Aguardando callback.\n");
    }

    // Configura os callbacks para mensagens de entrada (publicações e dados)
    mqtt_set_inpub_callback(cliente_mqtt_instancia, callback_publicacao_chegando_mqtt, callback_dados_chegando_mqtt, NULL); 
}

// Publica uma mensagem MQTT
void publicar_mensagem(const char *topico, const char *mensagem) 
{
    // Apenas publica se o cliente MQTT estiver inicializado e conectado
    if (cliente_mqtt_instancia != NULL && mqtt_esta_conectado)
    {
        mqtt_publish(cliente_mqtt_instancia, topico, mensagem, strlen(mensagem), 0, 0, callback_publicacao_concluida_mqtt, NULL); 
        printf("MQTT: Mensagem enviada: %s -> %s\n", topico, mensagem);
    } else if (cliente_mqtt_instancia == NULL) {
        printf("MQTT: Erro: Cliente MQTT nao inicializado (cliente_mqtt_instancia is NULL).\n");
    } else {
        printf("MQTT: Aviso: Cliente MQTT nao conectado. Nao foi possivel publicar '%s'.\n", topico);
    }
}

// Registra a função de callback do usuário para mensagens MQTT recebidas
void definir_callback_usuario_mqtt(void (*callback)(char *topico, char *mensagem)) { 
    _callback_usuario_mqtt = callback;
}

// Subscreve a um tópico MQTT
void subscrever_topico_mqtt(const char *topico) { 
    // Apenas subscreve se o cliente MQTT estiver inicializado e conectado
    if (cliente_mqtt_instancia != NULL && mqtt_esta_conectado) {
        err_t erro = mqtt_subscribe(cliente_mqtt_instancia, topico, 0, callback_solicitacao_sub_mqtt, (void *)topico); 
        if (erro != ERR_OK) {
            printf("MQTT: Erro ao subscrever ao topico %s: %d\n", topico, erro);
        } else {
            printf("MQTT: Tentando subscrever ao topico: %s\n", topico);
        }
    } else if (cliente_mqtt_instancia == NULL) {
        printf("MQTT: Erro: Cliente MQTT nao inicializado para subscrever (cliente_mqtt_instancia is NULL).\n");
    } else {
        printf("MQTT: Aviso: Cliente MQTT nao conectado. Nao foi possivel subscrever ao topico '%s'.\n", topico);
    }
}
