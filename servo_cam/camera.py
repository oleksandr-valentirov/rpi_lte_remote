#!/usr/bin/python

import time
from picamera2 import Picamera2

picam2 = Picamera2(0)
config = picam2.create_video_configuration()
picam2.configure(config)
picam2.start(show_preview=False)
picam2.set_controls({"ScalerCrop": (768, 432, 3072, 1728)})
picam2.capture_metadata()  # sync
time.sleep(1)

picam2.capture_file(f"image_{0}.jpg", format='jpeg')
