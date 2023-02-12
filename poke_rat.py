#!/usr/bin/env python3
import hid
from pprint import pprint

vid = 0x16c0
pid = 0x27d9

device_list = hid.enumerate(vid, pid)

for dev in device_list:
    pprint(dev)
    if dev['interface_number'] == 5 and dev['serial_number'] == 'GreyHelix.com:OrbitRatV1':
        raw_hid_path = dev['path']

print(raw_hid_path)
device = hid.Device(path=raw_hid_path)
device.write("you there?\0".encode("ascii"))
device.close()
