#!/usr/bin/python

import time
import socket
import io
from threading import Thread
from picamera2 import Picamera2
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


def cam_handler(ip, port):
    cam = init_cam()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while not is_exit:
        data = io.BytesIO()
        cam.capture_file(data, format='jpeg')
        data.seek(0)
        for i in range(0, data.getbuffer().nbytes, MAX_PACKET_SIZE):
            chunk = data.read(MAX_PACKET_SIZE)
            sock.sendto(chunk, (ip, port))
        time.sleep(0.03)


if __name__ == "__main__":
    args = ArgumentParser()
    args.add_argument("--ip", dest="ip", type=str, required=True)
    args.add_argument("--port", dest="port", type=int, required=True)
    args = args.parse_args()

    t = Thread(target=cam_handler, args=(args.ip, args.port))
    t.start()

    while not is_exit:
        cmd = input(">>> ")
        if cmd == "exit":
            is_exit = True
