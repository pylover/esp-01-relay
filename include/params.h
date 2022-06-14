#ifndef PARAMS_H
#define PARAMS_H

#include "debug.h"
#include "partition.h"
#include "user_config.h"

#include <c_types.h>
#include <user_interface.h>



#define PARAMS_PRINT(p) INFO(CR"%s.%s, ssid: %s psk: %s ap-psk: %s", \
			p.zone, \
			p.name, \
			p.station_ssid, \
			p.station_psk, \
			p.ap_psk \
		)

#define PARAMS_ZONE_MAXLEN  32
#define PARAMS_NAME_MAXLEN  32


struct params {
	 char zone[PARAMS_ZONE_MAXLEN];
	 char name[PARAMS_NAME_MAXLEN];
	 char ap_psk[32];
	 char station_ssid[32];
	 char station_psk[32];
	 char magic;
};


bool params_save(struct params* params);
bool params_load(struct params* params);
bool params_defaults(struct params* params);

#endif

