#!/usr/bin/python

import time
import socket
import signal
from picamera2 import Picamera2
from picamera2.encoders import H264Encoder, MJPEGEncoder
from picamera2.outputs import FileOutput
from argparse import ArgumentParser


is_exit = False
MAX_PACKET_SIZE = 1024


def init_cam():
    picam2 = Picamera2(0)
    config = picam2.create_video_configuration()
    picam2.configure(config)
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
        pass

    cam.stop_recording()
    sock.close()
    print("cam closed")
