#include "httpd.h"

#include <c_types.h>

static char demobuff[128];
static uint32_t demobufflen = 0;


static ICACHE_FLASH_ATTR
httpd_err_t demo_download(struct httpd_session *s) {
    httpd_err_t err = httpd_response_start(s, HTTPSTATUS_OK, NULL, 0, 
            HTTPHEADER_CONTENTTYPE_BINARY, 0, HTTPD_FLAG_STREAM);
    if (err) {
        return err;
    }
    
    err = httpd_send(s, "Foo"CR, 5);
    if (err) {
        return err;
    }
    err = httpd_send(s, "Bar"CR, 5);
    if (err) {
        return err;
    }
    err = httpd_send(s, "Baz"CR, 5);
    if (err) {
        return err;
    }
    err = httpd_send(s, "Qux"CR, 5);
    if (err) {
        return err;
    }
    httpd_response_finalize(s, HTTPD_FLAG_CLOSE);
    return HTTPD_OK;
}


static struct httpd_session *downloader;
static struct httpd_multipart *uploader;

#define TBSIZE  8192


static struct ringbuffer *tb;


static ICACHE_FLASH_ATTR
httpd_err_t _multipart_stream_cb(struct httpd_multipart *m, char *data, 
        size16_t len, bool lastchunk, bool finish) {
    httpd_err_t err;
    size16_t avail = HTTPD_REQ_LEN(m->session);
    size16_t tbfree = RB_AVAILABLE(tb);
    size32_t more = HTTPD_REQUESTBODY_REMAINING(m->session);
    size16_t resplen;
    char *tmp;
    size16_t tmplen;

    if (uploader == NULL) {
        uploader = m;
    }
    
    DEBUG("CB: more: %6d S-avail: %4u len: %4u tbfree: %4u l: %d f: %d", 
            more, avail, len, tbfree, lastchunk, finish);

    /* Try to fill buffer */
    err = rb_write(tb, data, len);
    if (err) {
        goto reterr;
    }
    
    if (downloader) {
        resplen = HTTPD_RESP_LEN(downloader);
        if (!resplen) {
            tmp = os_zalloc(HTTPD_RESP_BUFFSIZE - 1);
            tmplen = rb_read(tb, tmp, HTTPD_RESP_BUFFSIZE - 1);

            /* httpd_send(%u) */
            err = httpd_send(downloader, tmp, tmplen);
            if (err) {
                goto reterr;
            }
            os_free(tmp);
        }
    }
    if (finish){
        /* Response uploader */
        err = HTTPD_RESPONSE_TEXT(m->session, HTTPSTATUS_OK, "Ok"CR, 4);
        if(err) {
            return err;
        }
        if (downloader != NULL) {
            /* Finalize downloader */
            httpd_response_finalize(downloader, HTTPD_FLAG_CLOSE);
            downloader = NULL;
        }
    }
    return HTTPD_OK;
reterr:
    os_free(tb->blob);
    os_free(tb);
    return err;
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_download_chunk_sent(struct httpd_session *s) {
    httpd_err_t err;
    size16_t respfree = HTTPD_RESP_FREE(s);
    size16_t tblen = RB_USED(tb);
    char *tmp;
    size16_t tmplen;

    //DEBUG("SENT CB: tblen: %4u respfree: %4u", tblen, respfree);
    if (respfree && tblen) {
        tmp = os_zalloc(respfree);
        tmplen = rb_read(tb, tmp, respfree);
        HTTPD_RESP_WRITE(s, tmp, tmplen);
        tblen -= tmplen;
        respfree -= tmplen;
        os_free(tmp);
    }
    
    if ((uploader != NULL) && (respfree >= (HTTPD_RESP_BUFFSIZE - 1))) {
        /* Unhold */
        if(!HTTPD_SCHEDULE(HTTPD_SIG_RECVUNHOLD, 
                    (os_param_t) uploader->session)) {
            return HTTPD_ERR_TASKQ_FULL;
        }
    }
    if ((!tblen) && (s->status >= HTTPD_SESSIONSTATUS_CLOSING)) {
        uploader = NULL;
        DEBUG("Finish: tblen: %4u respfree: %4u", tblen, respfree);
        /* free tb */
        os_free(tb->blob);
        os_free(tb);
    }
    return HTTPD_OK;
}

static ICACHE_FLASH_ATTR
httpd_err_t demo_download_stream(struct httpd_session *s) {
    s->sentcb = demo_download_chunk_sent;
    httpd_err_t err = httpd_response_start(s, HTTPSTATUS_OK, NULL, 0, 
            HTTPHEADER_CONTENTTYPE_BINARY, 0, HTTPD_FLAG_STREAM);
    if (err) {
        return err;
    }
    
    downloader = s;
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_multipart_stream(struct httpd_session *s) {
    /* Allocate tb */
    tb = os_zalloc(sizeof(struct ringbuffer));
    char *tbbuff = os_zalloc(TBSIZE);
    rb_init(tb, tbbuff, TBSIZE);
    return httpd_form_multipart_parse(s, _multipart_stream_cb);
}


static ICACHE_FLASH_ATTR
httpd_err_t _multipart_cb(struct httpd_multipart *m, char *data, 
        size16_t len, bool lastchunk, bool finish) {
    char tmp[HTTPD_MP_CHUNK];
    if (finish) {
        /* Finish */
        return HTTPD_RESPONSE_TEXT(m->session, HTTPSTATUS_OK, demobuff, demobufflen);
    }

    if (lastchunk) {
        /* len: %d */
        os_strncpy(tmp, data, len);
        tmp[len] = 0;
        demobufflen += os_sprintf(demobuff + demobufflen, "%s=%s ", m->field, tmp);
    }
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_multipart(struct httpd_session *s) {
    demobufflen = 0;
    return httpd_form_multipart_parse(s, _multipart_cb);
}


static ICACHE_FLASH_ATTR
httpd_err_t _form_cb(struct httpd_session *s, const char *field, 
        const char *value) {
    demobufflen += os_sprintf(demobuff + demobufflen, "%s=%s ", field, value);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_urlencoded(struct httpd_session *s) {
    err_t err;
    uint32_t more = HTTPD_REQUESTBODY_REMAINING(s);
    if (more) {
        return HTTPD_OK;
    }
    
    demobufflen = 0;
    err = httpd_form_urlencoded_parse(s, _form_cb);
    if (err) {
        return err;
    }
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, demobuff, demobufflen);
}

static ICACHE_FLASH_ATTR
httpd_err_t demo_querystring(struct httpd_session *s) {
    err_t err;
    demobufflen = 0;
    err = httpd_querystring_parse(s, _form_cb);
    if (err) {
        return err;
    }
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, demobuff, demobufflen);
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_headersecho(struct httpd_session *s) {
    return httpd_response(s, HTTPSTATUS_OK, s->request.headers, 
            s->request.headerscount, NULL, NULL, 0, false);
}


static ICACHE_FLASH_ATTR
httpd_err_t demo_index(struct httpd_session *s) {
    return HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, "Index", 5);
}


static
void demo_httpcb(int status, char *body, void *arg) {
    struct httpd_session *s = (struct httpd_session *) arg;
    httpd_err_t err = HTTPD_RESPONSE_TEXT(s, HTTPSTATUS_OK, body, 
            strlen(body));
    if (err) {
        DEBUG("response error\n");
        httpd_tcp_print_err(err);
    }
}

static ICACHE_FLASH_ATTR
httpd_err_t demo_tls_test(struct httpd_session *s) {
    https_get("fotassl.dobisel.com", "/", "", "", demo_httpcb, s);
    return HTTPD_MORE;
}
