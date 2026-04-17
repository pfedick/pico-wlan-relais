#include "pico/stdlib.h"
#include "stdlib.h"
#include "stdio.h"
#include <unistd.h>
#include <math.h>
#include <locale.h>
#include <malloc.h>

#include "pico/types.h"
#include "pico/time.h"

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

#include "oled.h"
#include "lightstrip.h"

WlanRelais* g_relais = NULL;

int main()
{
    size_t free_heap_start = get_free_heap();
    setlocale(LC_ALL, "en_US.UTF-8");

    picopplib::Grafix gfx;
    loadFonts(gfx);
    size_t free_heap_after_font_load = get_free_heap();

    stdio_init_all();

    gpio_put(LED_PIN, 1);

    picopplib::String data;

    WlanRelais relais;

    relais.init_wlan();
    relais.run();
    return 0;
}

WlanRelais::WlanRelais()
{
    initGpio();
    initDisplay();
    setState(ConnectionStatus::Disconnected);
    g_relais = this;
}

void WlanRelais::initGpio()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(USB_BOOT_PIN);
    gpio_set_dir(USB_BOOT_PIN, GPIO_IN);
    gpio_pull_up(USB_BOOT_PIN);

    relais.init(RELAY_PIN, this);

    gpio_init(RELAIS_BUTTON);
    gpio_set_dir(RELAIS_BUTTON, GPIO_IN);
    gpio_pull_up(RELAIS_BUTTON);
}

void WlanRelais::initDisplay()
{
    oled.init(I2C_SDA_PIN, I2C_SCL_PIN);
}

void WlanRelais::setState(ConnectionStatus state)
{
    connection_status = state;
    last_status_change_ms = to_ms_since_boot(get_absolute_time());
    switch (state) {
    case ConnectionStatus::Disconnected:
        lights.putPixel(0, picopplib::Color(0, 0, 32));
        break;
    case ConnectionStatus::Connecting:
        lights.putPixel(0, picopplib::Color(32, 0, 32));
        break;
    case ConnectionStatus::Connected:
        lights.putPixel(0, picopplib::Color(0, 16, 0));
        break;
    case ConnectionStatus::Error:
        lights.putPixel(0, picopplib::Color(127, 0, 0));
        break;
    }
    lights.write();
    sleep_ms(100);
}

void WlanRelais::enterUsbBoot()
{
    setState(ConnectionStatus::Disconnected);
    gpio_put(LED_PIN, 1);
    MessageBox("WlanRelais", "Reset button pressed...");
    lights.clear(picopplib::Color(0, 0, 16));
    bool goto_program_mode = false;
    uint32_t ms_count = 0;

    while (gpio_get(USB_BOOT_PIN) == 0) {
        sleep_ms(1);
        ms_count++;
        if (ms_count > 1000) {
            goto_program_mode = true;
            break;
        }
    }
    gpio_put(LED_PIN, 0);
    if (!goto_program_mode) {
        MessageBox("WlanRelais", "Rebooting...");
        // short press, just reboot
        watchdog_reboot(0, 0, 0);
    }
    // long press, enter usb bootloader
    MessageBox("WlanRelais", "Programming Mode");

    sleep_ms(100);
    reset_usb_boot(0, 0);
}

void WlanRelais::run()
{
    uint32_t start_time_ms = to_ms_since_boot(get_absolute_time());
    // MessageBox("WlanRelais", "Running...");
    while (1) {
        if (gpio_get(USB_BOOT_PIN) == 0) {
            sleep_ms(1);
            enterUsbBoot();
        }
        if (gpio_get(RELAIS_BUTTON) == 0) {
            relais.requestCommand(Relais::Command::Toggle);
            while (gpio_get(RELAIS_BUTTON) == 0) {
                sleep_ms(1);
            }
        }

        led_blinking_task();
        uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());
        if (current_time_ms > start_time_ms + 100) {
            start_time_ms = current_time_ms;
            lights.write();
        }
        if (display_is_on && current_time_ms > last_display_change_ms + display_timeout_ms) {
            oled.clearDisplay();
            display_is_on = false;
        }
        if (connection_status != ConnectionStatus::Connected && current_time_ms > last_status_change_ms + 10000) {
            MessageBox("Reboot", "Reboot and try to reconnect...");
            sleep_ms(1000);
            watchdog_reboot(0, 0, 0);
        }
        relais.update(current_time_ms);
        if (connection_status == ConnectionStatus::Connected && current_time_ms > next_wlan_check_ms) {
            checkWlanConnection();
            next_wlan_check_ms = current_time_ms + 10000;
        }
        sleep_ms(1);
    }
}

void WlanRelais::led_blinking_task(void)
{
    static uint32_t start_ms = 0;

    // blink is disabled
    if (!blink_interval_ms) return;

    // Blink every interval ms
    uint32_t msSinceBoot = to_ms_since_boot(get_absolute_time());

    if (msSinceBoot - start_ms < blink_interval_ms) return; // not enough time
    start_ms += blink_interval_ms;

    gpio_put(LED_PIN, led_state);
#ifdef TEST_PIN
    gpio_put(TEST_PIN, led_state);
#endif
    // gpio_put(STATUS_LED,led_state);
    led_state = 1 - led_state; // toggle
}

extern "C"
{
    extern char __StackLimit;
}

