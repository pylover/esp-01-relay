#include "params.h"
#include "status.h"
#include "webadmin.h"
#include "httpd.h"
#include "uns.h"
#include "http.h"
#include "relay.h"
#include "helpers.c"

#include <upgrade.h>
#include <osapi.h>
#include <mem.h>


#define WEBADMIN_ERR_FLASHREAD        -100
#define WEBADMIN_ERR_SAVEPARAMS       -101
#define WEBADMIN_UNKNOWNFIELD         -102
#define WEBADMIN_ERR_FLASHWRITE       -103
#define WEBADMIN_ERR_FLASHWRPROTECT   -104
#define WEBADMIN_BUFFSIZE             1024


static ICACHE_FLASH_ATTR
httpd_err_t relay_on(struct httpd_session *s) {
  update_relay_status(RELAY_ON);
  return HTTPD_RESPONSE_HEAD(s, HTTPSTATUS_OK);
}


static ICACHE_FLASH_ATTR
httpd_err_t relay_off(struct httpd_session *s) {
  update_relay_status(RELAY_OFF);
  return HTTPD_RESPONSE_HEAD(s, HTTPSTATUS_OK);
}


static ICACHE_FLASH_ATTR
httpd_err_t relay_toggle(struct httpd_session *s) {
  toggle_relay_status();
  return HTTPD_RESPONSE_HEAD(s, HTTPSTATUS_OK);
}


#define RELAY_JSON "{" \
  "\"status\": \"%s\"" \
"}"


static struct params *params;
static char buff[WEBADMIN_BUFFSIZE];
static size16_t bufflen;


static ICACHE_FLASH_ATTR
httpd_err_t relay_get(struct httpd_session *s) {
  char *st;
  if (get_relay_status() == RELAY_ON) {
     st = "On"; 
  }
  else {
    st = "Off";
  }
  bufflen = os_sprintf(buff, RELAY_JSON, st);
  return HTTPD_RESPONSE_JSON(s, HTTPSTATUS_OK, buff, bufflen);
}



struct fileserve {
    uint32_t remain;
    uint32_t addr;
};


