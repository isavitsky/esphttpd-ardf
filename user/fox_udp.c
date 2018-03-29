#include <esp8266.h>
#include <user_interface.h>
#include <ip_addr.h>
#include "fox_udp.h"
#include "user_main.h"
#include "uart.h"
#include "fox_cmd.h"
#include "fox_cgi.h"
#include "fox_wifi.h"

static struct espconn bc_conn;
static esp_udp bc_udp;
static uint8 rmip[4] = { 255, 255, 255, 255 };

// receive callback
static void ICACHE_FLASH_ATTR udp_recv(void *arg, char *pusrdata, unsigned short length) {
  char *t;
  uint8 i, x;

  /* Each command packet consists of:
     t[0] = 'COMMAND'
     t[…] = '… some data'
     t[last] = 'COMMAND'
   */

  t = pusrdata;
#ifdef DEBUG
  os_printf("   * %c UDP command RXed\n", t[0]);
#endif
  switch (t[0]) {
    case UDP_CMD_C:
        if ( t[length - 1] == UDP_CMD_C ) {
#ifdef DEBUG
          os_printf(" OK\n");
#endif
           // check fox_id for correctness
           x = t[MY_FOX_IDX+1];
           if ( x >= 0 && x < FOXNUM && foxes[x][0] != LOCAL_TAG ) {
              foxes[x][0] = 9; // reset the foxTimerCb counter
              for ( i = 1; i < MYCFG_SIZE; i++ ) {
                 foxes[x][i] = t[i+1];
              }
           }
           if (t[MY_TSI_IDX+1] > mycfg[MY_TSI_IDX]) {
            mycfg[MY_SDN_IDX] = t[MY_SDN_IDX+1];
            mycfg[MY_CSS_IDX] = t[MY_CSS_IDX+1];
            mycfg[MY_CMM_IDX] = t[MY_CMM_IDX+1];
            mycfg[MY_CHH_IDX] = t[MY_CHH_IDX+1];
            mycfg[MY_CDD_IDX] = t[MY_CDD_IDX+1];
            mycfg[MY_CMN_IDX] = t[MY_CMN_IDX+1];
            mycfg[MY_CYY_IDX] = t[MY_CYY_IDX+1];
            mycfg[MY_SMM_IDX] = t[MY_SMM_IDX+1];
            mycfg[MY_SHH_IDX] = t[MY_SHH_IDX+1];
            mycfg[MY_SDD_IDX] = t[MY_SDD_IDX+1];
            mycfg[MY_TSI_IDX] = t[MY_TSI_IDX+1];

            for ( uint8 i=0; i<CMD_T_TXSZ; i++) {
                cmd_tx_buffer[i] = t[i+3];
            }
            send_uart_cmd(CMD_TIMESET);
           }
        }
#ifdef DEBUG
        else
          os_printf(" ERR\n");
#endif
        break;
  }
}

void ICACHE_FLASH_ATTR udp_send_update(uint8 cmd) {
  uint8 udp_payload[UDP_PAYLOAD_SZ];
  uint8 i;

  udp_payload[0] = cmd; // Leader
  udp_payload[UDP_PAYLOAD_SZ - 1] = cmd; // Trailer

  for (i = 1; i <= MYCFG_SIZE; i++) udp_payload[i] = mycfg[i-1];

  bc_udp.remote_ip[0] = rmip[0];
  bc_udp.remote_ip[1] = rmip[1];
  bc_udp.remote_ip[2] = rmip[2];
  bc_udp.remote_ip[3] = rmip[3];
  bc_udp.remote_port=UDP_REMOTE_PORT;
   
#ifdef DEBUG2
  os_printf("*** Sending UDP update: %c\n", cmd);
#endif
  if (wifi_station_get_connect_status() == STATION_GOT_IP) {
    espconn_sendto(&bc_conn, udp_payload, UDP_PAYLOAD_SZ);
  }
}

// setup the udp port
void ICACHE_FLASH_ATTR udp_init() {
  // Clear esp connection variables
  memset(&bc_conn, 0, sizeof(bc_conn));
  memset(&bc_udp, 0, sizeof(bc_udp));

  bc_conn.type = ESPCONN_UDP;
  bc_conn.state = ESPCONN_NONE;
  bc_conn.proto.udp = &bc_udp;
  bc_conn.reverse = NULL;
  bc_udp.remote_ip[0] = rmip[0];
  bc_udp.remote_ip[1] = rmip[1];
  bc_udp.remote_ip[2] = rmip[2];
  bc_udp.remote_ip[3] = rmip[3];
  bc_udp.local_port=UDP_LOCAL_PORT;
  bc_udp.remote_port=UDP_REMOTE_PORT;
  espconn_delete(&bc_conn);
  espconn_create(&bc_conn);
  espconn_regist_recvcb(&bc_conn, udp_recv);
}
