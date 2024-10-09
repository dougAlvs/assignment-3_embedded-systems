#ifndef MQTT_H
#define MQTT_H

#define MOSQUITTO 0
#define THINGSBOARD 1

extern int modo_movimento;
extern int leu_modo;

void mqtt_start();

void mqtt_envia_mensagem(int client, char * topico, char * mensagem);

void mqtt_stop();

#endif