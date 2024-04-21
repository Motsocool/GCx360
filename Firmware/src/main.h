// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _MAIN_H_
#define _MAIN_H_

#include "usbd/usbd_xid.h"

#ifndef MAX_GAMEPADS
#define MAX_GAMEPADS 4
#endif

#define USB_HOST_RESET_PIN 9
#define ARDUINO_LED_PIN 17
#define PLAYER_ID1_PIN 19
#define PLAYER_ID2_PIN 20

// Pin assignments for GameCube controller communication
#define SHIELD_PIN_L 4
#define SHIELD_PIN_R 26
#define BOOTSEL_PIN 11
#define GC_DATA_PIN 7
#define GC_3V3_PIN 6

typedef struct
{
   xid_type_t type;
   usbd_gamecube_t gamecube;
} usbd_controller_t;

void master_init();
void master_task();
void slave_init();
void slave_task();

#endif