from os.path import join, isfile
import os
from re import I
import shutil

Import("env")

#print(env.Dump())

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoteensy")

original_file = join(FRAMEWORK_DIR, "cores", "teensy3", "usb_desc.h")
backup = original_file + ".or_bkup"
my_file = "usb_desc_p.h"

def doit(*args, **kwargs):
    if os.path.exists(backup): return

    print("patching `usb_desc.h`")
    shutil.copyfile(original_file, backup)
    os.remove(original_file)
    shutil.copyfile(my_file, original_file)

def undoit(*args, **kwargs):
    if not os.path.exists(backup): return

    print("unpatching `usb_desc.h`")
    os.remove(original_file)
    shutil.copyfile(backup, original_file)
    os.remove(backup)

doit()
env.AddPostAction("checkprogsize", undoit)

