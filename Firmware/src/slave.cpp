<<<<<<< HEAD
// Copyright 2021, Ryan Wendland
=======
// Copyright 2021, Ryan Wendland, ogx360
>>>>>>> 026bdfb91d27390fb435aa2361c032a6225952bb
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <Wire.h>
<<<<<<< HEAD
=======

>>>>>>> 026bdfb91d27390fb435aa2361c032a6225952bb
#include "main.h"

extern usbd_controller_t usbd_c[MAX_GAMEPADS];

<<<<<<< HEAD
// Function prototypes
void handle_controller_state_packet(uint8_t packet_id);
void handle_ping_packet();

// Initialize slave settings and register I2C handlers
void slave_init() {
    uint8_t slave_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);
    Wire.begin(slave_id);  // Start I2C as slave with computed ID
    Wire.setClock(400000); // Set I2C clock speed to 400 kHz
    Wire.onRequest(i2c_send_data); // Register callback for sending data
    Wire.onReceive(i2c_get_data);  // Register callback for receiving data
    Wire.setWireTimeout(4000, true); // Set timeout for I2C communication
    gamecube_init();  // Initialize GameCube controller specifics
}

// Handle incoming I2C data
void i2c_get_data(int len) {
    if (len == 0) return; // Exit early if no data

    uint8_t packet_id = Wire.read();
    len--;  // Adjust for the packet ID read

    switch(packet_id) {
        case 0xAA:
            handle_ping_packet();
            break;
        case 0xF0 ... 0xFF:
            if(len > 0) {
                handle_controller_state_packet(packet_id);
            } else {
                Serial.println("Error: Expected more data for GameCube controller packet.");
            }
            break;
        default:
            Serial.print("Unknown packet ID: ");
            Serial.println(packet_id, HEX);
            break;
    }

    // Flush any remaining bytes in buffer
    while (Wire.available()) Wire.read();
}

// Respond to I2C ping packets
void handle_ping_packet() {
    digitalWrite(ARDUINO_LED_PIN, LOW);
    delay(250);
    digitalWrite(ARDUINO_LED_PIN, HIGH);
}

// Handle controller-specific data packets
void handle_controller_state_packet(uint8_t packet_id) {
    usbd_c[0].type = (xid_type_t)(packet_id & 0x0F);
    uint8_t *rxbuf = NULL;
    uint8_t rxlen = 0;

    if (usbd_c[0].type == GAMECUBE) {
        rxbuf = (uint8_t*)&usbd_c[0].gamecube.in;
        rxlen = sizeof(usbd_c[0].gamecube.in);

        if (Wire.available() != rxlen) {
            Serial.print("Error: Data length mismatch. Expected ");
            Serial.print(rxlen);
            Serial.print(", got ");
            Serial.println(Wire.available());
            return;
        }

        for (uint8_t i = 0; i < rxlen; i++) {
            rxbuf[i] = Wire.read();
        }
    } else {
        Serial.println("Unsupported controller type.");
    }
}

// Send data to I2C master
void i2c_send_data() {
    if (usbd_c[0].type == GAMECUBE) {
        Wire.write((uint8_t *)&usbd_c[0].gamecube.out, sizeof(usbd_c[0].gamecube.out));
    } else {
        Serial.println("No valid data to send.");
        Wire.write(0x00); // Send a placeholder or error code.
    }
}

// Additional tasks for slave mode
void slave_task() {
    // Periodic checks or updates for GameCube controller, if necessary
}

// GameCube controller initialization
void gamecube_init() {
    pinMode(GAMECUBE_DATA_PIN, OUTPUT);
    digitalWrite(GAMECUBE_POWER_PIN, HIGH);
    // Additional initialization logic for GameCube controller, including protocol setup
}
=======
void i2c_get_data(int len)
{
    uint8_t packet_id = Wire.read();

    //0xAA is a ping to see if the slave module is connected
    //Flash the LED to confirm receipt.
    if (packet_id == 0xAA)
    {
        digitalWrite(ARDUINO_LED_PIN, LOW);
        delay(250);
        digitalWrite(ARDUINO_LED_PIN, HIGH);
        goto flush_and_leave;
    }

    //Controller state packet 0xFx, where 'x' is the controller type.
    if ((packet_id & 0xF0) == 0xF0)
    {
        usbd_c[0].type = (xid_type_t)(packet_id & 0x0F);

        uint8_t *rxbuf = (usbd_c[0].type == GAMECUBE) ? ((uint8_t*)&usbd_c[0].gamecube.in) : NULL;
        uint8_t  rxlen = (usbd_c[0].type == GAMECUBE) ? sizeof(usbd_c[0].gamecube.in) : 0;

        if (len != rxlen + 1 /* because of status byte */ || rxbuf == NULL || rxlen == 0)
        {
            goto flush_and_leave;
        }

        for (uint8_t i = 0; i < rxlen; i++)
        {
            rxbuf[i] = Wire.read();
        }
    }

flush_and_leave:
    while (Wire.available())
    {
        Wire.read();
    }
}

void i2c_send_data(void)
{
    if (usbd_c[0].type == GAMECUBE)
    {
        Wire.write((uint8_t *)&usbd_c[0].gamecube.out, sizeof(usbd_c[0].gamecube.out));
    }
    else
    {
        //Just send something back so master isnt waiting
        Wire.write(0x00);
    }
}

void slave_init(void)
{
    uint8_t slave_id = digitalRead(PLAYER_ID1_PIN) << 1 | digitalRead(PLAYER_ID2_PIN);
    Wire.begin(slave_id);
    Wire.setClock(400000);
    Wire.onRequest(i2c_send_data);
    Wire.onReceive(i2c_get_data);
    Wire.setWireTimeout(4000, true);
}

void slave_task(void)
{
    //nothing to do!
}
>>>>>>> 026bdfb91d27390fb435aa2361c032a6225952bb
