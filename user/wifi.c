#include "wifi.h"
#include "debug.h"
#include "user_config.h"
#include "params.h"
#include "uns.h"

#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>


static ETSTimer WiFiLinker;
static uint8_t wifiStatus = STATION_IDLE;
static uint8_t lastWifiStatus = STATION_IDLE;
static wificb wifiCb = NULL;


#define WIFI_CHECKINTERVAL_STABLE       5000
#define WIFI_CHECKINTERVAL              1000


#ifdef SNTP_ENABLED

#define SNTP_STOPPED    0
#define SNTP_STARTED    1
#define SNTP_ACQUIRED   2

static uint8_t sntp_status = SNTP_STOPPED;

static void ICACHE_FLASH_ATTR 
_sntp_stop() {
    sntp_stop();
    sntp_status = SNTP_STOPPED;
}

static void ICACHE_FLASH_ATTR 
_sntp_check(){
    
    if (sntp_status == SNTP_STOPPED) {
        sntp_setservername(0, "us.pool.ntp.org"); 
        sntp_setservername(1, "ntp.sjtu.edu.cn"); 
        sntp_init();
        sntp_status = SNTP_STARTED;
        INFO("SNTP initialized");
        return;
    }

    uint32 current_stamp;
    current_stamp = sntp_get_current_timestamp();
    if(current_stamp == 0) {
        ERROR("SNTP time fetch failed!");
    } 
    else {
        sntp_status = SNTP_ACQUIRED;
        os_printf("sntp: %d, %s \n", current_stamp, 
                sntp_get_real_time(current_stamp));
    }
}

#endif

static void ICACHE_FLASH_ATTR 
wifi_check_ip(void *arg) {
    struct ip_info ipConfig;

    os_timer_disarm(&WiFiLinker);
    wifi_get_ip_info(STATION_IF, &ipConfig);
    wifiStatus = wifi_station_get_connect_status();
    if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0) {
        uns_cleanup();
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
        os_timer_arm(&WiFiLinker, WIFI_CHECKINTERVAL_STABLE, 0);
#ifdef SNTP_ENABLED
        if (sntp_status != SNTP_ACQUIRED) {
            _sntp_check();
        }
#endif
    }
    else
    {
#ifdef SNTP_ENABLED
        _sntp_stop();
#endif
        if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD) {
            INFO("STATION_WRONG_PASSWORD");
            wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_NO_AP_FOUND) {
            INFO("STATION_NO_AP_FOUND");
            wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_CONNECT_FAIL) {
            INFO("STATION_CONNECT_FAIL");
            wifi_station_connect();
        }
        else {
            //INFO("STATION_IDLE");
        }

        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
        os_timer_arm(&WiFiLinker, WIFI_CHECKINTERVAL, 0);
    }
    
    if(wifiStatus != lastWifiStatus){
        lastWifiStatus = wifiStatus;
        if(wifiCb) {
            wifiCb(wifiStatus);
        }
    }
}


