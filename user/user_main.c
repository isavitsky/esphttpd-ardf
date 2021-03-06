/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off 
the server, including WiFi connection management capabilities, some IO and
some pictures of cats.
*/

#include <esp8266.h>
#include "httpd.h"
#include "httpdespfs.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "stdout.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "cgiwebsocket.h"
#include "webpages-espfs.h"

#include "user_main.h"
#include "config.h"
#include "uart.h"
#include "min.h"
#include "fox_cmd.h"
#include "fox_wifi.h"
#include "fox_cgi.h"
#include "fox_udp.h"

#ifdef SHOW_HEAP_USE
static ETSTimer prHeapTimer;

static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
        os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
}
#endif

struct min_context min_ctx;
struct flag_t flag;
struct rtctime_t tm;
LOCAL ETSTimer foxcTimer;
LOCAL ETSTimer rtcTimer;
LOCAL ETSTimer wsTimer;
LOCAL ETSTimer iodelayTimer;
uint8 foxes[FOXNUM][MYCFG_SIZE];
uint8 mycfg[MYCFG_SIZE];

#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_ESPFS,
	.fw1Pos=ESPFS_POS,
	.fw2Pos=0,
	.fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
#if OTA_FLASH_SIZE_K < 2049
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
#else
	.fw2Pos=0x101000,
	.fwSize=0xFF000,
#endif
	.tagName=OTA_TAGNAME
};
#define INCLUDE_FLASH_FNS
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "ardf.sport"},
	{"/", cgiRedirect, "/index.html"},
#ifdef INCLUDE_FLASH_FNS
	{"/flash/next", cgiGetFirmwareNext, &uploadParams},
	{"/flash/upload", cgiUploadFirmware, &uploadParams},
#endif
	{"/flash/reboot", cgiRebootFirmware, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.html"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.html"},
	{"/wifi/setmode.cgi", cgiWiFiSetCfg, NULL},

	{WS_RESOURCE_UPD, cgiWebsocket, myWsConnect},
	{WS_RESOURCE_IAN, cgiWebsocket, myWsConnectIant},

	{"/fox/cfg_get", cgiFoxCfgGet, NULL},
	{"/fox/cfg_save", cgiFoxCfgSave, NULL},
	{"/fox/cfg_savexpert", cgiFoxCfgSaveXpert, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

void ICACHE_FLASH_ATTR foxcTimerCb(void *arg) { // called from fox_wifi.c
   uint8 i;

   for (i = 0; i <FOXNUM; i++) {
      if ( foxes[i][0] != LOCAL_TAG && foxes[i][0] > 0 ) foxes[i][0]--;
   }
#ifdef DEBUG2
	os_printf("____________fx: ");
	for (i=0; i<FOXNUM; i++) os_printf(" %d", foxes[i][0]);
	os_printf("\n");
	os_printf("   my_id: %d, got_mycfg: %d\n", mycfg[0], flag.got_mycfg);
#endif
}

void ICACHE_FLASH_ATTR iodelayCb(void *arg) {
	init_wifi();
}

void ICACHE_FLASH_ATTR io_delay(void) {
	wifi_station_disconnect();
	wifi_station_set_auto_connect(0);
    os_timer_disarm(&iodelayTimer);
    os_timer_setfn(&iodelayTimer, iodelayCb, NULL);
    os_timer_arm(&iodelayTimer, IO_DELAY, 0);
}

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void ICACHE_FLASH_ATTR user_init(void) {
	min_init_context(&min_ctx, 0);
    uart_init(UART0_SPEED, UART1_SPEED);
    uart_add_recv_cb(&rxUartCb);
	init_fx();
	stdoutInit();
	os_printf("\n\n\nSDK version:%s\n", system_get_sdk_version());
	// Set up timer for read out local config from fox's EEPROM
    flag.got_mycfg = false;
	send_uart_cmd(REQ_READCFG);
	rtc_init();

	// set wifi STA mode on startup
	//wifi_set_opmode_current(SOFTAP_MODE);

//    udp_init();
//	captdnsInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif
	httpdInit(builtInUrls, 80);

#ifdef SHOW_HEAP_USE
        os_timer_disarm(&prHeapTimer);
        os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
        os_timer_arm(&prHeapTimer, 3000, 1);
#endif

    // WebSocket timer
    os_timer_disarm(&wsTimer);
    os_timer_setfn(&wsTimer, wsTimerCb, NULL);
    os_timer_arm(&wsTimer, WS_TIMER, 1);

    // fox configuration expiration timer for web-interface
	os_timer_disarm(&foxcTimer);
    os_timer_setfn(&foxcTimer, foxcTimerCb, NULL);
    os_timer_arm(&foxcTimer, FOXC_TIMER, 1);

    // I/O delay to get current config via serial
    system_init_done_cb(io_delay);
	os_printf("\nuser_main() Ready\n");
}

void ICACHE_FLASH_ATTR user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}


//Sdk 2.0.0 needs extra sector to store rf cal stuff. Place that at the end of the flash.
/*uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
		case FLASH_SIZE_4M_MAP_256_256:
			rf_cal_sec = 128 - 8;
			break;

		case FLASH_SIZE_8M_MAP_512_512:
			rf_cal_sec = 256 - 5;
			break;

		case FLASH_SIZE_16M_MAP_512_512:
		case FLASH_SIZE_16M_MAP_1024_1024:
			rf_cal_sec = 512 - 5;
			break;

		case FLASH_SIZE_32M_MAP_512_512:
		case FLASH_SIZE_32M_MAP_1024_1024:
			rf_cal_sec = 1024 - 5;
			break;

		default:
			rf_cal_sec = 0;
			break;
	}

	return rf_cal_sec;
}
*/

