// Copyright 2021, Ryan Wendland
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include "main.h"
#include "usbd/usbd_xid.h"
#include "usbh/usbh_xinput.h"

uint8_t player_id;
XID_ usbd_xid;
usbd_controller_t usbd_c[MAX_GAMEPADS];

void setup() {
    Serial1.begin(115200);
    pinMode(ARDUINO_LED_PIN, OUTPUT);
    pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
    pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    memset(usbd_c, 0x00, sizeof(usbd_controller_t) * MAX_GAMEPADS);

    for (uint8_t i = 0; i < MAX_GAMEPADS; i++) {
        usbd_c[i].type = GAMECUBE;
        usbd_c[i].gamecube.in.bLength = sizeof(usbd_gamecube_in_t);
        usbd_c[i].gamecube.out.bLength = sizeof(usbd_gamecube_out_t);
        gamecube_init();  // Initialize each GameCube controller
    }

    player_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);

    if (player_id == 0) {
        master_init();  // Setup master controller logic
    } else {
        slave_init();  // Setup slave controller logic
    }
}

void loop() {
    if (player_id == 0) {
        master_task();  // Handle master controller tasks
    } else {
        slave_task();  // Handle slave controller tasks
    }

    static uint32_t poll_timer = millis();
    if (millis() - poll_timer > 4) {  // Poll every 4 ms
        for (uint8_t i = 0; i < MAX_GAMEPADS; i++) {
            if (usbd_c[i].type == GAMECUBE) {
                UDCON &= ~(1 << DETACH);  // Ensure USB is connected
                RXLED1;  // Activate RX LED to indicate activity
                usbd_xid.sendReport(&usbd_c[i].gamecube.in, sizeof(usbd_gamecube_in_t));
                usbd_xid.getReport(&usbd_c[i].gamecube.out, sizeof(usbd_gamecube_out_t));
            } else if (usbd_xid.getType() == DISCONNECTED) {
                UDCON |= (1 << DETACH);  // Disconnect if no controller is active
                RXLED0;  // Deactivate RX LED
            }
        }
        poll_timer = millis();  // Reset polling timer
    }
}

void gamecube_init() {
    pinMode(GAMECUBE_DATA_PIN, OUTPUT);
    digitalWrite(GAMECUBE_POWER_PIN, HIGH);  // Power on GameCube controller

    // Send initialization commands if necessary
    // This should be defined according to GameCube protocol specifics
    gamecube_send_command(INIT_COMMAND);  // Hypothetical constant representing an init command
}

void gamecube_send_command(uint8_t command) {
    pinMode(GAMECUBE_DATA_PIN, OUTPUT);
    digitalWrite(GAMECUBE_DATA_PIN, LOW);  // Start bit
    delayMicroseconds(1);  // Initial delay

    for (int i = 0; i < 8; i++) {
        digitalWrite(GAMECUBE_DATA_PIN, (command & 0x80) ? HIGH : LOW);
        command <<= 1;
        delayMicroseconds(3);  // Bit transmission delay
    }

    digitalWrite(GAMECUBE_DATA_PIN, HIGH);  // Stop bit
    delayMicroseconds(1);
}

uint8_t gamecube_read_data() {
    uint8_t data = 0;
    pinMode(GAMECUBE_DATA_PIN, INPUT);
    delayMicroseconds(2);  // Stabilization delay

    for (int i = 0; i < 8; i++) {
        data <<= 1;
        if (digitalRead(GAMECUBE_DATA_PIN) == HIGH) {
            data |= 0x01;
        }
        delayMicroseconds(3);  // Read delay per bit
    }

    return data;
}