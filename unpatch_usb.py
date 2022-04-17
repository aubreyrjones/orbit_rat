from os.path import join, isfile
import os
import shutil

Import("env")

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoteensy")

original_file = join(FRAMEWORK_DIR, "cores", "teensy3", "usb_desc.h")
backup = original_file + ".bkup"

def doit(*args, **kwargs):
    print("unpatching `usb_desc.h`")
    os.remove(original_file)
    shutil.copyfile(backup, original_file)


if isfile(backup):
    env.Execute(doit)
    