#ifndef USER_UDP_H
#define USER_UDP_H

#define UDP_LOCAL_PORT 50001
#define UDP_REMOTE_PORT 50001
#define UDP_PAYLOAD_SZ ( ( MYCFG_SIZE ) + 2 ) /* LEADER mycfg[] TRAILER */

#define UDP_CMD_C 'C' /* configuration report */

void udp_init(void);
void udp_send_update(uint8);

#endif
