// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>
#include "main.h"
#include "gamecube.h"

extern usbd_controller_t usbd_c[MAX_GAMEPADS];
extern gc_report_t gc_report;

void mapButtons(const uint16_t xbox_buttons, gc_report_t& gc_report) {
    gc_report.a = xbox_buttons & GAMECUBE_BUTTON_A;
    gc_report.b = xbox_buttons & GAMECUBE_BUTTON_B;
    gc_report.x = xbox_buttons & GAMECUBE_BUTTON_X;
    gc_report.y = xbox_buttons & GAMECUBE_BUTTON_Y;
    gc_report.start = xbox_buttons & GAMECUBE_BUTTON_START;
    gc_report.z = xbox_buttons & GAMECUBE_BUTTON_Z;
    gc_report.r = xbox_buttons & GAMECUBE_BUTTON_R;
    gc_report.l = xbox_buttons & GAMECUBE_BUTTON_L;
    gc_report.dpad_up = xbox_buttons & GAMECUBE_BUTTON_DPAD_UP;
    gc_report.dpad_down = xbox_buttons & GAMECUBE_BUTTON_DPAD_DOWN;
    gc_report.dpad_left = xbox_buttons & GAMECUBE_BUTTON_DPAD_LEFT;
    gc_report.dpad_right = xbox_buttons & GAMECUBE_BUTTON_DPAD_RIGHT;
}

void validateAnalogInputs(gc_report_t& report) {
    report.stick_x = constrain(report.stick_x, 0, 255);
    report.stick_y = constrain(report.stick_y, 0, 255);
    report.cstick_x = constrain(report.cstick_x, 0, 255);
    report.cstick_y = constrain(report.cstick_y, 0, 255);
    report.l_analog = constrain(report.l_analog, 0, 255);
    report.r_analog = constrain(report.r_analog, 0, 255);
}

void processControllerData(uint8_t *rxbuf, uint8_t rxlen) {
    if (rxbuf == NULL || rxlen == 0) {
        Serial.println("Error: Buffer is null or empty");
        return;
    }

    for (uint8_t i = 0; i < rxlen; i++) {
        rxbuf[i] = Wire.read();
    }

    // Convert Xbox 360 controller inputs to GameCube format
    gc_report = default_gc_report;

    // Map buttons
    mapButtons(usbd_c[0].gamecube.in.buttons, gc_report);

    // Map analog sticks
    gc_report.stick_x = usbd_c[0].gamecube.in.stickX;
    gc_report.stick_y = usbd_c[0].gamecube.in.stickY;
    gc_report.cstick_x = usbd_c[0].gamecube.in.cStickX;
    gc_report.cstick_y = usbd_c[0].gamecube.in.cStickY;

    // Map analog triggers
    gc_report.l_analog = usbd_c[0].gamecube.in.lTrigger;
    gc_report.r_analog = usbd_c[0].gamecube.in.rTrigger;

    // Validate and constrain analog inputs
    validateAnalogInputs(gc_report);
}

void handleControllerType(uint8_t packet_id, uint8_t* rxbuf, uint8_t rxlen) {
    if ((packet_id & 0xF0) == 0xF0) {
        processControllerData(rxbuf, rxlen);
    } else {
        Serial.println("Error: Unsupported controller type or malformed packet");
    }
}

void i2c_get_data(int len) {
    uint8_t packet_id = Wire.read();

    // 0xAA is a ping to see if the slave module is connected
    // Flash the LED to confirm receipt.
    if (packet_id == 0xAA) {
        digitalWrite(ARDUINO_LED_PIN, LOW);
        delay(250);
        digitalWrite(ARDUINO_LED_PIN, HIGH);
        goto flush_and_leave;
    }

    uint8_t *rxbuf = (usbd_c[0].type == GAMECUBE) ? ((uint8_t*)&usbd_c[0].gamecube.in) : NULL;
    uint8_t rxlen = (usbd_c[0].type == GAMECUBE) ? sizeof(usbd_c[0].gamecube.in) : 0;

    if (len != rxlen + 1 /* because of status byte */) {
        Serial.println("Error: Incorrect data length");
        goto flush_and_leave;
    }

    handleControllerType(packet_id, rxbuf, rxlen);

flush_and_leave:
    while (Wire.available()) {
        Wire.read();
    }
}

void i2c_send_data(void) {
    if (usbd_c[0].type == GAMECUBE) {
        Wire.write((uint8_t *)&usbd_c[0].gamecube.out, sizeof(usbd_c[0].gamecube.out));
    } else {
        // Just send something back so master isn't waiting
        Wire.write(0x00);
    }
}

void slave_init(void) {
    uint8_t slave_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);
    Wire.begin(slave_id);
    Wire.setClock(400000);
    Wire.onRequest(i2c_send_data);
    Wire.onReceive(i2c_get_data);
    Wire.setWireTimeout(4000, true);
}

void slave_task(void) {
    // Nothing to do!
}