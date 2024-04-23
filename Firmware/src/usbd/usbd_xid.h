// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _XID_H_
#define _XID_H_

#include <Arduino.h>
#include <PluggableUSB.h>

#define XID_INTERFACECLASS 88
#define XID_INTERFACESUBCLASS 66

#ifndef _USING_HID
#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09
#define HID_REPORT_TYPE_INPUT 1
#define HID_REPORT_TYPE_OUTPUT 2
#endif

#define XID_EP_IN (pluggedEndpoint)
#define XID_EP_OUT (pluggedEndpoint + 1)

// Definitions specific to GameCube controllers
typedef enum {
    DISCONNECTED = 0,
    GAMECUBE
} xid_type_t;

// Constants and definitions from gamecube.h
#define GC_DATA_PIN PIN_A0
#define GC_DELAY_US 100 // Adjust the delay based on USBRetro project's values

#define GC_KEY_NOT_FOUND 0x00
#define BUTTON_MODE_0 0x00
#define BUTTON_MODE_1 0x01
#define BUTTON_MODE_2 0x02
#define BUTTON_MODE_3 0x03
#define BUTTON_MODE_4 0x04
#define BUTTON_MODE_KB 0x05

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t stickX;
    uint8_t stickY;
    uint8_t cStickX;
    uint8_t cStickY;
    uint8_t lTrigger;
    uint8_t rTrigger;
} usbd_gamecube_in_t;

typedef struct __attribute__((packed)) {
    uint8_t rumble;
} usbd_gamecube_out_t;

typedef struct __attribute__((packed)) {
    usbd_gamecube_in_t in;
    usbd_gamecube_out_t out;
} usbd_gamecube_t;

class XID_ : public PluggableUSBModule {
public:
    XID_(void);
    int begin(void);
    int sendReport(const void *data, int len);
    int getReport(void *data, int len);
    void setType(xid_type_t type);
    xid_type_t getType(void);
    void sendGCData(uint8_t data);
    uint8_t receiveGCData();

protected:
    int getInterface(uint8_t *interfaceCount);
    int getDescriptor(USBSetup &setup);
    bool setup(USBSetup &setup);

private:
    xid_type_t xid_type;
    uint8_t epType[2];
    uint8_t xid_in_data[32];
    uint8_t xid_out_data[32];
    uint32_t xid_out_expired;
};

XID_ &XID();

#endif // _XID_H_

static const DeviceDescriptor xid_dev_descriptor PROGMEM =
    D_DEVICE(0x00, 0x00, 0x00, USB_EP_SIZE, USB_VID, USB_PID, 0x0121, 0, 0, 0, 1);

static const uint8_t GAMECUBE_DESC_XID[] PROGMEM = {
    0x10,
    0x42,
    0x00, 0x01,
    0x05,
    0x03,
    0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const uint8_t GAMECUBE_CAPABILITIES_IN[] PROGMEM = {
    0x00,
    0x0E,
    0xFF,
    0x00,
    0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF
};

static const uint8_t GAMECUBE_CAPABILITIES_OUT[] PROGMEM = {
    0x00,
    0x01,
    0xFF
};