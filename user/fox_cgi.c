/*
Cgi routines as used by the tests in the html/test subdirectory.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "cgiwebsocket.h"
#include "uart.h"
#include "config.h"
#include "user_main.h"
#include "fox_cgi.h"
#include "fox_cmd.h"
#include "fox_wifi.h"
#include "fox_udp.h"

#include "cJson.h"

#define HIBYTE(x) ((char*)(&(x)))[1]
#define LOBYTE(x) ((char*)(&(x)))[0]

#define FOXSYMB_SIZE 3

LOCAL os_timer_t wifi_timer;
LOCAL uint8 mode;
LOCAL struct station_config stationConf;

typedef struct {
	int len;
	int sendPos;
} CgiState;

int ICACHE_FLASH_ATTR cgiFoxCfgGet(HttpdConnData *connData) {
    cJSON *cfg_json;
    char *json_str;
    char tmp[3];

    if (connData->conn==NULL) {
#ifdef DEBUG2
        os_printf("                        cgiFoxCfgGet():NULL\n");
#endif
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    if (connData->requestType!=HTTPD_METHOD_GET) {
#ifdef DEBUG2
        os_printf("                        cgiFoxCfgGet():NOMETHOD\n");
#endif
        //Sorry, we only accept GET requests.
        httpdStartResponse(connData, 406);  //http error code 'unacceptable'
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }

    if ( ! flag.uart_req_for_cgi_sent ) {
#ifdef DEBUG2
        os_printf("                        cgiFoxCfgGet():SENDCMD\n");
#endif
        flag.uart_rep_for_cgi_rcvd = false;
        send_uart_cmd(REQ_READCFG);
        flag.uart_req_for_cgi_sent = true;
        //Generate the header
        //We want the header to start with HTTP code 200, which means the document is found.
        httpdStartResponse(connData, 200); 
        //We are going to send some HTML.
        httpdHeader(connData, "Content-Type", "application/json");
        //No more headers.
        httpdEndHeaders(connData);
        return HTTPD_CGI_MORE;
    }

    if ( flag.uart_rep_for_cgi_rcvd ) {
#ifdef DEBUG2
        os_printf("                        cgiFoxCfgGet():GOTREPLY\n");
#endif
        cfg_json = cJSON_CreateObject();

        os_sprintf(tmp, "%02x", cmd_rx_buffer[0]);
        cJSON_AddStringToObject(cfg_json, "hwver", tmp);
        os_sprintf(tmp, "%02x", cmd_rx_buffer[1]);
        cJSON_AddStringToObject(cfg_json, "swver", tmp);
        cJSON_AddNumberToObject(cfg_json, "foxid", cmd_rx_buffer[2]);
        cJSON_AddNumberToObject(cfg_json, "numfoxes", cmd_rx_buffer[3]);
        cJSON_AddNumberToObject(cfg_json, "seancelen", cmd_rx_buffer[4]);
        cJSON_AddNumberToObject(cfg_json, "cwspeed", cmd_rx_buffer[5]);
        // add TX_BASE_FREQ to freq
        cJSON_AddNumberToObject(cfg_json, "txfreq", TX_BASE_FREQ+cmd_rx_buffer[6]);
        // compute the correct value for tone in range -500..+500
        // where -500 is 0 and +500 is 255
        cJSON_AddNumberToObject(cfg_json, "tone", cmd_rx_buffer[7] * 4 - 500);
        cJSON_AddNumberToObject(cfg_json, "accucap", cmd_rx_buffer[8] * 256 + cmd_rx_buffer[9]);
        cJSON_AddNumberToObject(cfg_json, "totaltime", cmd_rx_buffer[10] * 256 + cmd_rx_buffer[11]);
        cJSON_AddNumberToObject(cfg_json, "dodidx", cmd_rx_buffer[12] * 256 + cmd_rx_buffer[13]);

        // update current config
        mycfg[MY_FOX_IDX]=cmd_rx_buffer[2]; // foxid
        mycfg[MY_NRF_IDX]=cmd_rx_buffer[3]; // numfoxes
        mycfg[MY_SDN_IDX]=cmd_rx_buffer[4]; // seancelen
        
        json_str = cJSON_PrintUnformatted(cfg_json);
#ifdef DEBUG2
        os_printf("JSON: %s\n", json_str);
#endif
        httpdSend(connData, json_str, os_strlen(json_str));
        os_free(json_str);
        cJSON_Delete(cfg_json);
        flag.uart_req_for_cgi_sent = false;
        flag.uart_rep_for_cgi_rcvd = false;
        return HTTPD_CGI_DONE;
    }
#ifdef DEBUG2
        os_printf("                        cgiFoxCfgGet():GETMORE_DEFAULT\n");
#endif
        save_connData = connData;
        return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR cgiFoxCfgSave(HttpdConnData *connData) {
    uint16 tmp;

    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    if (connData->requestType!=HTTPD_METHOD_POST) {
        //Sorry, we only accept POST requests.
        httpdStartResponse(connData, 406);  //http error code 'unacceptable'
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }

    if ( ! flag.uart_req_for_cgi_sent ) {
        //Generate the header
        //We want the header to start with HTTP code 200, which means the document is found.
        httpdStartResponse(connData, 200); 
        //We are going to send some HTML.
        httpdHeader(connData, "Content-Type", "application/json");
        //No more headers.
        httpdEndHeaders(connData);

        cJSON * root = cJSON_Parse(connData->post->buff);
#ifdef DEBUG
        os_printf("JSON:%s\n", connData->post->buff);
#endif
        if(root==NULL) return HTTPD_CGI_DONE;

        cJSON * foxid = cJSON_GetObjectItem(root, "foxid");
        tmp = foxid->valueint;
        cmd_tx_buffer[0] = (uint8)tmp;

        cJSON * numfoxes = cJSON_GetObjectItem(root, "numfoxes");
        tmp = numfoxes->valueint;
        cmd_tx_buffer[1] = (uint8)tmp;

        cJSON * seancelen = cJSON_GetObjectItem(root, "seancelen");
        tmp = seancelen->valueint;
        cmd_tx_buffer[2] = (uint8)tmp;

        cJSON * txfreq = cJSON_GetObjectItem(root, "txfreq");
        tmp = txfreq->valueint - TX_BASE_FREQ;
        cmd_tx_buffer[3] = (uint8)tmp;

        cJSON_Delete(root);

        flag.uart_rep_for_cgi_rcvd = false;
        send_uart_cmd(CMD_WRITEBASIC);
        flag.uart_req_for_cgi_sent = true;
        return HTTPD_CGI_MORE;
    }

    if ( flag.uart_rep_for_cgi_rcvd ) {
        cJSON * root = cJSON_CreateObject();
        if (cmd_rx_buffer[0] == OK) {
                cJSON_AddStringToObject(root, "status", "ok");
        } else {
                cJSON_AddStringToObject(root, "status", "cfg_err");
        }

        char * json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        httpdSend(connData, json_str, os_strlen(json_str));
        os_free(json_str);
        flag.uart_req_for_cgi_sent = false;
        flag.uart_rep_for_cgi_rcvd = false;
        return HTTPD_CGI_DONE;
    }
    // for httpdContinue():
    save_connData = connData;
    return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR cgiFoxCfgSaveXpert(HttpdConnData *connData) {
    cJSON * root;
    cJSON * tmpjs;
    char *json_str;
    uint16 tmp;

    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    if (connData->requestType!=HTTPD_METHOD_POST) {
        //Sorry, we only accept POST requests.
        httpdStartResponse(connData, 406);  //http error code 'unacceptable'
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }

    if ( ! flag.uart_req_for_cgi_sent ) {
        //Generate the header
        //We want the header to start with HTTP code 200, which means the document is found.
        httpdStartResponse(connData, 200); 
        //We are going to send some HTML.
        httpdHeader(connData, "Content-Type", "application/json");
        //No more headers.
        httpdEndHeaders(connData);

        root = cJSON_Parse(connData->post->buff);
#ifdef DEBUG
        os_printf("JSON:%s\n", connData->post->buff);
#endif
        if(root==NULL) return HTTPD_CGI_DONE;

        tmpjs = cJSON_GetObjectItem(root, "foxid");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint;
        cmd_tx_buffer[0] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "numfoxes");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint;
        cmd_tx_buffer[1] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "seancelen");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint;
        cmd_tx_buffer[2] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "cwspeed");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint;
        cmd_tx_buffer[3] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "txfreq");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint - TX_BASE_FREQ;
        cmd_tx_buffer[4] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "tone");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        // Allowed tone correction values: -500..+500 convert to char
        tmp = (tmpjs->valueint+500)/4;
        cmd_tx_buffer[5] = (uint8)tmp;

        tmpjs = cJSON_GetObjectItem(root, "accucap");
        if(tmpjs==NULL) return HTTPD_CGI_DONE;
        tmp = tmpjs->valueint;
        cmd_tx_buffer[6] = HIBYTE(tmp);
        cmd_tx_buffer[7] = LOBYTE(tmp);

        cJSON_Delete(root);
        //if ( tmpjs != NULL ) cJSON_Delete(tmpjs);

        flag.uart_rep_for_cgi_rcvd = false;
        send_uart_cmd(CMD_WRITEXPERT);
        flag.uart_req_for_cgi_sent = true;
        return HTTPD_CGI_MORE;
    }

    if ( flag.uart_rep_for_cgi_rcvd ) {
        root = cJSON_CreateObject();
#ifdef DEBUG2
        os_printf("   ** cmd_rx_buffer[0]:%x\n", cmd_rx_buffer[0]);
#endif
        if (cmd_rx_buffer[0] == OK) {
            cJSON_AddStringToObject(root, "status", "ok");
        } else {
            cJSON_AddStringToObject(root, "status", "cfg_err");
        }

        json_str = cJSON_PrintUnformatted(root);
        httpdSend(connData, json_str, os_strlen(json_str));
        os_free(json_str);
        cJSON_Delete(root);
        flag.uart_req_for_cgi_sent = false;
        flag.uart_rep_for_cgi_rcvd = false;
        return HTTPD_CGI_DONE;
    }
    // for httpdContinue():
    save_connData = connData;
    return HTTPD_CGI_MORE;
 }

void ICACHE_FLASH_ATTR wifiTimerCb(void *arg) {
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
    wifi_set_opmode(mode);
    if (mode == STATION_MODE || mode == STATIONAP_MODE) {
        wifi_station_disconnect();
        wifi_station_set_hostname(hname);
        wifi_station_set_config(&stationConf);
        wifi_station_connect();
    }

}

int ICACHE_FLASH_ATTR cgiWiFiSetCfg(HttpdConnData *connData) {
    char buff[128];
    int len, l;
    CgiState *state=(CgiState*)connData->cgiData;
    char ssid[32];
    char password[64];

    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        if(state) free(state);
        return HTTPD_CGI_DONE;
    }

    if (state==NULL) {
        //First call
        state=malloc(sizeof(CgiState));
        memset(state, 0, sizeof(state));
        connData->cgiData=state;
    }

    if (connData->requestType!=HTTPD_METHOD_POST) {
        //Sorry, we only accept POST requests.
        httpdStartResponse(connData, 406);  //http error code 'unacceptable'
        httpdEndHeaders(connData);
        return HTTPD_CGI_DONE;
    }  else {
        if (connData->post->len!=connData->post->received) {
            //Still receiving data. Ignore this.
            return HTTPD_CGI_MORE;
        } else {
            httpdStartResponse(connData, 200);
            httpdHeader(connData, "content-type", "text/html");
            httpdEndHeaders(connData);
            len=httpdFindArg(connData->post->buff, "mode", buff, sizeof(buff));
            mode=atoi(buff);
            if(len == 0) {
                l=sprintf(buff, "Error: incorrect mode.<br>");
                httpdSend(connData, buff, l);
                if(state) free(state);
                return HTTPD_CGI_DONE;
            }
            switch (mode) {
                case 1: // STATION_MODE
                case 2: // SOFTAP_MODE
                case 3: // STATIONAP_MODE
                    break;
                default:
                    mode=STATIONAP_MODE; // STA+AP
            }
            len=httpdFindArg(connData->post->buff, "ssid", ssid, sizeof(ssid));
            if(mode == STATION_MODE || mode == STATIONAP_MODE) {
                if(len==0) {
                    l=sprintf(buff, "Error: no SSID set.<br>");
                    httpdSend(connData, buff, l);
                    if(state) free(state);
                    return HTTPD_CGI_DONE;
                }
                os_printf("*** ssid: \"%s\"\r\n", ssid);
                stationConf.bssid_set = 0; //need  not check   MAC address of  AP
                os_memcpy(&stationConf.ssid, ssid, 32);
            }
            len=httpdFindArg(connData->post->buff, "pass", password, sizeof(password));
            if(mode == STATION_MODE || mode == STATIONAP_MODE) {
                if(len==0) {
                    l=sprintf(buff, "Error: no password set.<br>");
                    httpdSend(connData, buff, l);
                    if(state) free(state);
                    return HTTPD_CGI_DONE;
                }
                os_printf("*** pass: \"%s\"\r\n", password);
                os_memcpy(&stationConf.password, password, 64);
            }
            l=sprintf(buff, "OK<br>");
            httpdSend(connData, buff, l);
            os_timer_disarm(&wifi_timer);
            os_timer_setfn(&wifi_timer, (os_timer_func_t *)wifiTimerCb, NULL);
            os_timer_arm(&wifi_timer, CWIFI_TIMEOUT, false);
            if(state) free(state);
            return HTTPD_CGI_DONE;
        }

    }
}

//Broadcast the uptime in seconds over connected websockets
void ICACHE_FLASH_ATTR wsTimerCb(void *arg) {
    cJSON *json;
    cJSON *json_payload;
    cJSON *json_fxdata;
    char *json_str;
    char tmp[32];
    uint8 i;

    // update structure: {"msg_type":"U","payload":{"0":{"type":-1,"tsync":0,"addr":"192.168.4.2","numfoxes":0,accucap":0,"dodidx":0},"1": {} … etc }}}
    json = cJSON_CreateObject();
    json_payload = cJSON_CreateObject();
    os_sprintf(tmp, "%c", WS_CMD_U);
    cJSON_AddStringToObject(json, "msg_type", tmp);
    for(i=0; i<FOXNUM; i++) {
        json_fxdata = cJSON_CreateObject();
        if ( i == mycfg[MY_FOX_IDX] ) {
            cJSON_AddNumberToObject(json_fxdata, "type", 1); // MODE_MASTER (currently not used)
            cJSON_AddNumberToObject(json_fxdata, "numfoxes", mycfg[MY_NRF_IDX]);
            cJSON_AddNumberToObject(json_fxdata, "accucap", ((mycfg[MY_ACH_IDX] << 8) | mycfg[MY_ACL_IDX]));
            cJSON_AddNumberToObject(json_fxdata, "dodidx", ((mycfg[MY_DDH_IDX] << 8) | mycfg[MY_DDL_IDX]));
        } else if ( foxes[i][0] > 0 ) {
            cJSON_AddNumberToObject(json_fxdata, "type", 0); // MODE_SLAVE (currently not used)
            os_sprintf(tmp, "%d.%d.%d.%d", foxes[i][MY_IP4_IDX], foxes[i][MY_IP3_IDX], foxes[i][MY_IP2_IDX], foxes[i][MY_IP1_IDX]);
            cJSON_AddStringToObject(json_fxdata, "addr", tmp);
            cJSON_AddNumberToObject(json_fxdata, "numfoxes", foxes[i][MY_NRF_IDX]);
            cJSON_AddNumberToObject(json_fxdata, "accucap", ((foxes[i][MY_ACH_IDX] << 8) | foxes[i][MY_ACL_IDX]));
            cJSON_AddNumberToObject(json_fxdata, "dodidx", ((foxes[i][MY_DDH_IDX] << 8) | foxes[i][MY_DDL_IDX]));
        } else {
            cJSON_AddNumberToObject(json_fxdata, "type", -1); // inactive
        }
        if (foxes[i][MY_TSI_IDX] == mycfg[MY_TSI_IDX]) {
            cJSON_AddNumberToObject(json_fxdata, "tsync", 1); // In sync
        } else {
            cJSON_AddNumberToObject(json_fxdata, "tsync", 0); // Not in sync
        }
        os_sprintf(tmp, "%d", i);
        cJSON_AddItemToObject(json_payload, tmp, json_fxdata);
    }
    cJSON_AddItemToObject(json, "payload", json_payload);
    json_str = cJSON_PrintUnformatted(json);

    cgiWebsockBroadcast(WS_RESOURCE_UPD, json_str, os_strlen(json_str), WEBSOCK_FLAG_NONE);
#ifdef DEBUG2
    os_printf("JSON: %s\n", json_str);
#endif
    os_free(json_str);
    cJSON_Delete(json);
}

//On reception of a message, send "You sent: " plus whatever the other side sent
void ICACHE_FLASH_ATTR myWsRecv(Websock *ws, char *data, int len, int flags) {
    cJSON * item;
    uint16 tmp;

#ifdef DEBUG
    os_printf("WS RECV: %s\n", data);
#endif
    cJSON * root = cJSON_Parse(data);
    if(root==NULL) return;
    cJSON * payload = cJSON_GetObjectItem(root, "payload");
    cJSON * msgt = cJSON_GetObjectItem(root, "msg_type");
    //cJSON_Delete(tmpjs);

    switch (msgt->valuestring[0]) {
        case WS_CMD_T:
#ifdef DEBUG
            os_printf("WS: T Received\n");
#endif
            item = cJSON_GetObjectItem(payload, "cur_ss");
            tmp = item->valueint;
            mycfg[MY_CSS_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "cur_mm");
            tmp = item->valueint;
            mycfg[MY_CMM_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "cur_hh");
            tmp = item->valueint;
            mycfg[MY_CHH_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "cur_dd");
            tmp = item->valueint;
            mycfg[MY_CDD_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "cur_mn");
            tmp = item->valueint;
            mycfg[MY_CMN_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "cur_yy");
            tmp = item->valueint;
            mycfg[MY_CYY_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "start_mm");
            tmp = item->valueint;
            mycfg[MY_SMM_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "start_hh");
            tmp = item->valueint;
            mycfg[MY_SHH_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "start_dd");
            tmp = item->valueint;
            mycfg[MY_SDD_IDX] = (uint8)tmp;

            item = cJSON_GetObjectItem(payload, "seancelen");
            tmp = item->valueint;
            mycfg[MY_SDN_IDX] = (uint8)tmp;

            cJSON_Delete(root);

            mycfg[MY_TSI_IDX]++;
            rtc_init();
            for ( uint8 i=0; i<CMD_T_TXSZ; i++) {
                cmd_tx_buffer[i] = mycfg[i+2];
            }
            send_uart_cmd(CMD_TIMESET);
            break;
    }
}

void ICACHE_FLASH_ATTR myWsRecvIant(Websock *ws, char *data, int len, int flags) {
    cJSON * root;
    cJSON * msgt;
    cJSON * payload;
    int8 val;

#ifdef DEBUG
    os_printf("WS RECV: %s\n", data);
#endif
    root = cJSON_Parse(data);
    if(root==NULL) return;
    msgt = cJSON_GetObjectItem(root, "msg_type");
    //cJSON_Delete(tmpjs);

    switch (msgt->valuestring[0]) {
        case WS_CMD_I:
#ifdef DEBUG
            os_printf("WS: I Received\n");
#endif
            send_uart_cmd(REQ_TUNING);
            break;
        case WS_REQ_B:
            payload = cJSON_GetObjectItem(root, "payload");
            val = payload->valueint;
            if (val > 0)
            {
                send_uart_cmd(REQ_BLINC);
            } else
            {
                send_uart_cmd(REQ_BLDEC);
            }
    }
    cJSON_Delete(root);
}

//Websocket connected. Install reception handler and send welcome message.
void ICACHE_FLASH_ATTR myWsConnect(Websock *ws) {
        ws->recvCb=myWsRecv;
#ifdef DEBUG
        os_printf("WS: CONNECT\n");
#endif
}

void ICACHE_FLASH_ATTR myWsConnectIant(Websock *ws) {
        ws->recvCb=myWsRecvIant;
#ifdef DEBUG
        os_printf("WS: CONNECT\n");
#endif
}

void wsReply(int8 id, uint8 status) {
    cJSON *root;
    char *json_str;

    switch (id) {
        case REP_TIMESET:
        root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "msg_type", WS_REP_T);
        cJSON_AddNumberToObject(root, "payload", status);
        json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
#ifdef DEBUG3
        os_printf("Broadcast: %s", json_str);
#endif
        cgiWebsockBroadcast(WS_RESOURCE_UPD, json_str, os_strlen(json_str), WEBSOCK_FLAG_NONE);
#ifdef DEBUG3
        os_printf(" ... Done\n");
#endif
        os_free(json_str);
        break;
    }
}

void ICACHE_FLASH_ATTR wsReplyIant(int8 id, uint8 *data, uint8 len) {
    cJSON *root;
    cJSON *jdata;
    char *json_str;
    uint8 i;

    switch (id) {
        case REP_TUNING:
            root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "msg_type", WS_REP_I);
            jdata = cJSON_CreateArray();
            for (i=0;i<len;i++)
                cJSON_AddItemToArray(jdata, cJSON_CreateNumber(data[i]));
            cJSON_AddItemToObject(root, "payload", jdata);
            json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
#ifdef DEBUG
        os_printf("Broadcast: %s", json_str);
#endif
            cgiWebsockBroadcast(WS_RESOURCE_IAN, json_str, os_strlen(json_str), WEBSOCK_FLAG_NONE);
#ifdef DEBUG
        os_printf(" ... Done\n");
#endif
            os_free(json_str);
            break;
        case REP_TUNMAX:
            root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "msg_type", WS_REP_J);
            jdata = cJSON_CreateArray();
            for (i=0;i<len;i+=2)
                cJSON_AddItemToArray(jdata, cJSON_CreateNumber((sint16)(data[i]<<8 | data[i+1])));
            cJSON_AddItemToObject(root, "payload", jdata);
            json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            cgiWebsockBroadcast(WS_RESOURCE_IAN, json_str, os_strlen(json_str), WEBSOCK_FLAG_NONE);
            os_free(json_str);
            break;
        case REP_BLVAL:
            root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "msg_type", WS_REP_B);
            cJSON_AddNumberToObject(root, "payload", (int8)data[0]);
            json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            cgiWebsockBroadcast(WS_RESOURCE_IAN, json_str, os_strlen(json_str), WEBSOCK_FLAG_NONE);
            os_free(json_str);
    }
}
