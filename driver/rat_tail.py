#!/usr/bin/env python3

import hid
from Xlib import display
from Xlib import X
from numpy import array

VENDOR_ID = 0x16c0
PRODUCT_ID = 0x0487

user_display = display.Display()
root_window = user_display.screen().root
screen_dim = array([user_display.screen().width_in_pixels, user_display.screen().height_in_pixels])
print(screen_dim)

def open_rat():
    device_list = hid.enumerate(VENDOR_ID, PRODUCT_ID)
    for dev in device_list:
        if dev['interface_number'] == 6:  # rawhid interface on the rat.
            raw_hid_path = dev['path']
    return hid.Device(path=raw_hid_path)


def main():
    root_window.change_attributes(event_mask=X.PropertyChangeMask)
    while True:
        print(user_display.next_event())
    pass

if __name__ == '__main__':
    exit(main())