static ICACHE_FLASH_ATTR
httpd_err_t _index_chunk_sent(struct httpd_session *s) {
    httpd_err_t err = HTTPD_OK;
    struct fileserve *f = (struct fileserve *) s->reverse;
    size16_t available = HTTPD_RESP_LEN(s);
    uint16_t readlen;
    uint16_t sendlen;
    
    if (available) {
        return HTTPD_OK;
    }
    
    if (f == NULL) {
        return HTTPD_OK;
    }
    
    /* Allocate a temp buff */
    char *tmp = os_zalloc(HTTPD_CHUNK);
    if (f->remain) {
        sendlen = readlen = MIN(HTTPD_CHUNK, f->remain);
        if (readlen % 4) {
            readlen += 4 - (readlen % 4);
        }
        /* Reading a chunk, len: %d addr:  */
        err = spi_flash_read(f->addr, (uint32_t*)tmp, readlen);
        if (err) {
            ERROR("SPI Flash read failed: %d", err);
            goto reterr;
        }
        
        /* Sending: %u bytes remain: %u */
        err = httpd_send(s, tmp, sendlen);
        if (err) {
            goto reterr;
        }
        f->addr += sendlen;
        f->remain -= sendlen;
    }
    
    if (!f->remain){
        /* Finalize, remain: %u */
        httpd_response_finalize(s, HTTPD_FLAG_NONE);
        os_free(f);
        s->reverse = NULL;
    } 
    
retok:
    /* Free OK */
    os_free(tmp);
    return HTTPD_OK;

reterr:
    /* Free Err: %d */
    os_free(tmp);
    os_free(f);
    s->reverse = NULL;
    return err;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_index_get(struct httpd_session *s) {
    httpd_err_t err;
    struct fileserve *f = os_zalloc(sizeof(struct fileserve));            
    s->reverse = f;
    f->addr = USER_INDEXHTML_ADDR;

    /* Read 4 bytes to determine the size  */
    err = spi_flash_read(f->addr, &f->remain, sizeof(uint32_t));
    if (err) {
        ERROR("SPI Flash read failed: %d", err);
        return err;
    }
    f->addr += sizeof(uint32_t);

    /* Response headers */
    struct httpd_header deflate = {"Content-Encoding", "deflate"};
    
    /* Start response: %u */
    s->sentcb = _index_chunk_sent;
    err = httpd_response_start(s, HTTPSTATUS_OK, &deflate, 1, 
            HTTPHEADER_CONTENTTYPE_HTML, f->remain, HTTPD_FLAG_NONE);
    if (err) {
        return err;
    }

    return HTTPD_OK;
}


struct filesave {
    char buff[SECT_SIZE];
    uint32_t len;
    size16_t sect;
};

static ICACHE_FLASH_ATTR
httpd_err_t webadmin_index_post(struct httpd_session *s) {
    httpd_err_t err;
    size16_t avail = HTTPD_REQ_LEN(s);
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    size32_t wlen;
    struct filesave *f;
    
    if (s->request.handlercalls == 1) {
        /* initialize */
        /* Disable flash erase protect */
        if (!spi_flash_erase_protect_disable()) {
            err = WEBADMIN_ERR_FLASHWRPROTECT;
            ERROR("Cannot spi_flash_erase_protect_disable(void)");
            goto reterr;
        }

        /* Alocate memory */
        f = os_zalloc(sizeof(struct filesave));

        char *sector = rindex(s->request.path, '/') + 1;
        if (sector == NULL) {
            ERROR("Please provide a sector");
            goto reterr;
        }
        f->sect = parse_uint(sector);
        f->len = sizeof(uint32_t);
        os_memcpy(f->buff, &s->request.contentlen, f->len);
        s->reverse = f;
    }
    else {
        f = (struct filesave*) s->reverse;
    }

    while (avail) {
        /* Read from request */
        f->len += HTTPD_RECV(s, f->buff + f->len, 
                MIN(avail, SECT_SIZE - f->len));
        avail = HTTPD_REQ_LEN(s);

        /* Decide to write a sector or not */
        if ((f->len == SECT_SIZE) || (f->len && (!more) && (!avail))) {
            err = spi_flash_erase_sector(f->sect);
            if (err) {
                ERROR("Erase sector "SECTFMT" error: %d: ", f->sect, err);
                goto reterr;
            }
            DEBUG("W Index: Sect: "SECTFMT" more: %6u avail: %4u wlen: %4u", 
                    f->sect, more, avail, f->len);
            
            wlen = f->len;
            if (wlen % 4) {
                wlen += 4 - (wlen % 4);
            }

            /* Write sector: %u */
            err = spi_flash_write(f->sect * SECT_SIZE, (uint32_t*)f->buff, 
                    wlen);
            if (err) {
                goto reterr;
            }
            f->len = 0;
            f->sect++; 
            if (more) {
                /* Unhold */
                if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
                    err = HTTPD_ERR_TASKQ_FULL;
                    goto reterr;
                }
            }
        }
    } 
    if (!more) {
        /* Terminating. */
        s->reverse = NULL;
        os_free(f);
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Done"CR, 6);
    }
    return HTTPD_MORE;

reterr:
    if (f) { 
        os_free(f);
    }
    if (s->reverse) {
        os_free(s->reverse);
    }
    return err;
}


static ICACHE_FLASH_ATTR
void _toggleboot() {
    system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    system_upgrade_reboot();
}


struct upgradestate {
    char buff[SECT_SIZE] STORE_ATTR;
    uint32_t len;
};

