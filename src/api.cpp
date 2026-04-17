#include "pico/stdlib.h"
#include "stdlib.h"
#include "stdio.h"
#include <unistd.h>
#include <math.h>
#include <locale.h>
#include <malloc.h>

#include "pico/types.h"
#include "pico/time.h"
#include <boards/pico.h>
#include "picopplib.h"
#include "picopplib-grafix.h"
#include "hardware/i2c.h"
#include "pico/util/datetime.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"

#include "wlanrelais.h"

extern WlanRelais* g_relais;

// Custom Handler für JSON-Response
err_t httpd_post_begin(void* connection,
                       const char* uri,
                       const char* http_request,
                       u16_t http_request_len,
                       int content_len,
                       char* response_uri,
                       u16_t response_uri_len,
                       u8_t* post_auto_wnd)
{
    if (strncmp(uri, "/api/relay", 10) == 0) {
        // JSON wird direkt geschrieben
        return ERR_OK;
    }
    return ERR_VAL;
}

err_t httpd_post_receive_data(void* connection, struct pbuf* p)
{
    // Daten empfangen (für POST)
    return ERR_OK;
}

void httpd_post_finished(void* connection, char* response_uri, u16_t response_uri_len)
{
    // Antwort fertig - JSON wird über response_uri zurückgegeben
    snprintf(response_uri, response_uri_len, "/api/relay/response");
}

int fs_open_custom(struct fs_file* file, const char* name)
{
    // Root-Seite mit API-Doku
    if (strcmp(name, "/") == 0 || strcmp(name, "/index.html") == 0) {
        static const char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
                                       "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                                       "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                                       "<title>WLAN Relais</title><style>"
                                       "body{font-family:Arial;max-width:500px;margin:20px auto;padding:15px}"
                                       "h1{color:#333}a{display:block;padding:10px;margin:8px 0;background:#4CAF50;"
                                       "color:#fff;text-decoration:none;border-radius:4px;text-align:center}"
                                       "a:hover{background:#45a049}.info{color:#666;font-size:14px;margin-top:20px}"
                                       "</style></head><body>"
                                       "<h1>WLAN Relais</h1>"
                                       "<a href='/api/relay/on'>Turn ON</a>"
                                       "<a href='/api/relay/off'>Turn OFF</a>"
                                       "<a href='/api/relay/toggle'>Toggle</a>"
                                       "<a href='/api/relay/pulse'>Pulse (500ms)</a>"
                                       "<a href='/api/relay/status'>Show status</a>"
                                       "<div class='info'><b>API Endpoints:</b><br>"
                                       "GET /api/relay/on<br>GET /api/relay/off<br>GET /api/relay/toggle<br>"
                                       "GET /api/relay/pulse<br>GET /api/relay/status (JSON)</div>"
                                       "</body></html>";
        file->data = response;
        file->len = sizeof(response) - 1;
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }

    if (strcmp(name, "/api/relay/on") == 0) {
        g_relais->relais.requestCommand(Relais::Command::On);
        static const char response[] = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Connection: close\r\n\r\n"
                                       "{\"status\":\"ok\",\"relay\":\"on\"}";
        file->data = response;
        file->len = sizeof(response) - 1;
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    if (strcmp(name, "/api/relay/off") == 0) {
        g_relais->relais.requestCommand(Relais::Command::Off);
        static const char response[] = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Connection: close\r\n\r\n"
                                       "{\"status\":\"ok\",\"relay\":\"off\"}";
        file->data = response;
        file->len = sizeof(response) - 1;
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    if (strcmp(name, "/api/relay/pulse") == 0) {
        g_relais->relais.requestCommand(Relais::Command::Pulse);
        static const char response[] = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Connection: close\r\n\r\n"
                                       "{\"status\":\"ok\",\"relay\":\"pulsed\"}";
        file->data = response;
        file->len = sizeof(response) - 1;
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    if (strcmp(name, "/api/relay/toggle") == 0) {
        g_relais->relais.requestCommand(Relais::Command::Toggle);
        static const char response[] = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Connection: close\r\n\r\n"
                                       "{\"status\":\"ok\",\"relay\":\"toggled\"}";
        file->data = response;
        file->len = sizeof(response) - 1;
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }

    if (strcmp(name, "/api/relay/status") == 0) {
        const char* state = g_relais->relais.isActive() ? "on" : "off";
        static char response[128];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: close\r\n\r\n"
                 "{\"status\":\"ok\",\"relay\":\"%s\"}",
                 state);
        file->data = response;
        file->len = strlen(response);
        file->index = file->len;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    return 0;
}

void fs_close_custom(struct fs_file* file)
{
    // Nichts zu tun
}
