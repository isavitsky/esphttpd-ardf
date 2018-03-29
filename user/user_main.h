#ifndef USER_MAIN_H
#define USER_MAIN_H

#include "httpd.h"
#include "config.h"

struct rtctime_t {
  uint8 ss;
  uint8 mm;
  uint8 hh;
  uint8 dd;
  uint8 mn;
  uint8 yy;
};

struct flag_t {
  uint8 got_mycfg               : 1;
  uint8 uart_cmd_issued         : 1;
  uint8 uart_cmd_gotreply       : 1;
  uint8 uart_req_for_cgi_sent	: 1;
  uint8 uart_rep_for_cgi_rcvd	: 1;
};

HttpdConnData *save_connData;
extern struct min_context min_ctx;

/*
    foxes[][] structure:
 f
 o
 x
 #
 |
 v
[ ][0 ] - update timeout counter (0..9)
[ ][1 ] - number of foxes (1..5)
[ ][2 ] - seance duration (12, 24, 30, 60, etc...)
[ ][3 ] - current seconds
[ ][4 ] - current minutes
[ ][5 ] - current hours
[ ][6 ] - current day
[ ][7 ] - current month
[ ][8 ] - current year
[ ][9 ] - start minute
[ ][10] - start hour
[ ][11] - start day
[ ][12] - IP.4
[ ][13] - IP.3
[ ][14] - IP.2
[ ][15] - IP.1

*/
extern uint8 foxes[FOXNUM][MYCFG_SIZE];
extern uint8 mycfg[MYCFG_SIZE];
extern struct flag_t flag;
extern struct rtctime_t tm;

void init_fx(void);
void ICACHE_FLASH_ATTR foxcTimerCb(void *);
int myPassFn(HttpdConnData *, int, char *, int, char *, int);
uint16 min_tx_space(uint8);
void min_tx_byte(uint8, uint8);
void min_tx_start(uint8);
void min_tx_finished(uint8);
void min_application_handler(uint8, uint8 *, uint8, uint8);
void rtc_init(void);

#endif