static
httpd_err_t webadmin_fw_upgrade(struct httpd_session *s) {
    httpd_err_t err;
    size16_t avail = HTTPD_REQ_LEN(s);
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    size32_t chunk;
    struct upgradestate *u;
     
    if (s->request.handlercalls == 1) {
        INFO("Initialize system upgrade");
        system_upgrade_init();
        system_upgrade_flag_set(UPGRADE_FLAG_START);
        u = os_zalloc(sizeof(struct upgradestate)); 
        u->len = 0;
        s->reverse = u;
        
        /* Erase flash */
        INFO("Erasing flash");
        uint32_t toerase = more;
        while (true) {
            if (toerase >= LIMIT_ERASE_SIZE) {
                system_upgrade_erase_flash(0xFFFF);
                toerase -= LIMIT_ERASE_SIZE;
                continue;
            }
            system_upgrade_erase_flash(toerase);
            break;
        }
    }
    else {
        u = (struct upgradestate*) s->reverse;
    }

    while (avail) {
        u->len += HTTPD_RECV(s, u->buff + u->len, 
                MIN(avail, SECT_SIZE - u->len));
        avail = HTTPD_REQ_LEN(s);
        
        if ((u->len == SECT_SIZE) || (u->len && (!more) && (!avail))) {
            DEBUG("FW: more: %6u avail: %4u wlen: %4u", more, avail, 
                    u->len);
            system_upgrade(u->buff, u->len);
            u->len = 0;
            if (more) {
                /* Unhold */
                if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, s)) {
                    os_free(u);
                    return HTTPD_ERR_TASKQ_FULL;
                }
            }
        }
    }
    
    if (!more) {
        /* Terminating. */
        status_update(200, 200, 5, _toggleboot);
        s->reverse = NULL;
        os_free(u);
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Rebooting"CR, 11);
    }
    return HTTPD_MORE;
}

