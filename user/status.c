#include "status.h"
#include "debug.h"

#include <osapi.h>
#include <gpio.h>

static ETSTimer t;
static bool _on;
static uint16_t _onms;
static uint16_t _offms;
static uint8_t _repeat;
static status_cb _cb = NULL;

static void ICACHE_FLASH_ATTR
status_toggle(void *a) {
    _on = !_on;
    LED_SET(_on);

    os_timer_disarm(&t);
    if ((_repeat > 0) && (!_on)) {
        if (--_repeat == 0) {
            if (_cb != NULL) {
                _cb();
            }
            return;
        };
        
    }
	os_timer_setfn(&t, (os_timer_func_t*)status_toggle, NULL);
    os_timer_arm(&t, _on? _onms: _offms, 0);
}


void ICACHE_FLASH_ATTR
status_update(uint16_t onms, uint16_t offms, uint8_t repeat, 
        status_cb cb) { 
    
    _on = false;
    _onms = onms;
    _offms = offms;
    _repeat = repeat;
    _cb = cb;

    status_toggle(NULL);
}


void ICACHE_FLASH_ATTR
status_stop() {
    os_timer_disarm(&t);
}


ICACHE_FLASH_ATTR
void status_init() {
	// LED
	PIN_FUNC_SELECT(LED_MUX, LED_FUNC);
    LED_SET(ON);
}