static void ICACHE_FLASH_ATTR 
wifi_init_softap(const char *ssid, const char *psk) {
    uint8_t mac[6];

    /***add by tzx for saving ip_info to avoid dhcp_client start****/
    struct dhcp_client_info dhcp_info;
    struct ip_info sta_info;

    system_rtc_mem_read(64, &dhcp_info, sizeof(struct dhcp_client_info));
    if(dhcp_info.flag == 0x01 ) {
        if (true == wifi_station_dhcpc_status()) {
            wifi_station_dhcpc_stop();
        }
        sta_info.ip = dhcp_info.ip_addr;
        sta_info.gw = dhcp_info.gw;
        sta_info.netmask = dhcp_info.netmask;
        if ( true != wifi_set_ip_info(STATION_IF, &sta_info)) {
            ERROR("set default ip wrong");
        }
    }
    os_memset(&dhcp_info, 0, sizeof(struct dhcp_client_info));
    system_rtc_mem_write(64, &dhcp_info, sizeof(struct rst_info));

    // Get the device mac address
    bool ok = wifi_get_macaddr(SOFTAP_IF, &mac[0]);
    if (!ok) {
        ERROR("Cannot get softap macaddr");
    }

    // initialization
    struct softap_config *config = (struct softap_config *) \
            os_zalloc(sizeof(struct softap_config));

    // Get soft-AP config first.
    wifi_softap_get_config(config);     

    // Updating ssid
    os_sprintf(config->ssid, "%s", ssid);

    INFO("SSID: %s", config->ssid);
    config->ssid_len = 0; 
    if (os_strlen(psk)) {
        os_sprintf(config->password, psk);
        config->authmode = AUTH_WPA_WPA2_PSK;
    }
    else {
        config->authmode = AUTH_OPEN;
    }

    config->max_connection = 4;
    config->channel = 5;    
    config->beacon_interval = 120;

    // Set ESP8266 soft-AP config
    ok = wifi_softap_set_config(config); 
    os_free(config);
    if (!ok) {
        ERROR("Cannot set softap config");
        return;
    }

    struct station_info * station = wifi_softap_get_station_info();
    while (station) {
        INFO("bssid: "MACSTR", ip: "IPSTR, MAC2STR(station->bssid), 
                IP2STR(&station->ip));
        station = STAILQ_NEXT(station, next);
    }

    // Free it by calling functionss
    wifi_softap_free_station_info(); 
    wifi_softap_dhcps_stop(); // disable soft-AP DHCP server
    struct ip_info info;
    IP4_ADDR(&info.ip, 192, 168, 43, 1); // set IP
    IP4_ADDR(&info.gw, 192, 168, 43, 1); // set gateway
    IP4_ADDR(&info.netmask, 255, 255, 255, 0); // set netmask
    wifi_set_ip_info(SOFTAP_IF, &info);
    struct dhcps_lease dhcp_lease;
    IP4_ADDR(&dhcp_lease.start_ip, 192, 168, 43, 100);
    IP4_ADDR(&dhcp_lease.end_ip, 192, 168, 43, 105);
    wifi_softap_set_dhcps_lease(&dhcp_lease);
    wifi_softap_dhcps_start(); // enable soft-AP DHCP server
}


ICACHE_FLASH_ATTR 
void wifi_ap_start() {
    if(wifi_get_opmode() == STATIONAP_MODE) {
        return;
    }
    INFO("Starting softap\n");
    wifi_set_opmode_current(STATIONAP_MODE);
    wifi_softap_dhcps_start(); // enable soft-AP DHCP server
}


ICACHE_FLASH_ATTR 
void wifi_ap_stop() {
    if(wifi_get_opmode() == STATION_MODE) {
        return;
    }
    INFO("Stopping softap\n");
    wifi_softap_dhcps_stop(); // disable soft-AP DHCP server
    wifi_set_opmode_current(STATION_MODE);
    wifi_station_connect();
}


void ICACHE_FLASH_ATTR wifi_start(struct params *params, wificb cb) {
    struct station_config stationConf;
    
    INFO("WIFI_INIT");
    wifi_init_softap(params->name, params->ap_psk);
    wifi_set_opmode_current(STATIONAP_MODE);

    //wifi_set_broadcast_if(1); // 1: Station, 2: ap, 3: both
    //wifi_set_sleep_type(NONE_SLEEP_T);
    //wifi_set_sleep_type(MODEM_SLEEP_T);
    //wifi_set_sleep_type(LIGHT_SLEEP_T);
    //wifi_station_set_auto_connect(FALSE);
    wifiCb = cb;

    os_memset(&stationConf, 0, sizeof(struct station_config));

    os_sprintf(stationConf.ssid, "%s", params->station_ssid);
    os_sprintf(stationConf.password, "%s", params->station_psk);

    wifi_station_set_config_current(&stationConf);

    os_timer_disarm(&WiFiLinker);
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
    os_timer_arm(&WiFiLinker, WIFI_CHECKINTERVAL, 0);

    wifi_station_set_auto_connect(TRUE);
    wifi_station_connect();
}