size_t get_free_heap()
{
    char* heap_end = (char*)sbrk(0);
    // Der freie Bereich liegt zwischen dem aktuellen Heap-Ende
    // und dem untersten Ende des Stacks
    return (size_t)(&__StackLimit - heap_end);
}

picopplib::Array wrapString(const picopplib::Font& font, const picopplib::String& str, int width)
{
    picopplib::Array lines;
    picopplib::Array words(str, " ");
    picopplib::String currentLine;
    for (auto& word : words) {
        if (currentLine.isEmpty()) {
            currentLine = word;
        } else {
            picopplib::String testLine = currentLine + " " + word;
            if (font.measure(testLine).width <= width) {
                currentLine = testLine;
            } else {
                lines.add(currentLine);
                currentLine = word;
            }
        }
    }
    if (currentLine.notEmpty()) {
        lines.add(currentLine);
    }
    return lines;
}

void WlanRelais::MessageBox(const picopplib::String& subject, const picopplib::String& message)
{
    picopplib::Font font;
    font.setColor(picopplib::Color(255, 255, 255));
    font.setOrientation(picopplib::Font::Orientation::TOP);
    font.setSize(12);
    font.setBold(true);
    screen.clear(0);
    screen.print(font, 0, -2, subject);
    screen.line(0, 13, 127, 13, 1);
    screen.line(0, 14, 127, 14, 1);

    font.setSize(9);
    font.setBold(false);
    picopplib::Array lines = wrapString(font, message, 128);

    int y = 15;
    for (const auto& line : lines) {
        if (y > 64) break;
        screen.print(font, 0, y, line);
        // y += font.size();
        y += 10;
        if (y > 64) break;
    }
    // screen.print(font, 0, 18, message);
    oled.draw(screen);
    last_display_change_ms = to_ms_since_boot(get_absolute_time());
    display_is_on = true;
}

void WlanRelais::Debug(const picopplib::String& message)
{
    picopplib::Font font;
    font.setColor(picopplib::Color(255, 255, 255));
    font.setOrientation(picopplib::Font::Orientation::TOP);
    font.setSize(9);
    font.setBold(false);
    screen.clear(0);
    picopplib::Array lines = wrapString(font, message, 128);

    int y = 0;
    for (const auto& line : lines) {
        if (y > 64) break;
        screen.print(font, 0, y, line);
        y += font.size();
        if (y > 64) break;
    }

    oled.draw(screen);
    last_display_change_ms = to_ms_since_boot(get_absolute_time());
    display_is_on = true;
}

void WlanRelais::showStatus()
{
    uint8_t mac[6];
    int res = cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    picopplib::String macStr;
    if (res == 0) {
        // Formatierung in einen String (Hexadezimal)
        macStr = picopplib::ToString("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    const ip4_addr_t* ip = netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]);
    picopplib::String ipStr = picopplib::ToString("%d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
    MessageBox("Connected!", "IP: " + ipStr + "\nMAC: " + macStr + "\nRelay: " + (relais.isActive() ? "ON" : "OFF"));
}

void WlanRelais::init_wlan()
{
    setState(ConnectionStatus::Connecting);
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_GERMANY)) {
        MessageBox("WlanRelais", "Wi-Fi init failed");
        return;
    }
    cyw43_arch_enable_sta_mode();

    sleep_ms(100);
    uint8_t mac[6];
    int res = cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
    picopplib::String macStr;
    if (res == 0) {
        // Formatierung in einen String (Hexadezimal)
        macStr = picopplib::ToString("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        MessageBox("Connecting...", "MAC:" + macStr + "\nSSID: " + WIFI_SSID);
    }

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 60000)) {
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        picopplib::String errorMsg;

        switch (status) {
        case CYW43_LINK_BADAUTH:
            errorMsg = "Bad Auth - falsches Passwort?";
            break;
        case CYW43_LINK_NONET:
            errorMsg = "No Network - SSID nicht gefunden";
            break;
        case CYW43_LINK_FAIL:
            errorMsg = "Link Fail - Verbindung abgebrochen";
            break;
        default:
            errorMsg = picopplib::ToString("Status: %d", status);
            break;
        }
        MessageBox("Error", errorMsg + "\nMAC: " + macStr);
        setState(ConnectionStatus::Error);

    } else {
        init_http_server();
        setState(ConnectionStatus::Connected);
        showStatus();
    }
    next_wlan_check_ms = to_ms_since_boot(get_absolute_time()) + 10000;
}

void WlanRelais::checkWlanConnection()
{
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

    if (connection_status == ConnectionStatus::Connected && status != CYW43_LINK_UP) {
        MessageBox("WLAN Error", "Reconnecting...");
        setState(ConnectionStatus::Connecting);

        // Versuche Reconnect
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) == 0) {
            setState(ConnectionStatus::Connected);
            showStatus();
        } else {
            // Reconnect fehlgeschlagen → Reboot
            MessageBox("WLAN Error", "Reconnect failed! Rebooting...");
            setState(ConnectionStatus::Error);
            sleep_ms(2000);
            watchdog_reboot(0, 0, 0);
        }
    }
}

// Dynamischer Content-Generator
extern void init_cgi_handlers(void); // Definiert in api.cpp

void WlanRelais::init_http_server(void)
{
    init_cgi_handlers(); // CGI-Handler für Query-Parameter registrieren
    httpd_init();
}
