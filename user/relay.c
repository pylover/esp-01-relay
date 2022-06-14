#include "relay.h"

#include <osapi.h>
#include <gpio.h>


static enum relay_status relay_status = RELAY_OFF;


ICACHE_FLASH_ATTR 
enum relay_status 
get_relay_status() {
   return relay_status;
}


ICACHE_FLASH_ATTR void
update_relay_status(enum relay_status s) {
  relay_status = s;
  GPIO_OUTPUT_SET(GPIO_ID_PIN(RELAY_NUM), s);
}


ICACHE_FLASH_ATTR void
toggle_relay_status() {
  if (relay_status == RELAY_ON) { 
    relay_status = RELAY_OFF;
  }
  else {
    relay_status = RELAY_ON;
  }
  GPIO_OUTPUT_SET(GPIO_ID_PIN(RELAY_NUM), relay_status);
}


ICACHE_FLASH_ATTR void
relay_init() {
  PIN_FUNC_SELECT(RELAY_MUX, RELAY_FUNC);
  GPIO_OUTPUT_SET(GPIO_ID_PIN(RELAY_NUM), relay_status);
}
