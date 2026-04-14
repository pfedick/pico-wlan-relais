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

#include "wlanrelais.h"

#include "oled.h"
#include "lightstrip.h"

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
    relais.run();
    return 0;
}

WlanRelais::WlanRelais()
{
    initGpio();
    initDisplay();
}

void WlanRelais::initGpio()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(RELAY_PIN);
    gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 0);

    gpio_init(USB_BOOT_PIN);
    gpio_set_dir(USB_BOOT_PIN, GPIO_IN);
    gpio_pull_up(USB_BOOT_PIN);

    gpio_init(RELAIS_BUTTON);
    gpio_set_dir(RELAIS_BUTTON, GPIO_IN);
    gpio_pull_up(RELAIS_BUTTON);
}

void WlanRelais::initDisplay()
{
    oled.init(I2C_SDA_PIN, I2C_SCL_PIN);
}

void WlanRelais::enterUsbBoot()
{
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
    MessageBox("WlanRelais", "Running...");
    lights.putPixel(0, picopplib::Color(0, 16, 0));
    lights.write();
    while (1) {

        if (gpio_get(USB_BOOT_PIN) == 0) {
            sleep_ms(1);
            enterUsbBoot();
        }
        if (gpio_get(RELAIS_BUTTON) == 0) {
            activateRelais(true);
        } else {
            activateRelais(false);
        }

        led_blinking_task();
        uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());
        if (current_time_ms > start_time_ms + 100) {
            start_time_ms = current_time_ms;
            lights.write();
        }
        sleep_ms(1);
    }
}

void WlanRelais::activateRelais(bool activate)
{
    if (activate && relais_state == 0) {
        gpio_put(RELAY_PIN, 1);
        relais_state = 1;
    } else if (!activate && relais_state == 1) {
        gpio_put(RELAY_PIN, 0);
        relais_state = 0;
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
    font.setSize(14);
    font.setBold(true);
    screen.clear(0);
    screen.print(font, 0, -2, subject);
    screen.line(0, 16, 127, 16, 1);
    screen.line(0, 17, 127, 17, 1);

    font.setSize(10);
    font.setBold(false);
    picopplib::Array lines = wrapString(font, message, 128);

    int y = 18;
    for (const auto& line : lines) {
        if (y > 64) break;
        screen.print(font, 0, y, line);
        y += font.size();
        if (y > 64) break;
    }
    // screen.print(font, 0, 18, message);
    oled.draw(screen);
}