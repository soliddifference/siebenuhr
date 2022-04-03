Import('env')
import os, shutil, sys
import subprocess
#import commands
import re

from base64 import b64decode

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
# Dump build environment (for debug)
# print env.Dump()

try:
  env['PROJECT_DIR']
except NameError:
  project_dir = '/Users/foz/src/siebenuhr_2022'
  print(bcolors.OKBLUE + "Script was called from the shell. Setting project dir to " + project_dir + bcolors.ENDC)
  call_method = "commandline"
  # before_buildfs();
else:
  project_dir = env['PROJECT_DIR']
  print(bcolors.OKBLUE + "Script called through build target 'PIO upload file system image' (within PlatformIO). Setting project dir to " + project_dir + bcolors.ENDC)
  call_method = "pio"

##############################################################################

data = project_dir + "/data/"
css = data + "css/"
js = data + "js/"
www = project_dir + "/www/"

def before_buildfs(source, target, env):
    print(project_dir + bcolors.ENDC)

    print_MAC_address_project_Directory = project_dir + "/rsc/print_MAC_address/"

    print(bcolors.OKGREEN + "\n====   compiling MAC-address printing firmware ====" + bcolors.ENDC)
    os.system("pio run -d " + print_MAC_address_project_Directory)

    print(bcolors.OKGREEN + "\n====   uploading MAC-address printing firmware ====\n" + bcolors.ENDC)
    print(bcolors.WARNING + "===================================================" + bcolors.ENDC)
    print(bcolors.WARNING + "====    please press the 'boot' button on the  ====" + bcolors.ENDC)
    print(bcolors.WARNING + "====   ESP to successfully upload the firmware ====" + bcolors.ENDC)
    print(bcolors.WARNING + "===================================================" + bcolors.ENDC)


    os.system("pio run -t upload -d " + print_MAC_address_project_Directory)

    # install serial on OS X? "python -m pip install pyserial" as root
    import serial, time

    # ser = serial.Serial('/dev/cu.usbserial-14240', 115200)

    # while 1:
    #     serial_line = ser.readline()
    #     matchObj = re.match( r'MAC: (.*)', serial_line, re.M|re.I)
    #     if matchObj:
    #        macAddress = matchObj.group(1).strip()
    #        break

    # print(bcolors.OKGREEN + "\n====   found MAC " + macAddress + " on device  ====\n" + bcolors.ENDC)

    # ser.close() # Only executes once the loop exits
    macAddress = "jer"

    serialFile = project_dir + "/rsc/serial_numbers.txt"
    serialNumberFile = project_dir + "/rsc/serial_number.txt"

    serials = {}
    for line in open(serialFile):
        line=line.rstrip('\n')
        if not line.startswith("#") and not line.startswith("\n") and line:
            (key, val) = line.split()
            serials[key] = val
            #print line

    #print "result:"
    SerialNumber = ""
    SerialNumberMax = 0
    for mac in serials:
        if mac == macAddress:
            SerialNumber = serials[mac]
        if SerialNumberMax < int(serials[mac]):
            SerialNumberMax = int(serials[mac])
        #print mac + " " + serials[mac]

    if not SerialNumber:
        with open(serialFile, "a") as myfile:
            SerialNumber = str(SerialNumberMax+1).zfill(4)
            print(bcolors.OKGREEN + "====   New ESP32 found. Generating a new serial number  ====\n" + bcolors.ENDC)
            myfile.write("# siebenuhr deployed by script\n")
            myfile.write(macAddress + " " + SerialNumber + "\n")

    print(bcolors.OKGREEN + "====   Deploying to MAC " + macAddress + " with serial number " + SerialNumber + " ====\n" + bcolors.ENDC)

    with open(serialNumberFile, "w") as myfile:
        SerialNumber = int(SerialNumber)
        myfile.write(str(SerialNumber) + "\n")

    ########################################################

    print(bcolors.OKGREEN + "====   Packing files from www -> data for building filesystem  ====\n" + bcolors.ENDC)

    if not os.path.exists(data):
        os.makedirs(data)
    if not os.path.exists(css):
        os.makedirs(css)
    if not os.path.exists(js):
        os.makedirs(js)
    # os.system("cat " + project_dir + "/www/css/bootstrap.min.css " + project_dir + "/www/css/styles.css > " + project_dir + "/data/css/combined.css")
    # os.system("cat " + project_dir + "/www/js/jquery-1.12.1.js " + project_dir + "/www/js/jquery-minicolors.js " + project_dir + "/www/js/scripts.js > " + project_dir + "/data/js/combined.js")
    # os.system("cp " + project_dir + "/www/index.htm " + project_dir + "/data/")
    # os.system("cp " + project_dir + "/www/favicon.ico " + project_dir + "/data/")
    # os.system("cp " + project_dir + "/rsc/serial_number.txt " + project_dir + "/data/")
    # os.system("cp " + project_dir + "/rsc/timezones.txt " + project_dir + "/data/")
    # os.system("rm " + project_dir + "/rsc/serial_number.txt")


def after_buildfs(source, target, env):
    #print "removing temporary data directory after creation of filesystem  (script: build/prepare-SPIFF.py)"
    print(bcolors.OKGREEN + "\n====   Preparation complete. Removing data directory  ====\n" + bcolors.ENDC)
    print(bcolors.WARNING + "===================================================" + bcolors.ENDC)
    print(bcolors.WARNING + "====    please press the 'boot' button on the  ====" + bcolors.ENDC)
    print(bcolors.WARNING + "====   ESP to successfully upload the firmware ====" + bcolors.ENDC)
    print(bcolors.WARNING + "===================================================" + bcolors.ENDC)
    shutil.rmtree(data)

# if call_method == "commandline":
#     before_buildfs(0, 0, 0);
if call_method == "pio":
    env.AddPreAction("buildfs", before_buildfs)
    env.AddPreAction("$BUILD_DIR/spiffs.bin", before_buildfs)
    env.AddPostAction("$BUILD_DIR/spiffs.bin", after_buildfs)
