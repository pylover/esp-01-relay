#ifndef _H_STATUS_
#define _H_STATUS_

#include "user_config.h"

#include <c_types.h>

// LED
#define LED_MUX		        PERIPHS_IO_MUX_GPIO2_U	
#define LED_NUM			    2
#define LED_FUNC		    FUNC_GPIO2

#define ON	true
#define OFF false
#define LED_SET(on)	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_NUM), !on)
#define INFINITE    0

typedef void (*status_cb)(void);
 
void status_update(uint16_t on_ms, uint16_t off_ms, uint8_t repeat, 
        status_cb cb);

void status_init();

#endif