static ICACHE_FLASH_ATTR
void discovercb(struct unsrecord *rec, void *arg) {
    struct httpd_session *s = (struct httpd_session *) arg;
    int bufflen = os_sprintf(buff, "%s "IPSTR"\n", rec->fullname, 
            IP2STR(&rec->address));
    HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_uns_discover(struct httpd_session *s) {
    char *pattern = rindex(s->request.path, '/') + 1;
    return uns_discover(pattern, discovercb, s);
}


static ICACHE_FLASH_ATTR
httpd_err_t _params_cb(struct httpd_session *s, const char *field, 
        const char *value, struct params *p) {
    char *target;
    /* Compare: %s */
    system_soft_wdt_feed();
    if (os_strcmp(field, "zone") == 0) {
        target = p->zone;
    }
    else if (os_strcmp(field, "name") == 0) {
        target = p->name;
    }
    else if (os_strcmp(field, "ap_psk") == 0) {
        target = p->ap_psk;
    }
    else if (os_strcmp(field, "ssid") == 0) {
        target = p->station_ssid;
    }
    else if (os_strcmp(field, "psk") == 0) {
        target = p->station_psk;
    }
    else {
        return WEBADMIN_UNKNOWNFIELD;;
    }

    if (value == NULL) {
        value = "";
    }
    /* Copy */
    
    //INFO("Updating params: %s", field);
    os_strcpy(target, value);
    /* After Copy */
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_params_post(struct httpd_session *s) {
    httpd_err_t err;
    size32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    if (more) {
        return HTTPD_OK;
    }
   
    /* parse */
    err = httpd_form_urlencoded_parse(s, _params_cb, &params);
    if (err) {
        return err;
    }
    if (!params_save(params)) {
        return WEBADMIN_ERR_SAVEPARAMS;
    }

    bufflen = os_sprintf(buff, 
            "Params has been saved, Rebooting in 4 seconds."CR);
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    INFO("Rebooting...");
    status_update(500, 500, 1, system_restart);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_params_get(struct httpd_session *s) {
#define PARAMS_JSON "{" \
    "\"zone\": \"%s\"," \
    "\"name\": \"%s\"," \
    "\"apPsk\": \"%s\"," \
    "\"ssid\": \"%s\"," \
    "\"psk\": \"%s\"" \
    "}"


    bufflen = os_sprintf(buff, PARAMS_JSON, 
            params->zone, 
            params->name, 
            params->ap_psk, 
            params->station_ssid, 
            params->station_psk);
    return HTTPD_RESPONSE_JSON(s, HTTPSTATUS_OK, buff, bufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_toggle_boot(struct httpd_session *s) {
    httpd_err_t err;
    uint8_t image = system_upgrade_userbin_check();
    bufflen = os_sprintf(buff, "Rebooting to user%d mode..."CR, image + 1);
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    status_update(500, 500, 1, _toggleboot);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_reboot(struct httpd_session *s) {
    httpd_err_t err;
    bufflen = os_sprintf(buff, "Rebooting..."CR);
    err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    if (err) {
        return err;
    }
    status_update(500, 500, 1, system_restart);
    return HTTPD_OK;
}


static
void httpcb(int status, char *body, void *arg) {
    struct httpd_session *s = (struct httpd_session *) arg;
    httpd_err_t err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, body, strlen(body));
    if (err) {
        httpd_tcp_print_err(err);
    }
}

#define SYSINFO \
    "zone:       %s,"CR \
    "name:       %s,"CR \
    "Boot:       user%d"CR \
    "Version:    %s"CR \
    "Uptime:     %u"CR \
    "Free mem:   %u"CR \
    "RTC:        %u"CR

#define SYSINFO_JSON "{" \
  "\"zone\": \"%s\"," \
  "\"name\": \"%s\"," \
  "\"uptime\": %u," \
  "\"boot\": \"user%d\"," \
  "\"version\": \"%s\"," \
  "\"free\": %u," \
  "\"rtc\": %u" \
"}"

static ICACHE_FLASH_ATTR
httpd_err_t webadmin_sysinfo_json(struct httpd_session *s) {
    uint8_t image = system_upgrade_userbin_check();
    bufflen = os_sprintf(buff, SYSINFO_JSON, 
        params->zone,
        params->name,
        system_get_time(),
        image + 1,
        __version__,
        system_get_free_heap_size(),
        system_get_rtc_time()
    );
    return HTTPD_RESPONSE_JSON(s, HTTPSTATUS_OK, buff, bufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t webadmin_sysinfo(struct httpd_session *s) {
    if (strlen(s->request.path) <= 1) {
        uint8_t image = system_upgrade_userbin_check();
        bufflen = os_sprintf(buff, SYSINFO, 
            params->zone,
            params->name,
            image + 1,
            __version__,
            system_get_time(),
            system_get_free_heap_size(),
            system_get_rtc_time()
        );
        return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, buff, bufflen);
    }
    
    char *pattern = rindex(s->request.path, '/');
    pattern++;
    DEBUG("Trying UNS for: %s\n", pattern);
    http_nobody_uns(pattern, "INFO", "/", httpcb, s);
    return HTTPD_MORE;
}


static struct httpd_route routes[] = {
    /* relay */
    {"GET",    "/relay",           relay_get     },
    {"TOGGLE", "/relay",           relay_toggle  },
    {"ON",     "/relay",           relay_on      },
    {"OFF",    "/relay",           relay_off     },

    /* Upgrade firmware over the air (wifi) */
    {"UPGRADE",    "/firmware",           webadmin_fw_upgrade     },
    
    /* Feel free to change these handlers */
    {"DISCOVER",   "/uns",                   webadmin_uns_discover  },

    /* Webadmin */
    {"POST",       "/params",             webadmin_params_post    },
    {"GET",        "/params.json",        webadmin_params_get     },
    {"GET",        "/status.json",        webadmin_sysinfo_json   },
    {"INFO",       "/",                   webadmin_sysinfo        },
    {"GET",        "/",                   webadmin_index_get      },
    {"POST",       "/",                   webadmin_index_post     },
    {"REBOOT",     "/",                   webadmin_reboot         },
    {"TOGGLE",     "/boots",              webadmin_toggle_boot    },
    { NULL }
};


ICACHE_FLASH_ATTR
int webadmin_start(struct params *_params) {
    err_t err;
    params = _params;
    err = httpd_init(routes);
    if (err) {
        ERROR("Cannot init httpd: %d", err);
    }
    return OK;
}


ICACHE_FLASH_ATTR
void webadmin_stop() {
    httpd_deinit();
}
