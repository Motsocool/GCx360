// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include "main.h"
#include "usbd/usbd_xid.h"
#include "usbh/usbh_xinput.h"

uint8_t player_id;
XID_ usbd_xid;
usbd_controller_t usbd_c[MAX_GAMEPADS];

void setup()
{
    Serial1.begin(115200);

    pinMode(ARDUINO_LED_PIN, OUTPUT);
    pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
    pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
    digitalWrite(ARDUINO_LED_PIN, HIGH);

    memset(usbd_c, 0x00, sizeof(usbd_controller_t) * MAX_GAMEPADS);

    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        usbd_c[i].type = GAMECUBE;
        usbd_c[i].gamecube.in.bLength = sizeof(usbd_gamecube_in_t);
        usbd_c[i].gamecube.out.bLength = sizeof(usbd_gamecube_out_t);
    }

    //00 = Player 1 (MASTER)
    //01 = Player 2 (SLAVE 1)
    //10 = Player 3 (SLAVE 2)
    //11 = Player 4 (SLAVE 3)
    player_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);

    if (player_id == 0)
    {
        master_init();
    }
    else
    {
        slave_init();
    }

    usbd_xid.begin();
}

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

    //Handle GameCube communication
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
        }
        else if (usbd_xid.getType() == DISCONNECTED)
        {
            UDCON |= (1 << DETACH);
            RXLED0;
        }

        poll_timer = millis();
    }
}