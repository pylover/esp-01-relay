/*
 * wifi.h
 *
 */

#ifndef USER_WIFI_H_
#define USER_WIFI_H_

#include "params.h"
//#include <osapi.h>
//#include <espconn.h>
//#include <mem.h>
//#include <ets_sys.h>
//#include <c_types.h>
#include <user_interface.h>
#include <os_type.h>


#define WIFI_SOFTAP_CHANNEL		7


typedef void (*wificb)(uint8_t);

struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};

void wifi_start(struct params *params, wificb cb);
void wifi_ap_start();
void wifi_ap_stop();

#endif /* USER_WIFI_H_ */