void ICACHE_FLASH_ATTR init_fx(void) {
   uint8 i;

   for (i = 0; i <FOXNUM; i++) {
      foxes[i][0] = 0;
   }
}

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int ICACHE_FLASH_ATTR myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}

uint16 ICACHE_FLASH_ATTR min_tx_space(uint8 port) {
  return 256U;
}

void ICACHE_FLASH_ATTR min_tx_byte(uint8 port, uint8 byte) {
    uart_tx_one_char(port, byte);
}

void ICACHE_FLASH_ATTR min_tx_start(uint8 port) {
}

void ICACHE_FLASH_ATTR min_tx_finished(uint8 port) {
}

void ICACHE_FLASH_ATTR min_application_handler(uint8 min_id, uint8 *min_payload, uint8 len_payload, uint8 port) {
	uint8 i;

#ifdef DEBUG
    os_printf("MIN frame received: id: %x, length: %d, payload: ", min_id, len_payload);
    for (uint8 i=0; i<len_payload; i++) {
        os_printf("%02X ", min_payload[i]);
    }
    os_printf("\n");
#endif
    if (flag.uart_cmd_issued) {
    	flag.uart_cmd_issued = false;
    	flag.uart_cmd_gotreply = true;
    	for(i=0;i<len_payload;i++)
    		cmd_rx_buffer[i] = min_payload[i];
    }
    switch (min_id) {
    	case REP_SENDCFG:
    	flag.got_mycfg = true;
    	mycfg[MY_FOX_IDX] = cmd_rx_buffer[2];
    	mycfg[MY_NRF_IDX] = cmd_rx_buffer[3];
    	mycfg[MY_SDN_IDX] = cmd_rx_buffer[4];
    	mycfg[MY_ACH_IDX] = cmd_rx_buffer[8];
    	mycfg[MY_ACL_IDX] = cmd_rx_buffer[9];
    	mycfg[MY_DDH_IDX] = cmd_rx_buffer[12];
    	mycfg[MY_DDL_IDX] = cmd_rx_buffer[13];
    	break;
    	case REP_TIMESET:
#ifdef DEBUG3
    		os_printf("Calling wsReply()");
#endif
    	wsReply(REP_TIMESET, cmd_rx_buffer[0]);
#ifdef DEBUG3
    		os_printf(" ... Done\n");
#endif
    	break;
    	case REP_TUNING:
#ifdef DEBUG
    		os_printf("Calling wsReplyIant()");
#endif
    	wsReplyIant(REP_TUNING, min_payload, len_payload);
    	break;
    	case REP_TUNMAX:
    	wsReplyIant(REP_TUNMAX, min_payload, len_payload);
    	break;
    	case REP_BLVAL:
    	wsReplyIant(REP_BLVAL, min_payload, len_payload);
    	break;
    }
    if ( flag.uart_req_for_cgi_sent ) {
    	flag.uart_rep_for_cgi_rcvd = true;
    	httpdContinue(save_connData);
    }
}

void ICACHE_FLASH_ATTR rtcTimeCb (void *arg) {
	static uint8 month_day_table[12]={ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint8 tmp;

	tm.ss++;
	if (tm.ss > 59) {
		tm.ss = 0;
		tm.mm++;
		if (tm.mm > 59) {
			tm.mm = 0;
			tm.hh++;
			if (tm.hh > 23) {
				tm.hh = 0;
				tm.dd++;
				/* Test for leap year and February */
				if ((tm.yy != 0) && (tm.yy % 4 == 0) && (tm.mn == 2)) {
					tmp = 29;
				} else {
					tmp = month_day_table[tm.mn - 1];
				}
				if (tm.dd > tmp) {
					tm.dd = 1;
					tm.mn++;
					if (tm.mn > 12) {
						tm.mn = 1;
						tm.yy++;
					}
				}
			}
		}
	}
#ifdef DEBUG2
	os_printf("\n                            TSI: %02d\n", mycfg[MY_TSI_IDX]);
#endif
#ifdef DEBUG2
	os_printf("\n                            Date/Time: %02d-%02d-%02d %02d:%02d:%02d\n", tm.yy, tm.mn, tm.dd, tm.hh, tm.mm, tm.ss);
#endif
	mycfg[MY_CSS_IDX] = tm.ss;
	mycfg[MY_CMM_IDX] = tm.mm;
	mycfg[MY_CHH_IDX] = tm.hh;
	mycfg[MY_CDD_IDX] = tm.dd;
	mycfg[MY_CMN_IDX] = tm.mn;
	mycfg[MY_CYY_IDX] = tm.yy;
	udp_send_update(UDP_CMD_C);
}

void ICACHE_FLASH_ATTR rtc_init(void) {
	tm.ss = mycfg[MY_CSS_IDX];
	tm.mm = mycfg[MY_CMM_IDX];
	tm.hh = mycfg[MY_CHH_IDX];
	tm.dd = mycfg[MY_CDD_IDX];
	tm.mn = mycfg[MY_CMN_IDX];
	tm.yy = mycfg[MY_CYY_IDX];
	os_timer_disarm(&rtcTimer);
    os_timer_setfn(&rtcTimer, (os_timer_func_t *)rtcTimeCb, NULL);
    os_timer_arm(&rtcTimer, 1000, 1);
}
