#!/usr/bin/python

import time
import socket
import signal
import cv2
from picamera2 import Picamera2, MappedArray
from picamera2.encoders import H264Encoder
from picamera2.outputs import FileOutput
from argparse import ArgumentParser


is_exit = False
MAX_PACKET_SIZE = 1024


def drawing(request):
    with MappedArray(request, "main") as m:
        # draw a green rectangle example
        # cv2.rectangle(m.array, (100, 100), (500, 500), (0, 255, 0, 0))
        # draw a red cross example
        # cv2.line(m.array, (945, 540), (975, 540), (255, 0, 0), 1)
        # cv2.line(m.array, (960, 525), (960, 555), (255, 0, 0), 1)
        pass


def init_cam():
    picam2 = Picamera2(0)
    config = picam2.create_video_configuration(main={"size": (1920, 1080)})
    picam2.configure(config)
    picam2.post_callback = drawing
    picam2.start(show_preview=False)
    picam2.set_controls({"ScalerCrop": (768, 432, 3072, 1728)})
    picam2.capture_metadata()  # sync
    time.sleep(1)
    return picam2


def sing_handler(sign, frame):
    global is_exit
    if sign == signal.SIGINT:
        is_exit = True


if __name__ == "__main__":
    args = ArgumentParser()
    args.add_argument("--ip", dest="ip", type=str, required=True)
    args.add_argument("--port", dest="port", type=int, required=True)
    args = args.parse_args()

    signal.signal(signal.SIGINT, sing_handler)

    cam = init_cam()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.connect((args.ip, args.port))
    stream = sock.makefile("wb")
    encoder = H264Encoder()
    cam.start_recording(encoder, FileOutput(stream))

    while not is_exit:  # wait for the sigint
        pass  # run a tensorflow here

    cam.stop_recording()
    sock.close()
    print("cam closed")
