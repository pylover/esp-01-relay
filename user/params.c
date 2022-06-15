#include "partition.h"
#include "params.h"
#include "common.h"

#include <user_interface.h>


bool params_save(struct params* params) {
	params->magic = PARAMS_MAGIC;
	return system_param_save_with_protect(PARAMS_SECTOR, params, 
			sizeof(struct params));
}


ICACHE_FLASH_ATTR 
bool params_load(struct params* params) {
	bool ok = system_param_load(PARAMS_SECTOR, 0,
			params, sizeof(struct params));
	return ok && params->magic == PARAMS_MAGIC;
}


bool ICACHE_FLASH_ATTR 
params_defaults(struct params* params) {
    os_memset(params, 0, sizeof(struct params));
	os_sprintf(params->zone, PARAMS_DEFAULT_ZONE);
	os_sprintf(params->name, PARAMS_DEFAULT_NAME);
	os_sprintf(params->ap_psk, PARAMS_DEFAULT_AP_PSK);
	// params->ap_psk[0] = 0;

	os_sprintf(params->station_ssid, PARAMS_DEFAULT_STATION_SSID);
	// params->station_ssid[0] = 0;

	os_sprintf(params->station_psk, PARAMS_DEFAULT_STATION_PSK);
	//params->station_psk[0] = 0;
	return params_save(params);
}
