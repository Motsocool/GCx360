// Copyright 2021, Ryan Wendland, ogx360
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _XID_H_
#define _XID_H_

#include <Arduino.h>
#include <PluggableUSB.h>
#include "gamecube.h"

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

/**
 * @brief Represents the state of a GameCube controller, encapsulating all button
 * and analog controls in a structured manner.
 */
struct GameCubeControllerState {
    uint8_t buttonState[2]; ///< Stores state of buttons in two bytes
    uint8_t joystickX;      ///< Joystick X-axis value (0-255)
    uint8_t joystickY;      ///< Joystick Y-axis value (0-255)
    uint8_t cStickX;        ///< C-Stick X-axis value (0-255)
    uint8_t cStickY;        ///< C-Stick Y-axis value (0-255)
    uint8_t triggerL;       ///< Left trigger analog value (0-255)
    uint8_t triggerR;       ///< Right trigger analog value (0-255)
};

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
    /**
     * @brief Initialize the USB interface for a GameCube controller.
     * Sets up endpoints and configures the USB interface with appropriate class and subclass information.
     *
     * @param interfaceCount Pointer to store the updated count of USB interfaces.
     * @return int Status of the USB interface setup operation.
     */
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