#include <esp8266.h>
#include <user_interface.h>
#include "config.h"
#include "fox_wifi.h"
#include "user_main.h"
#include "fox_udp.h"

static void ICACHE_FLASH_ATTR event_handler(System_Event_t *evt) {
   struct ip_info ipinfo;

   switch (evt->event) {
      case EVENT_STAMODE_GOT_IP:
         udp_init();
         if (wifi_get_ip_info(0, &ipinfo)) {
            mycfg[MY_IP4_IDX] = ip4_addr1(&ipinfo.ip);
            mycfg[MY_IP3_IDX] = ip4_addr2(&ipinfo.ip);
            mycfg[MY_IP2_IDX] = ip4_addr3(&ipinfo.ip);
            mycfg[MY_IP1_IDX] = ip4_addr4(&ipinfo.ip);
         }
         break;
      case EVENT_STAMODE_DISCONNECTED:
      case EVENT_SOFTAPMODE_STADISCONNECTED:
         break;
      case EVENT_STAMODE_CONNECTED:
      case EVENT_SOFTAPMODE_STACONNECTED:
         wifi_station_dhcpc_start();
         break;
   }
}

void ICACHE_FLASH_ATTR init_wifi(void) {
   char hname[32];
   char suffix[4];

   switch (mycfg[MY_FOX_IDX]) {
       case 0:
           sprintf(suffix, "B");
           break;
       case 1:
       case 2:
       case 3:
       case 4:
       case 5:
           sprintf(suffix, "%d", mycfg[MY_FOX_IDX]);
           break;
       case 6:
           sprintf(suffix, "S");
           break;
       case 9:
       case 10:
       case 11:
       case 12:
       case 13:
           sprintf(suffix, "%d", mycfg[MY_FOX_IDX]-8);
           break;
       default:
           sprintf(suffix, "%d", mycfg[MY_FOX_IDX]);
   }
   sprintf(hname, "%s%s", HOSTNAME, suffix);
   wifi_set_event_handler_cb(event_handler);
   wifi_station_set_hostname(hname);
   wifi_station_connect();
}
