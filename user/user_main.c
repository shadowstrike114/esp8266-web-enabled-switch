#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <user_interface.h>
#include <mem.h>
#include "driver/uart.h"

void netinit();
os_event_t    procTaskQueue[1];
extern int ets_uart_printf(const char *fmt, ...);
int status = 0;

void ICACHE_FLASH_ATTR recv_cb(void *arg, char *pdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;

	if (strstr(pdata,"GET /?Button=AN"))
	{
		ets_uart_printf("\r\n");
		ets_uart_printf("ANGESCHALTET\r\n");
		ets_uart_printf("\r\n");
		status = 1;
		GPIO_OUTPUT_SET(0,0);
	}else if (strstr(pdata,"GET /?Button=AUS"))
	{
		ets_uart_printf("\r\n");
		ets_uart_printf("AUSGESCHALTET\r\n");
		ets_uart_printf("\r\n");
		status = 0;
		GPIO_OUTPUT_SET(0,1);
	}else{
		//ERROR SEITE
		ets_uart_printf("\r\n");
		ets_uart_printf("FEHLER BZW. ERSTE VERBINDUNG\r\n");
		ets_uart_printf("\r\n");
	}
	ets_uart_printf(pdata);
	ets_uart_printf("\r\n");
	char html[512];

	char statusAN[128]="<span style=\"font-family:'Times New Roman',Times,serif; font-size:200%;color:#00FF00\">AN</span>";
	char statusAUS[128]="<span style=\"font-family:'Times New Roman',Times,serif; font-size:200%;color:#FF0000\">AUS</span>";
	if (status){
		os_sprintf(html,"<html><head><title>Lichtsteureung 1.0</title></head><span style=\"font-family:'Times New Roman',Times,serif; font-size:200%\">STATUS: </span>%s</p><form><input type =\"submit\" name=\"Button\" style=\"width:120px;height:120px;font-family:'Times New Roman',Times,serif;font-size:200%\" value=\"AN\"><input type =\"submit\" name=\"Button\" style=\"width:120px;height:120px;font-family:'Times New Roman',Times,serif;font-size:200%\" value=\"AUS\"></form", statusAN);
	}else{
		os_sprintf(html,"<html><head><title>Lichtsteureung 1.0</title></head><span style=\"font-family:'Times New Roman',Times,serif; font-size:200%\">STATUS: </span>%s</p><form><input type =\"submit\" name=\"Button\" style=\"width:120px;height:120px;font-family:'Times New Roman',Times,serif;font-size:200%\" value=\"AN\"><input type =\"submit\" name=\"Button\" style=\"width:120px;height:120px;font-family:'Times New Roman',Times,serif;font-size:200%\" value=\"AUS\"></form", statusAUS);
	}
	char httpsend[1024];
	os_sprintf(httpsend, "HTTP/1.0 200 OK\r\nServer: ESP8266\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\n\n%s", os_strlen(html),html );
	int leng = os_strlen(httpsend);
	espconn_sent(pespconn, httpsend, leng);
}

void ICACHE_FLASH_ATTR server_connectcb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;

	ets_uart_printf("VERBINDUNG EINGEHEND\r\n");

	espconn_regist_recvcb(pespconn, recv_cb);
}

void user_init(void)
{
	system_update_cpu_freq(160); //Mit SDK1.0 neu

	GPIO_OUTPUT_SET(0,1);

	// Configure the UART
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	ets_uart_printf("\r\n");
	ets_uart_printf("-------------------------\r\n");
	ets_uart_printf("START\r\n");
	ets_uart_printf("-------------------------\r\n");
	ets_uart_printf("CPU: ");
	ets_uart_printf("%d\r\n",system_get_cpu_freq());

	system_os_task(netinit, 0, procTaskQueue, 1);
	system_os_post(0, 0, 0 );
}

void ICACHE_FLASH_ATTR netinit()
{
	ets_uart_printf("Netzwerk wird initialisiert\r\n");

	const char ssid[32] = "LichtWlan";
	const char password[32] = "12345678";

	struct softap_config config;

	wifi_softap_get_config(&config);

	os_memcpy(&config.ssid, ssid, 32);
	os_memcpy(&config.password, password, 32);

	config.authmode = AUTH_WPA_WPA2_PSK;
//	config.channel = 7;
//	config.max_connection = 4;
//	config.ssid_hidden = 0;
//	config.ssid_len = 0;


	wifi_set_opmode(2);

	ets_uart_printf("Erfolg: %d\r\n",wifi_softap_set_config(&config));
	//wifi_station_set_config(&config);
	//wifi_station_connect();

	struct espconn *pserver;
	pserver = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pserver, 0, sizeof( struct espconn ) );

	espconn_create(pserver);
	pserver->type = ESPCONN_TCP;
	pserver->state = ESPCONN_NONE;

	pserver->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pserver->proto.tcp->local_port = 80;

	espconn_regist_connectcb(pserver, server_connectcb);
	espconn_accept(pserver);
	espconn_regist_time(pserver, 15, 0);
}

