#ifndef MQTT_CONFIGURACAO_H 
#define MQTT_CONFIGURACAO_H 


#include "lwip/apps/mqtt.h"//Biblioteca para suporte ao protocolo MQTT
#include <stdbool.h> //Biblioteca para manipulacao de valores

// --- CONFIGURAÇÕES DO BROKER MQTT ---

// OPÇÃO 1: Para um broker MQTT local (como HiveMQ CE ou Mosquitto rodando no seu PC)
// #define MQTT_BROKER_IP "192.XXX.0.XXX" // <<< Substitua pelo IP LOCAL do seu computador/servidor

// OPÇÃO 2: Para o broker público da HiveMQ (necessita de DNS, mais complexo para setup inicial)
#define NOME_HOST_BROKER_MQTT "broker.hivemq.com"  // Nome do host do broker MQTT público
#define MQTT_BROKER_PORTA 1883  // Porta padrão para MQTT (1883 para MQTT, 8883 para MQTTS)

// --- FUNÇÕES PARA MANIPULAÇÃO DO MQTT ---
void inicializar_mqtt(); 
// Publica uma mensagem em um tópico MQTT
void publicar_mensagem(const char *topico, const char *mensagem); 
// Define um callback para processar mensagens recebidas via MQTT
void definir_callback_usuario_mqtt(void (*callback)(char *topico, char *mensagem)); 
// Subscreve a um tópico MQTT para receber mensagens
void subscrever_topico_mqtt(const char *topico); 

// Variável para verificar o status da conexão MQTT
extern bool mqtt_esta_conectado; // Declarada como externa para ser acessada em outros arquivos 

#endif 