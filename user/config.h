#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG
//#define DEBUG2
//#define DEBUG3
//#define SHOW_HEAP_USE

#define FOXC_TIMER	1000
#define UPDATE_TIMER	1000
#define HOSTNAME	"FOX_"
#define IO_DELAY	1500
#define TX_BASE_FREQ 3500
#define UART0_SPEED 38400
//#define UART1_SPEED 921600
#define UART1_SPEED 74880
#define FOXNUM		14 /* B, 1—5, S, N => (B) F1—F5 */
#define LOCAL_TAG	0xFF
#define MYCFG_SIZE	21

#define OK 1
#define ERR 0

/*  mycfg[] structure: */
#define MY_FOX_IDX 0   /* fox id (0..11) or timeout counter */
#define MY_NRF_IDX 1   /* number of foxes (1..5) */
#define MY_SDN_IDX 2   /* seance duration (12, 24, 30, 60, etc) */
#define MY_CSS_IDX 3   /* Current seconds */
#define MY_CMM_IDX 4   /* Current minutes */
#define MY_CHH_IDX 5   /* Current hours */
#define MY_CDD_IDX 6   /* Current day */
#define MY_CMN_IDX 7   /* Current month */
#define MY_CYY_IDX 8   /* Current year */
#define MY_SMM_IDX 9   /* Start minute */
#define MY_SHH_IDX 10  /* Start hour */
#define MY_SDD_IDX 11  /* Start day */
#define MY_IP4_IDX 12  /* Most significant IP octet */
#define MY_IP3_IDX 13
#define MY_IP2_IDX 14
#define MY_IP1_IDX 15  /* Least significant IP octet */
#define MY_TSI_IDX 16  /* Time sequence IDX */
#define MY_ACH_IDX 17  /* Accumulator calibration point number */
#define MY_ACL_IDX 18
#define MY_DDH_IDX 19  /* Depth of dosachrge index */
#define MY_DDL_IDX 20

#endif
