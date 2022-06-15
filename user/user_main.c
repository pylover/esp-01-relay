// Internal 
#include "user_config.h"
#include "partition.h"
#include "wifi.h"
#include "params.h" 
#include "debug.h"
#include "status.h"
#include "uns.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>
#include <c_types.h>
#include <ip_addr.h> 
#include <espconn.h>

static bool configured;
static struct params params;


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        char hostname[UNS_HOSTNAME_MAXLEN];
        os_sprintf(hostname, "%s.%s", params.zone, params.name);
        uns_init(hostname);
        INFO("WIFI Connected to: %s", params.station_ssid);
        wifi_ap_stop();
        MEMCHECK();
    } 
    else {
        uns_deinit();    
        INFO("WIFI Disonnected from: %s", params.station_ssid);
        wifi_ap_start();
    }
}


ICACHE_FLASH_ATTR
void boothello() {
    uint8_t image = system_upgrade_userbin_check();
    INFO("");
    INFO("%s.%s version: "__version__, params.zone, params.name);
    INFO("My full name is: %s.%s", params.zone, params.name);
    INFO("Boot image: user%d", image + 1);
    INFO("Free memory: %d KB", system_get_free_heap_size());
    if (!configured) {
        INFO(
            "Connect to WIFI Access point: %s, "
            "open http://192.168.43.1 to configure me.",
            params.name
        );
    }
    //status_update(100, 1300, INFINITE, NULL);
    status_stop();

    /* Web UI */
	webadmin_start(&params);

    struct rst_info *r = system_get_rst_info();
    INFO("Boot reason: %d", r->reason);
    if (r->reason == REASON_WDT_RST || 
            r->reason == REASON_EXCEPTION_RST ||
            r->reason == REASON_SOFT_WDT_RST) {
        
        if (r->reason == REASON_EXCEPTION_RST) {
            ERROR("Fatal exception (%d)", r->exccause);
        }
    
        DEBUG("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, "
                "depc=0x%08x\n", r->epc1, r->epc2, r->epc3, r->excvaddr, 
                r->depc); //The address of the last crash is printed, which is used to debug garbled output.
    }
}


void user_init(void) {
    //uart_init(BIT_RATE_115200, BIT_RATE_115200);
    uart_div_modify(UART0, UART_CLK_FREQ / BIT_RATE_115200);
    uart_rx_intr_disable(UART0);
    uart_rx_intr_disable(UART1);
    //system_show_malloc(); 
    
    /* Uncomment and edit the interrupt.c to configure interrupts */
    //interrupt_init();

    relay_init();

    INFO("");
    INFO("BOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOTING");
    INFO("");

	configured = params_load(&params);
	if (!configured) {
		ERROR("Cannot load params");
		if(!params_defaults(&params)) {
			ERROR("Cannot save params");
			return;
		}
	}
    
    INFO("");
    PARAMS_PRINT(params);
	
    /* Status LED */
    status_init();

    /* Disable wifi led */
    wifi_status_led_uninstall();

    /* Start WIFI */
    wifi_start(&params, wifi_connect_cb);

    status_update(100, 400, 5, boothello);
}


ICACHE_FLASH_ATTR 
void user_pre_init(void) {
    MEMCHECK();
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		ERROR("system_partition_table_regist fail");
		while(1);
	}
    MEMCHECK();
}

