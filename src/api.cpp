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

// Statische HTML-Seite (im Flash, nicht RAM)
static const char html_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
static const char html_page[] = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                                "<title>WLAN Relais</title><style>"
                                "body{font-family:Arial;max-width:600px;margin:50px auto;padding:20px;background:#f5f5f5}"
                                "h1{color:#333;border-bottom:2px solid #4CAF50;padding-bottom:10px}"
                                ".btn{display:block;padding:12px;margin:10px 0;text-decoration:none;background:#4CAF50;"
                                "color:#fff;border-radius:5px;text-align:center;font-weight:bold}"
                                ".api{background:#fff;padding:10px;margin:5px 0;border-left:3px solid #2196F3}"
                                "</style></head><body>"
                                "<h1>WLAN Relais Control</h1>"
                                "<a class='btn' href='/api/relay/on'>Turn ON</a>"
                                "<a class='btn' href='/api/relay/off'>Turn OFF</a>"
                                "<a class='btn' href='/api/relay/toggle'>Toggle</a>"
                                "<a class='btn' href='/api/relay/pulse'>Pulse</a>"
                                "<h2>API Endpoints</h2>"
                                "<div class='api'>GET /api/relay/on</div>"
                                "<div class='api'>GET /api/relay/off</div>"
                                "<div class='api'>GET /api/relay/toggle</div>"
                                "<div class='api'>GET /api/relay/pulse</div>"
                                "<div class='api'>GET /api/relay/status (JSON)</div>"
                                "</body></html>";

int fs_open_custom(struct fs_file* file, const char* name)
{
    // Root-Seite mit API-Dokumentation
    if (strcmp(name, "/") == 0 || strcmp(name, "/index.html") == 0) {
        static char response[1024];
        snprintf(response, sizeof(response), "%s%s", html_header, html_page);
        file->data = response;
        file->len = strlen(response);
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
