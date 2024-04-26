// Copyright 2021, Ryan Wendland
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _MAIN_H_
#define _MAIN_H_

#include "usbd/usbd_xid.h"  // Includes for USB device interface
#include "usbh/usbh_xinput.h"  // Includes for USB host interface

// Define the maximum number of gamepads supported
#ifndef MAX_GAMEPADS
#define MAX_GAMEPADS 4
#endif

// Define pins used for USB reset and LED signaling
#define USB_HOST_RESET_PIN 9
#define ARDUINO_LED_PIN 17

// Define pins used to determine the player's ID
#define PLAYER_ID1_PIN 19
#define PLAYER_ID2_PIN 20

// Define pins specific to GameCube controller communication
#define GAMECUBE_DATA_PIN 7
#define GAMECUBE_POWER_PIN 8

// Default sensitivity setting for controllers where applicable
#ifndef SB_DEFAULT_SENSITIVITY
#define SB_DEFAULT_SENSITIVITY 400
#endif

// Structure to manage different types of controllers
typedef struct {
   xid_type_t type;                  // Type of controller (Duke, Steel Battalion, GameCube)
   usbd_duke_t duke;                 // Specifics for Duke controller
   usbd_steelbattalion_t sb;         // Specifics for Steel Battalion controller
   usbd_gamecube_t gamecube;         // Specifics for GameCube controller
} usbd_controller_t;

// Function declarations for system initialization and regular tasks
void master_init();    // Initialize the system in master mode
void master_task();    // Tasks to perform in master mode
void slave_init();     // Initialize the system in slave mode
void slave_task();     // Tasks to perform in slave mode

// GameCube specific function declarations
void gamecube_init();              // Initialize the GameCube controller
void gamecube_send_command(uint8_t command);  // Send a command to the GameCube controller
uint8_t gamecube_read_data();      // Read data from the GameCube controller

#endif // _MAIN_H_
