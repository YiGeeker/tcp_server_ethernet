/* TCP Server Ethernet Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "esp_eth_com.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "ethernet_init.h"
#include "sdkconfig.h"
#include "tcp_server.h"

static const char *TAG = "tcp_server_ethernet";

static esp_err_t ethernet_set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}
 
static void ethernet_set_static_ip(esp_netif_t *netif)
{
	const char *ip = "192.168.12.15";
	const char *netmask = "255.255.255.0";
	const char *gw = "192.168.12.1";
	
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0 , sizeof(esp_netif_ip_info_t));
    ip_info.ip.addr = ipaddr_addr(ip);
    ip_info.netmask.addr = ipaddr_addr(netmask);
    ip_info.gw.addr = ipaddr_addr(gw);
    if (esp_netif_set_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }
    ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", ip, netmask, gw);
    ESP_ERROR_CHECK(ethernet_set_dns_server(netif, ipaddr_addr(gw), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(ethernet_set_dns_server(netif, ipaddr_addr(gw), ESP_NETIF_DNS_BACKUP));	
}

/* Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/* Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

void app_main(void)
{
    /* Initialize Ethernet driver */ 
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    /* Initialize TCP/IP network interface aka the esp-netif (should be called only once in application) */ 
    ESP_ERROR_CHECK(esp_netif_init());
    /* Create default event loop that running in background */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create instance(s) of esp-netif for Ethernet(s) */
    if (eth_port_cnt == 1) {
        /* Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
           default esp-netif configuration parameters.
        */
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        /* Attach Ethernet driver to TCP/IP stack */
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));

		/* ESP_LOGI(TAG, "dhcpc_stop begin"); */
		/* ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif)); */
		/* ESP_LOGI(TAG, "dhcpc_stop end"); */

        ethernet_set_static_ip(eth_netif);
		/* Use ethernet_set_static function or codes below without setting dns. */
		/* esp_netif_ip_info_t info_t; */

        /* memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
		   info_t.ip.addr = esp_ip4addr_aton((const char *)"192.168.12.15");
		   info_t.netmask.addr = esp_ip4addr_aton((const char *)"255.255.255.0");
		   info_t.gw.addr = esp_ip4addr_aton((const char *)"192.168.12.1");
		   ESP_LOGI(TAG, "static ip begin");
		   ESP_ERROR_CHECK( esp_netif_set_ip_info(eth_netif, &info_t));
		   ESP_LOGI(TAG, "static ip end"); 
        */
    } else {
        /* Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
           esp-netif configuration parameters for each interface (name, priority, etc.). 
        */
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
        };
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++) {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i*5;
            esp_netif_t *eth_netif = esp_netif_new(&cfg_spi);

            /* Attach Ethernet driver to TCP/IP stack */
            ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        }
    }

    /* Register user defined event handers */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    /* Start Ethernet driver state machine */
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }

	#ifdef CONFIG_TCP_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
    #endif
    #ifdef CONFIG_TCP_IPV6
        xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
    #endif
}
