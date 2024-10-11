/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "stm32h753xx.h"

#define USB_PID   0x4000 
#define USB_VID   0xCafe
#define USB_BCD   0x0200

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum
{
  ITF_NUM_CDC_0 = 0,
  ITF_NUM_CDC_0_DATA,
  ITF_NUM_FASTIC_0,
  ITF_NUM_FASTIC_1,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN)

#define EPNUM_CDC_NOTIF         0x81
#define EPNUM_CDC_OUT           0x02
#define EPNUM_CDC_IN            0x82

#define EPNUM_FASTIC_0_OUT      0x03
#define EPNUM_FASTIC_0_IN       0x83

#define EPNUM_FASTIC_1_OUT      0x04
#define EPNUM_FASTIC_1_IN       0x84

uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // CDC interface used for communication and logging
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

  // FastIC bulk interfaces used for data transfer
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_FASTIC_0, 5, EPNUM_FASTIC_0_OUT, EPNUM_FASTIC_0_IN, 64),
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_FASTIC_1, 6, EPNUM_FASTIC_1_OUT, EPNUM_FASTIC_1_IN, 64)
};

uint8_t const desc_hs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // CDC interface used for communication and logging
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 512),

  // FastIC bulk interfaces used for data transfer
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_FASTIC_0, 5, EPNUM_FASTIC_0_OUT, EPNUM_FASTIC_0_IN, 512),
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_FASTIC_1, 6, EPNUM_FASTIC_1_OUT, EPNUM_FASTIC_1_IN, 512)
};

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
tusb_desc_device_qualifier_t const desc_device_qualifier =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,

  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .bNumConfigurations = 0x01,
  .bReserved          = 0x00
};

uint8_t const* tud_descriptor_device_qualifier_cb(void) {
  return (uint8_t const*) &desc_device_qualifier;
}

uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index) {
  return (tud_speed_get() == TUSB_SPEED_HIGH) ?  desc_fs_configuration : desc_hs_configuration;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
  return (tud_speed_get() == TUSB_SPEED_HIGH) ?  desc_hs_configuration : desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_VENDOR_0,
  STRID_VENDOR_1
};

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "CERN",                        // 1: Manufacturer
  "FastIC+ Readout",             // 2: Product
  NULL,                          // 3: UID (generated bellow)
  "Readout Communication",       // 4: Serial Interface
  "FastIC+ A Data",              // 5: FastIC+ A interface
  "FastIC+ B Data",              // 6: FastIC+ B interface
};

static uint16_t _desc_str[32 + 1];
static char * strUID;

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  size_t chr_count;

  switch ( index ) {
    case STRID_LANGID:
      memcpy(&_desc_str[1], string_desc_arr[0], 2);
      chr_count = 1;
      break;

    case STRID_SERIAL:
      // Create the serial string from the MCUs 96bit UID
      sprintf(strUID, "%08X%08X%08X", *(uint32_t *)(UID_BASE), *(uint32_t *)(UID_BASE + 4), *(uint32_t *)(UID_BASE + 8));
      chr_count = strlen(strUID);
      // Convert ASCII string into UTF-16
      for ( size_t i = 0; i < chr_count; i++ ) {
        _desc_str[1 + i] = strUID[i];
      }
      break;

    default:
      // Note: the 0xEE index string      memcpy(&_desc_str[1], string_desc_arr[0], 2); is a Microsoft OS 1.0 Descriptors.
      // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

      if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;

      const char *str = string_desc_arr[index];

      // Cap at max char
      chr_count = strlen(str);
      size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
      if ( chr_count > max_count ) chr_count = max_count;

      // Convert ASCII string into UTF-16
      for ( size_t i = 0; i < chr_count; i++ ) {
        _desc_str[1 + i] = str[i];
      }
      break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}
