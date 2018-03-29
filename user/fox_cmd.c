#include <esp8266.h>
#include <user_interface.h>
#include "config.h"
#include "user_main.h"
#include "uart.h"
#include "min.h"
#include "fox_cmd.h"
//#include "fox_udp.h"
//#include "fox_wifi.h"
//#include "fox-cgi.h"

uint8 cmd_tx_buffer[CMD_BUFSIZE];
uint8 cmd_rx_buffer[CMD_BUFSIZE];


LOCAL os_timer_t cmd_timer;

void ICACHE_FLASH_ATTR cmd_timeout(void) {
    flag.uart_cmd_issued = false;
    if (!flag.got_mycfg) {
        send_uart_cmd(REQ_READCFG);
    }
    if ( flag.uart_req_for_cgi_sent ) {
        flag.uart_rep_for_cgi_rcvd = true;
        httpdContinue(save_connData);
    }
}

void ICACHE_FLASH_ATTR send_uart_cmd(uint8 cmd) {
    uint8 sz = 0;

    if ( flag.uart_cmd_issued ) { return; } // prevent data crruption
    flag.uart_cmd_issued = true;
    flag.uart_cmd_gotreply = false;

    switch ( cmd ) {
    case REQ_READCFG:
        sz = 0;
        break;
    case CMD_TIMESET:
        sz = CMD_T_TXSZ;
        break;
    case CMD_WRITEBASIC:
        sz = CMD_B_TXSZ;
        break;
    case CMD_WRITEXPERT:
        sz = CMD_W_TXSZ;
        break;
    }
#ifdef DEBUG
    os_printf("MIN frame sending: id: %x, length: %d, payload: ", cmd, sz);
    for (uint8 i=0; i<sz; i++) {
        os_printf("%02X ", cmd_tx_buffer[i]);
    }
    os_printf("\n");
#endif
    min_send_frame(&min_ctx, cmd, cmd_tx_buffer, sz);

    os_timer_disarm(&cmd_timer);
    os_timer_setfn(&cmd_timer, (os_timer_func_t *)cmd_timeout, NULL);
    os_timer_arm(&cmd_timer, CMD_TIMEOUT, false);
}

// callback with a buffer of characters that have arrived on the uart
void ICACHE_FLASH_ATTR rxUartCb(char *buf, int length) {
    min_poll(&min_ctx, (uint8_t *)buf, (uint8_t)length);
}
