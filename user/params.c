#include "partition.h"
#include "params.h"
#include "common.h"

#include <user_interface.h>


#define PARAMS_SECTOR (USER_PARTITION_PARAMS_ADDR / SECT_SIZE) 
#define PARAMS_MAGIC '!'


ICACHE_FLASH_ATTR 
bool params_save(struct params* params) {
	params->magic = PARAMS_MAGIC;
    // system_soft_wdt_feed();
    // if (!spi_flash_erase_protect_disable()) {
    //     ERROR("spi_flash_erase_protect_disable() Error!");
    //     return false;
    // }
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
	params->ap_psk[0] = 0;
	params->station_ssid[0] = 0;
	params->station_psk[0] = 0;
	return params_save(params);
}
