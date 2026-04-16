#ifndef WLANRELAIS_H
#define WLANRELAIS_H
#include <boards/pico.h>
#include "picopplib.h"
#include "picopplib-grafix.h"
#include "oled.h"
#include "lightstrip.h"

#define USB_BOOT_PIN 15

#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17

#define LED_PIN PICO_DEFAULT_LED_PIN
#define WS2812_PIN 14
#define RELAIS_BUTTON 13

#define RELAY_PIN 18

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

void loadFonts(picopplib::Grafix& gfx);
size_t get_free_heap();
picopplib::Array wrapString(const picopplib::Font& font, const picopplib::String& str, int width);

enum class ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected,
    Error,
};

class WlanRelais
{
private:
    Oled oled;
    picopplib::Drawable screen = picopplib::Drawable(128, 64);
    LightStrip lights = LightStrip(WS2812_PIN, 7);
    uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
    bool led_state = false;
    uint8_t relais_state = 0;
    ConnectionStatus connection_status = ConnectionStatus::Disconnected;
    uint32_t last_display_change_ms = 0;
    uint32_t display_timeout_ms = 10000;
    uint32_t last_status_change_ms = 0;
    bool display_is_on = true;

private:
    void led_blinking_task();
    void initGpio();
    void initDisplay();
    void enterUsbBoot();
    void toggleRelais();
    void showStatus();

public:
    WlanRelais();
    void init_wlan();
    void init_http_server();
    void run();
    void MessageBox(const picopplib::String& subject, const picopplib::String& message);
    void Debug(const picopplib::String& message);
    void activateRelais(bool activate);
    bool isRelaisActive() const
    {
        return relais_state == 1;
    }
    void setState(ConnectionStatus state);
};

#endif // WLANRELAIS_H
