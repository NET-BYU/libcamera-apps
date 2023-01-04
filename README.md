# libcamera-apps

This is a small suite of libcamera-based apps that aim to copy the functionality of the existing "raspicam" apps. 

Build
-----
For usage and build instructions, see the official Raspberry Pi documenation pages [here.](https://www.raspberrypi.com/documentation/computers/camera_software.html#building-libcamera-and-libcamera-apps)

```bash
# Install dependencies for libcamera
sudo apt install -y libcamera-dev libjpeg-dev libtiff5-dev

# Install dependencies for libcamera-apps
sudo apt install -y cmake libboost-program-options-dev libdrm-dev libexif-dev

# Build libcamera-apps
cd
git clone https://github.com/raspberrypi/libcamera-apps.git
cd libcamera-apps
mkdir build
cd build
cmake .. -DENABLE_DRM=1 -DENABLE_X11=0 -DENABLE_QT=0 -DENABLE_OPENCV=0 -DENABLE_TFLITE=0
make -j1

# Install
sudo make install
sudo ldconfig # this is only necessary on the first build
```

License
-------

The source code is made available under the simplified [BSD 2-Clause license](https://spdx.org/licenses/BSD-2-Clause.html).

Status
------

[![ToT libcamera build/run test](https://github.com/raspberrypi/libcamera-apps/actions/workflows/libcamera-test.yml/badge.svg)](https://github.com/raspberrypi/libcamera-apps/actions/workflows/libcamera-test.yml)
