// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <UHS2/Usb.h>
#include <UHS2/usbhub.h>

#include "main.h"
#include "usbd_xid.h"
#include "gamecube.h"
#include "GamecubeConsole.h"

USB UsbHost;
USBHub Hub(&UsbHost);
USBHub Hub1(&UsbHost);
USBHub Hub2(&UsbHost);
USBHub Hub3(&UsbHost);
USBHub Hub4(&UsbHost);
XINPUT xinput1(&UsbHost);
XINPUT xinput2(&UsbHost);
XINPUT xinput3(&UsbHost);
XINPUT xinput4(&UsbHost);

typedef struct xinput_user_data
{
    uint8_t modifiers;
    uint32_t button_hold_timer;
    int32_t vmouse_x;
    int32_t vmouse_y;
} xinput_user_data_t;

extern usbd_controller_t usbd_c[MAX_GAMEPADS];
xinput_user_data_t user_data[MAX_GAMEPADS];

GamecubeConsole gc;
gc_report_t gc_report;

static void handle_gamecube(usbh_xinput_t *_usbh_xinput, usbd_gamecube_t *_usbd_gamecube, xinput_user_data_t *_user_data);

void master_init(void)
{
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    Wire.begin();
    Wire.setClock(400000);
    Wire.setWireTimeout(4000, true);

    // Init USB Host Controller
    digitalWrite(USB_HOST_RESET_PIN, LOW);
    delay(20);
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20);
    while (UsbHost.Init() == -1)
    {
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
        delay(500);
    }

    // Ping slave devices if present. This will cause them to blink.
    for (uint8_t i = 1; i < MAX_GAMEPADS; i++)
    {
        static const char ping = 0xAA;
        Wire.beginTransmission(i);
        Wire.write(&ping, 1);
        Wire.endTransmission(true);
        delay(100);
    }

    // Setup EEPROM for non-volatile settings
    static const uint8_t magic = 0xAB;
    if (EEPROM.read(0x00) != magic)
    {
        EEPROM.write(0, magic);
    }

    // Initialize GameCube controller
    int sm = -1;
    int offset = -1;
    GamecubeConsole_init(&gc, GC_DATA_PIN, pio, sm, offset);
    gc_report = default_gc_report;
}
void master_task(void)
{
    UsbHost.Task();
    UsbHost.IntHandler();
    UsbHost.busprobe();

    usbh_xinput_t *usbh_head = usbh_xinput_get_device_list();
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        usbh_xinput_t *_usbh_xinput = &usbh_head[i];
        usbd_controller_t *_usbd_c = &usbd_c[i];
        usbd_gamecube_t *_usbd_gamecube = &_usbd_c->gamecube;
        xinput_user_data_t *_user_data = &user_data[i];

        if (_usbh_xinput->bAddress == 0)
        {
            _usbd_c->type = DISCONNECTED;
        }

        // Must be connected, set a default device
        if (_usbh_xinput->bAddress && _usbd_c->type == DISCONNECTED)
        {
            _usbd_c->type = WAVEBIRD;
        }

        if (usbh_xinput_is_chatpad_pressed(_usbh_xinput, XINPUT_CHATPAD_GREEN))
        {
            _usbd_c->type = WAVEBIRD;
            _usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
        }

        if (_usbd_c->type == WAVEBIRD)
        {
            handle_gamecube(_usbh_xinput, _usbd_gamecube, _user_data);

            // Send updated gc_report to slave devices
            uint8_t *tx_buff = (uint8_t *)&gc_report;
            uint8_t tx_len = sizeof(gc_report_t);
            uint8_t status = 0xF0 | _usbd_c->type;

            Wire.beginTransmission(i);
            Wire.write(status);
            Wire.write(tx_buff, tx_len);
            Wire.endTransmission(true);
        }

        if (i == 0)
        {
            continue;
        }

        // Send data to slaves
        uint8_t *rx_buff = (_usbd_c->type == WAVEBIRD) ? ((uint8_t *)&_usbd_gamecube->out) : NULL;
        uint8_t rx_len = (_usbd_c->type == WAVEBIRD) ? sizeof(usbd_gamecube_out_t) : 0;

        if (rx_buff != NULL && rx_len != 0)
        {
            if (Wire.requestFrom(i, (int)rx_len) == rx_len)
            {
                while (Wire.available())
                {
                    *rx_buff = Wire.read();
                    rx_buff++;
                }
            }
        }
        // Flush
        while (Wire.available())
        {
            Wire.read();
        }
    }
}
static void handle_gamecube(usbh_xinput_t *_usbh_xinput, usbd_gamecube_t *_usbd_gamecube, xinput_user_data_t *_user_data)
{
    xinput_padstate_t *usbh_xstate = &_usbh_xinput->pad_state;
    _usbd_gamecube->in.buttons = 0x0000;

    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_UP)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_DPAD_UP;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_DPAD_DOWN;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_DPAD_LEFT;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_DPAD_RIGHT;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_START)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_START;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_A)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_A;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_B)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_B;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_X)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_X;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_Y)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_Y;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_L;
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_R;
    if (usbh_xstate->bRightTrigger > 128)
        _usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_Z;

    _usbd_gamecube->in.stickX = usbh_xstate->sThumbLX >> 8;
    _usbd_gamecube->in.stickY = usbh_xstate->sThumbLY >> 8;
    _usbd_gamecube->in.cStickX = usbh_xstate->sThumbRX >> 8;
    _usbd_gamecube->in.cStickY = usbh_xstate->sThumbRY >> 8;
    _usbd_gamecube->in.lTrigger = usbh_xstate->bLeftTrigger;
    _usbd_gamecube->in.rTrigger = usbh_xstate->bRightTrigger;

    _usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
    _usbh_xinput->lValue_requested = _usbd_gamecube->out.rumble;
    _usbh_xinput->rValue_requested = _usbd_gamecube->out.rumble;

    // Update gc_report based on Xbox 360 controller data
    gc_report.buttons = _usbd_gamecube->in.buttons;
    gc_report.stickX = _usbd_gamecube->in.stickX;
    gc_report.stickY = _usbd_gamecube->in.stickY;
    gc_report.cStickX = _usbd_gamecube->in.cStickX;
    gc_report.cStickY = _usbd_gamecube->in.cStickY;
    gc_report.lTrigger = _usbd_gamecube->in.lTrigger;
    gc_report.rTrigger = _usbd_gamecube->in.rTrigger;
}
void post_globals(uint8_t dev_addr, int8_t instance, uint32_t buttons, uint8_t analog_1x, uint8_t analog_1y,
                  uint8_t analog_2x, uint8_t analog_2y, uint8_t analog_l, uint8_t analog_r, uint32_t keys, uint8_t quad_x)
{
    bool is_extra = (instance == -1);
    if (is_extra) instance = 0;

    int player_index = find_player_index(dev_addr, instance);
    uint16_t buttons_pressed = (~(buttons | 0x800)) || keys;

    if (player_index < 0 && buttons_pressed)
    {
        player_index = add_player(dev_addr, instance);
    }

    if (player_index >= 0)
    {
        if (is_extra)
            players[0].altern_buttons = buttons;
        else
            players[player_index].global_buttons = buttons;

        if (analog_1x) players[player_index].output_analog_1x = analog_1x;
        if (analog_1y) players[player_index].output_analog_1y = analog_1y;
        if (analog_2x) players[player_index].output_analog_2x = analog_2x;
        if (analog_2y) players[player_index].output_analog_2y = analog_2y;
        players[player_index].output_analog_l = analog_l;
        players[player_index].output_analog_r = analog_r;
        players[player_index].output_buttons = players[player_index].global_buttons & players[player_index].altern_buttons;

        players[player_index].keypress[0] = (keys) & 0xff;
        players[player_index].keypress[1] = (keys >> 8) & 0xff;
        players[player_index].keypress[2] = (keys >> 16) & 0xff;

        if (!((players[player_index].output_buttons) & 0x8000))
            players[player_index].output_analog_r = 255;
        else if (analog_r > 250)
            players[player_index].output_buttons &= ~0x8000;

        if (!((players[player_index].output_buttons) & 0x4000))
            players[player_index].output_analog_l = 255;
        else if (analog_l > 250)
            players[player_index].output_buttons &= ~0x4000;

        // Update gc_report based on player data
        gc_report.buttons |= players[player_index].output_buttons;
        gc_report.stickX = furthest_from_center(gc_report.stickX, players[player_index].output_analog_1x, 128);
        gc_report.stickY = furthest_from_center(gc_report.stickY, players[player_index].output_analog_1y, 128);
        gc_report.cStickX = furthest_from_center(gc_report.cStickX, players[player_index].output_analog_2x, 128);
        gc_report.cStickY = furthest_from_center(gc_report.cStickY, players[player_index].output_analog_2y, 128);
        gc_report.lTrigger = furthest_from_center(gc_report.lTrigger, players[player_index].output_analog_l, 0);
        gc_report.rTrigger = furthest_from_center(gc_report.rTrigger, players[player_index].output_analog_r, 0);
    }
}

