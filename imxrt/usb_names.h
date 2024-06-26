/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _usb_names_h_
#define _usb_names_h_

// These definitions are intended to allow users to override the default
// USB manufacturer, product and serial number strings.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[16];
};

extern struct usb_string_descriptor_struct usb_string_manufacturer_name;
extern struct usb_string_descriptor_struct usb_string_product_name;
extern struct usb_string_descriptor_struct usb_string_serial_number;
extern struct usb_string_descriptor_struct usb_string_midi_port1;
extern struct usb_string_descriptor_struct usb_string_midi_port2;
extern struct usb_string_descriptor_struct usb_string_midi_port3;
extern struct usb_string_descriptor_struct usb_string_midi_port4;
extern struct usb_string_descriptor_struct usb_string_midi_port5;
extern struct usb_string_descriptor_struct usb_string_midi_port6;
extern struct usb_string_descriptor_struct usb_string_midi_port7;
extern struct usb_string_descriptor_struct usb_string_midi_port8;
extern struct usb_string_descriptor_struct usb_string_midi_port9;
extern struct usb_string_descriptor_struct usb_string_midi_port10;
extern struct usb_string_descriptor_struct usb_string_midi_port11;
extern struct usb_string_descriptor_struct usb_string_midi_port12;
extern struct usb_string_descriptor_struct usb_string_midi_port13;
extern struct usb_string_descriptor_struct usb_string_midi_port14;
extern struct usb_string_descriptor_struct usb_string_midi_port15;
extern struct usb_string_descriptor_struct usb_string_midi_port16;

#ifdef __cplusplus
}
#endif

#endif
