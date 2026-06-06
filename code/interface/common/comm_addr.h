#ifndef __COMM_ADDR_H
#define __COMM_ADDR_H

#define PC_ADDR 0x01
#define LLC_ADDR 0x02
#define PFC_ADDR 0x03
#define APP_ADDR 0x04

#ifndef HOST_ADDR
#define HOST_ADDR LLC_ADDR
#endif

#ifndef LOCAL_ADDR
#define LOCAL_ADDR HOST_ADDR
#endif

#endif