void post_mouse_globals(uint8_t dev_addr, int8_t instance, uint16_t buttons, uint8_t delta_x, uint8_t delta_y, uint8_t quad_x)
{
    bool is_extra = (instance == -1);
    if (is_extra) instance = 0;

    int player_index = find_player_index(dev_addr, instance);
    uint16_t buttons_pressed = (~(buttons | 0x0f00));

    if (player_index < 0 && buttons_pressed)
    {
        player_index = add_player(dev_addr, instance);
    }

    if (player_index >= 0)
    {
        if (delta_x == 0) delta_x = 1;
        if (delta_y == 0) delta_y = 1;

        if (delta_x >= 128)
            players[player_index].global_x = players[player_index].global_x - (256-delta_x);
        else
            players[player_index].global_x = players[player_index].global_x + delta_x;

        if (players[player_index].global_x > 127)
            delta_x = 0xff;
        else if (players[player_index].global_x < -127)
            delta_x = 1;
        else
            delta_x = 128 + players[player_index].global_x;

        if (delta_y >= 128)
            players[player_index].global_y = players[player_index].global_y - (256-delta_y);
        else
            players[player_index].global_y = players[player_index].global_y + delta_y;

        if (players[player_index].global_y > 127)
            delta_y = 0xff;
        else if (players[player_index].global_y < -127)
            delta_y = 1;
        else
            delta_y = 128 + players[player_index].global_y;

        players[player_index].output_analog_1x = delta_x;
        players[player_index].output_analog_1y = delta_y;
        players[player_index].output_buttons = buttons;

        // Update gc_report based on player data
        gc_report.stickX = furthest_from_center(gc_report.stickX, players[player_index].output_analog_1x, 128);
        gc_report.stickY = furthest_from_center(gc_report.stickY, players[player_index].output_analog_1y, 128);
    }
}
// Main loop
void loop()
{
#if (0)
    //Performance loop timing.
    //Four slaves, four wireless controllers is about 4000us per loop
    //Perfect!
    static uint32_t loop_cnt = 0;
    static uint32_t loop_timer = 0;
    if (loop_cnt > 1000)
    {
        Serial1.print("Loop time: (us) ");
        Serial1.println(millis() - loop_timer);
        loop_cnt = 0;
        loop_timer = millis();
    }
    loop_cnt++;
#endif

    if (player_id == 0)
    {
        master_task();
    }
    else
    {
        slave_task();
    }

    //Handle GameCube side
    if (usbd_xid.getType() != usbd_c[0].type)
    {
        usbd_xid.setType(usbd_c[0].type);
    }

    static uint32_t poll_timer = 0;
    if (millis() - poll_timer > 4)
    {
        if (usbd_xid.getType() == GAMECUBE)
        {
            UDCON &= ~(1 << DETACH);
            RXLED1;

            // Wait for GameCube console to poll controller
            gc_rumble = GamecubeConsole_WaitForPoll(&gc) ? 255 : 0;

            // Send GameCube controller report
            GamecubeConsole_SendReport(&gc, &gc_report);

            usbd_xid.sendReport(&gc_report, sizeof(gc_report_t));
            usbd_xid.getReport(&gc_rumble, sizeof(uint8_t));
        }
        else if (usbd_xid.getType() == DISCONNECTED)
        {
            UDCON |= (1 << DETACH);
            RXLED0;
        }

        poll_timer = millis();
    }
}