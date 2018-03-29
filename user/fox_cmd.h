#ifndef USER_CMD_H
#define USER_CMD_H

#include "config.h"

#define CMD_BUFSIZE 255U
// Should be not less than (in ms):
//   (amount_of_tx_bytes + amount_of_rx_bytes) * 2 * 10 / UART_SPEED
#define CMD_TIMEOUT     1000

#define CMD_T_TXSZ 10
#define CMD_B_TXSZ 4
#define CMD_W_TXSZ 8

extern uint8 cmd_tx_buffer[CMD_BUFSIZE];
extern uint8 cmd_rx_buffer[CMD_BUFSIZE];

enum msg_id {
  REQ_READCFG = 1,
  REP_SENDCFG,
  CMD_WRITEBASIC,
  REP_WRITEBASIC,
  CMD_WRITEXPERT,
  REP_WRITEXPERT,
  CMD_TIMESET,
  REP_TIMESET,
  REQ_TUNING,
  REP_TUNING,   /* report i_ANT array */ 
  REP_TUNMAX,   /* report i_ANT maximum step# and ADC val */
  REQ_BLINC,    /*  request backlash increment */
  REQ_BLDEC,
  REP_BLVAL     /* backlash val report */
};

void send_uart_cmd(uint8);
void ICACHE_FLASH_ATTR rxUartCb(char *, int);

#endif
