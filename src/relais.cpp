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

Relais::Relais()
    : pin(0),
      state(false),
      pending_command(Command::None)
{
}

void Relais::init(uint gpio_pin, WlanRelais* controller)
{
    pin = gpio_pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
    state = false;
    relaisController = controller;
}

void Relais::update(uint32_t now)
{
    processPendingCommand();
    processPulseStateMachine(now);
}

void Relais::requestCommand(Command cmd)
{
    pending_command = cmd;
}

void Relais::processPendingCommand()
{
    switch (pending_command) {
    case Command::None:
        break;
    case Command::On:
        setState(true);
        break;
    case Command::Off:
        setState(false);
        break;
    case Command::Toggle:
        setState(!state);
        break;
    case Command::Pulse:
        if (state) {
            setState(false);
            pulse_state = PulseState::WaitingForOn;
            pulse_next_action_ms = to_ms_since_boot(get_absolute_time()) + 1000;
        } else {
            // Turn on and schedule off
            setState(true);
            pulse_state = PulseState::PulseOn;
            pulse_next_action_ms = to_ms_since_boot(get_absolute_time()) + 500;
        }
        break;
    }
    pending_command = Command::None;
}

void Relais::processPulseStateMachine(uint32_t now)
{
    switch (pulse_state) {
    case PulseState::Idle:
        break;
    case PulseState::WaitingForOn:
        if (now >= pulse_next_action_ms) {
            setState(true);
            pulse_state = PulseState::PulseOn;
            pulse_next_action_ms = now + 500;
        }
        break;
    case PulseState::PulseOn:
        if (now >= pulse_next_action_ms) {
            setState(false);
            pulse_state = PulseState::Idle;
        }
        break;
    case PulseState::PulseOff:
        if (now >= pulse_next_action_ms) {
            setState(true);
            pulse_state = PulseState::Idle;
        }
        break;
    }
}

void Relais::setState(bool active)
{
    if (active && state == false) {
        gpio_put(pin, 1);
        state = true;
        if (relaisController) relaisController->showStatus();
    } else if (!active && state == true) {
        gpio_put(pin, 0);
        state = false;
        if (relaisController) relaisController->showStatus();
    }
}
