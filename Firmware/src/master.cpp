#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <UHS2/Usb.h>
#include <UHS2/usbhub.h>

#include "main.h"
#include "usbd_xid.h"

USB UsbHost;
USBHub Hub(&UsbHost);  // Assuming only one hub is actually required
XINPUT xinput1(&UsbHost);
XINPUT xinput2(&UsbHost);
XINPUT xinput3(&UsbHost);
XINPUT xinput4(&UsbHost);

typedef struct xinput_user_data {
    uint8_t modifiers;
    uint32_t button_hold_timer;
    int32_t vmouse_x;
    int32_t vmouse_y;
} xinput_user_data_t;

extern usbd_controller_t usbd_c[MAX_GAMEPADS];
xinput_user_data_t user_data[MAX_GAMEPADS];

void master_init(void) {
    pinMode(USB_HOST_RESET_PIN, OUTPUT);
    digitalWrite(USB_HOST_RESET_PIN, LOW);

    Wire.begin();
    Wire.setClock(400000);
    Wire.setWireTimeout(4000, true);

    digitalWrite(USB_HOST_RESET_PIN, LOW);  // Reset USB Host
    delay(20);
    digitalWrite(USB_HOST_RESET_PIN, HIGH);
    delay(20);
    while (UsbHost.Init() == -1) {
        digitalWrite(ARDUINO_LED_PIN, !digitalRead(ARDUINO_LED_PIN));
        delay(500);
    }

    // Ping slave devices
    const char ping = 0xAA;
    for (uint8_t i = 1; i < MAX_GAMEPADS; i++) {
        Wire.beginTransmission(i);
        Wire.write(ping);
        Wire.endTransmission(true);
        delay(100);
    }

    // EEPROM setup
    const uint8_t magic = 0xAB;
    if (EEPROM.read(0) != magic) {
        EEPROM.write(0, magic);
    }
}

void master_task(void) {
    UsbHost.Task();
    UsbHost.IntHandler();
    UsbHost.busprobe();

    usbh_xinput_t *usbh_head = usbh_xinput_get_device_list();
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        usbh_xinput_t *usbh_xinput = &usbh_head[i];
        usbd_controller_t *usbd_c = &usbd_c[i];
        usbd_gamecube_t *usbd_gamecube = &usbd_c->gamecube;
        xinput_user_data_t *user_data = &user_data[i];

        if (usbh_xinput->bAddress == 0) {
            usbd_c->type = DISCONNECTED;
        } else if (usbd_c->type == DISCONNECTED) {
            usbd_c->type = GAMECUBE;
        }

        if (usbh_xinput_is_chatpad_pressed(usbh_xinput, XINPUT_CHATPAD_GREEN)) {
            usbd_c->type = GAMECUBE;
            usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
        }

        if (usbd_c->type == GAMECUBE) {
            handle_gamecube(usbh_xinput, usbd_gamecube, user_data);

            uint8_t *tx_buff = (uint8_t *)usbd_gamecube;
            uint8_t tx_len = sizeof(usbd_gamecube_t);
            uint8_t status = 0xF0 | usbd_c->type;

            Wire.beginTransmission(i);
            Wire.write(status);
            Wire.write(tx_buff, tx_len);
            Wire.endTransmission(true);
        }
    }
}

void handle_gamecube(usbh_xinput_t *usbh_xinput, usbd_gamecube_t *usbd_gamecube, xinput_user_data_t *user_data) {
    xinput_padstate_t *usbh_xstate = &usbh_xinput->pad_state;
    usbd_gamecube->in.buttons = 0;  // Clear previous state

    // Map buttons and analog inputs from XINPUT to GameCube controller
    if (usbh_xstate->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
        usbd_gamecube->in.buttons |= GAMECUBE_BUTTON_DPAD_UP;
    }
    // Repeat for other buttons and analog inputs...

    usbd_gamecube->in.stickX = usbh_xstate->sThumbLX >> 8;
    usbd_gamecube->in.stickY = usbh_xstate->sThumbLY >> 8;
    // Setup other analog inputs...

    usbh_xinput->chatpad_led_requested = CHATPAD_GREEN;
    usbh_xinput->lValue_requested = usbd_gamecube->out.rumble;
    usbh_xinput->rValue_requested = usbd_gamecube->out.rumble;
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

        // Update usbd_gamecube_t data based on player data
        usbd_c[0].gamecube.in.buttons |= players[player_index].output_buttons;
        usbd_c[0].gamecube.in.stickX = furthest_from_center(usbd_c[0].gamecube.in.stickX, players[player_index].output_analog_1x, 128);
        usbd_c[0].gamecube.in.stickY = furthest_from_center(usbd_c[0].gamecube.in.stickY, players[player_index].output_analog_1y, 128);
        usbd_c[0].gamecube.in.cStickX = furthest_from_center(usbd_c[0].gamecube.in.cStickX, players[player_index].output_analog_2x, 128);
        usbd_c[0].gamecube.in.cStickY = furthest_from_center(usbd_c[0].gamecube.in.cStickY, players[player_index].output_analog_2y, 128);
        usbd_c[0].gamecube.in.lTrigger = furthest_from_center(usbd_c[0].gamecube.in.lTrigger, players[player_index].output_analog_l, 0);
        usbd_c[0].gamecube.in.rTrigger = furthest_from_center(usbd_c[0].gamecube.in.rTrigger, players[player_index].output_analog_r, 0);
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

        // Update usbd_gamecube_t data based on player data
        usbd_c[0].gamecube.in.stickX = furthest_from_center(usbd_c[0].gamecube.in.stickX, players[player_index].output_analog_1x, 128);
        usbd_c[0].gamecube.in.stickY = furthest_from_center(usbd_c[0].gamecube.in.stickY, players[player_index].output_analog_1y, 128);
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
            while (usbd_xid.receiveGCData() != 0x00);

            // Send GameCube controller button report
            usbd_gamecube_in_t gc_report = usbd_c[0].gamecube.in;
            for (int i = 0; i < sizeof(usbd_gamecube_in_t); i++)
            {
                usbd_xid.sendGCData(((uint8_t*)&gc_report)[i]);
            }

            // Receive rumble data from GameCube console
            usbd_c[0].gamecube.out.rumble = usbd_xid.receiveGCData();

            usbd_xid.sendReport(&usbd_c[0].gamecube, sizeof(usbd_gamecube_t));
            usbd_xid.getReport(&usbd_c[0].gamecube.out.rumble, sizeof(uint8_t));
        }
        else if (usbd_xid.getType() == DISCONNECTED)
        {
            UDCON |= (1 << DETACH);
            RXLED0;
        }

        poll_timer = millis();
    }
}