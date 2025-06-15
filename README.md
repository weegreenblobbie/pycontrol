Detecting your camera with gphoto2
==================================

Plug your camera into your raspberry pi via USB, then ssh in and run this command to
verify gphoto2 can detect your camera:

```
gphoto2 --auto-detect
```

In my case:
```
gphoto2 --auto-detect

Model                          Port
----------------------------------------------------------
Nikon Z7                       usb:001,005
```

List the properties that can be commanded:

```
gphoto2 --list-config
```

Try setting the iso, fstop, and shutter speed:

```
# List avialable iso options.
gphoto2 --get-config iso

# Set iso.
gphoto2 --get-config iso=800

# Set fstop, a.k.a. f-number.
gphoto2 --set-config f-number=f/8

# Set shutter speed to 1/500.
gphoto2 --set-config shutterspeed2=1/500
```

Try triggering the shuutter to capture an image to the internal memory
card:

```
gphoto2 --capture-image --frames 1

New file is in location /store_00010001/DCIM/103NZ7__/_NIK2973.NEF on the camera
```

Common errors
=============

```
*** Error: No camera found. ***
```

You have probably changed state of ther camera, like unplugging it or
turning off and back on.  To recover, run auto-detect again to "connect"
to the camera on the command line:

```
gphoto2 --auto-detect
```

Preventing Linux Automounting the camera as a media drive
=========================================================

If linux auto detect your camera's memory card, it may try to automatically
mount it as a disk you you can copy data off of it, which is great
for the desktop experience for most users.  However, this may prevent us
from controlling the camera with gphoto2 over the USB port.  We can
disable this behavior on the raspberry pi.

BEST: Adding a udev rule to stop mounting your camera
-----------------------------------------------------

This is a very targeted approcah, but requries you to identify your
camera's <Vendor ID> and <Product ID> per camera.

```
lsusb | grep -i nikon

Bus 001 Device 012: ID 04b0:0442 Nikon Corp. NIKON DSC Z 7
```
In my case:
```
<Vendor ID> == 04b0
<Product ID> == 0442
```
ACTION=="add", ATTRS{idVendor}=="<vendor_id>", ATTRS{idProduct}=="<product_id>", ENV{UDISKS_PRESENTATION_HIDE}="1"

(
VENDOR_ID=04b0
PRODUCT_ID=0442
echo "ACTION==\"add\", ATTRS{idVendor}==\"${VENDOR_ID}\", ATTRS{idProduct}==\"${PRODUCT_ID}\", ENV{UDISKS_PRESENTATION_HIDE}=\"1\"" | sudo tee -a /etc/udev/rules.d/99-ignore-camera.rules
sudo udevadm control --reload-rules
echo "SUCCESS"
)


Compiling the project
=====================

We need to install the folling depencies for building:
```
cmake
g++
git
gphoto2
libgphoto2-dev
libgtest-dev
libprotobuf-dev
libxml2-dev            # libgphoto2
libcurl4-gnutls-dev    # libgphoto2
libgd-dev              # libgphoto2
libexif-dev            # libgphoto2
libltdl-dev            # libgphoto2
libusb-1.0-0-dev       # libusb-dev
make
meson
protobuf-dev
python3.12-venv
```
