#ifndef TCP_SERVER
#define TCP_SERVER

#include "esp_log.h"
#include "lwip/sockets.h"

#define PORT                        CONFIG_SERVER_PORT
#define KEEPALIVE_IDLE              CONFIG_TCP_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_TCP_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_TCP_KEEPALIVE_COUNT

void do_retransmit(const int sock);
void tcp_server_task(void *pvParameters);

#endif
