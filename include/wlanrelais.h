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

#define RELAY_PIN 12

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

class WlanRelais;

class Relais
{
public:
    enum class Command
    {
        None,
        On,
        Off,
        Toggle,
        Pulse
    };
    Relais();
    void init(uint gpio_pin, WlanRelais* controller = nullptr);
    void update(uint32_t now);        // In Main-Loop aufrufen
    void requestCommand(Command cmd); // Thread-safe
    bool isActive() const
    {
        return state;
    }

private:
    enum class PulseState
    {
        Idle,
        WaitingForOn,
        PulseOn,
        PulseOff
    };

    void processPendingCommand();
    void processPulseStateMachine(uint32_t now);
    void setState(bool active);

    uint pin;
    bool state = false;
    volatile Command pending_command = Command::None;
    PulseState pulse_state = PulseState::Idle;
    uint32_t pulse_next_action_ms = 0;
    WlanRelais* relaisController = nullptr;
};

class WlanRelais
{
private:
    Oled oled;
    picopplib::Drawable screen = picopplib::Drawable(128, 64);
    LightStrip lights = LightStrip(WS2812_PIN, 1);
    uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
    bool led_state = false;
    ConnectionStatus connection_status = ConnectionStatus::Disconnected;
    uint32_t last_display_change_ms = 0;
    uint32_t display_timeout_ms = 10000;
    uint32_t last_status_change_ms = 0;
    uint32_t next_wlan_check_ms = 0;
    bool display_is_on = true;

private:
    void led_blinking_task();
    void initGpio();
    void initDisplay();
    void enterUsbBoot();
    void checkWlanConnection();

public:
    Relais relais;

    WlanRelais();
    void init_wlan();
    void init_http_server();
    void run();
    void MessageBox(const picopplib::String& subject, const picopplib::String& message);
    void Debug(const picopplib::String& message);
    void showStatus();
    void setState(ConnectionStatus state);
};

#endif // WLANRELAIS_H
