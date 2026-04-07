#ifndef __COMM_ADDR_H
#define __COMM_ADDR_H

#define PC_ADDR 0x01
#define LLC_ADDR 0x02
#define PFC_ADDR 0x03
#define APP_ADDR 0x04

#define DC_ADDR LLC_ADDR
#define AC_ADDR PFC_ADDR

#ifdef IS_AC
#define LOCAL_ADDR AC_ADDR
#endif

#ifdef IS_DC
#define LOCAL_ADDR DC_ADDR
#endif

#endif
