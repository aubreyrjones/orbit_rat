#!/usr/bin/env python3
import hid
from pprint import pprint

vid = 0x16c0	# Change it for your device
pid = 0x0478	# Change it for your device

device_list = hid.enumerate()

for dev in device_list:
    if dev['interface_number'] == 6:
        raw_hid_path = dev['path']


device = hid.Device(path=raw_hid_path)
device.write("you there?\0".encode("ascii"))
device.close()
