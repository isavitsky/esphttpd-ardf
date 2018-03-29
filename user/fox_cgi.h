#ifndef CGI_FOX_H
#define CGI_FOX_H

#include "httpd.h"
#include "cgiwebsocket.h"

#define CWIFI_TIMEOUT 1000 /* timeout for wifiTimerCb() */

#define TSI_CL_OK "#8888FF" /* Colour id for correct Time sequence */
#define TSI_CL_ERR "#FF8888" /* Colour id for wrong Time seq */

#define WS_RESOURCE_UPD "/ws/upd"
#define WS_RESOURCE_IAN "/ws/iant"
#define WS_CMD_U 'U' /* Update command sent to client */
#define WS_CMD_T 'T' /* Time set command received from client */
#define WS_REP_T "t" /* Time set reply */
#define WS_CMD_I 'I' /* Do antenna tuning and collect the i_Ant data*/
#define WS_REP_I "i" /* --//-- */
#define WS_REP_J "j" /* report i_ANT maximum step# and ADC val */
#define WS_REQ_B 'B' /* Stepper backlash increment/decrement request */
#define WS_REP_B "b" /* report backlash value */

#define WS_TIMER 5000 /* Five seconds */

int cgiFoxCfgGet(HttpdConnData *connData);
int cgiFoxCfgGetMy(HttpdConnData *connData);
int cgiFoxCfgSave(HttpdConnData *connData);
int cgiFoxCfgSaveXpert(HttpdConnData *connData);
int cgiWiFiSetCfg(HttpdConnData *connData);
void ICACHE_FLASH_ATTR wsTimerCb(void *arg);
void ICACHE_FLASH_ATTR myWsConnect(Websock *ws);
void ICACHE_FLASH_ATTR myWsConnectIant(Websock *ws);
void ICACHE_FLASH_ATTR wsReply(int8 id, uint8 status);
void ICACHE_FLASH_ATTR wsReplyIant(int8 id, uint8 *data, uint8 len);

#endif
