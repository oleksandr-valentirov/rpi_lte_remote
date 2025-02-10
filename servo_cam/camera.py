#!/usr/bin/python

import time
import socket
import signal
import cv2
import numpy as np
from picamera2 import Picamera2, MappedArray
from picamera2.encoders import H264Encoder
from picamera2.outputs import FileOutput
from argparse import ArgumentParser
from collections import deque


is_exit = False
circular_frame_buf = deque(maxlen=2)
detections = []


def remove_contained_bboxes(boxes):
    """ Removes all smaller boxes that are contained within larger boxes.
        Requires bboxes to be soirted by area (score)
        Inputs:
            boxes - array bounding boxes sorted (descending) by area 
                    [[x1,y1,x2,y2]]
        Outputs:
            keep - indexes of bounding boxes that are not entirely contained 
                   in another box
        """
    check_array = np.array([True, True, False, False])
    keep = list(range(0, len(boxes)))
    for i in keep: # range(0, len(bboxes)):
        for j in range(0, len(boxes)):
            # check if box j is completely contained in box i
            if np.all((np.array(boxes[j]) >= np.array(boxes[i])) == check_array):
                try:
                    keep.remove(j)
                except ValueError:
                    continue
    return keep


def non_max_suppression(boxes, scores, threshold=1e-1):
    """
    Perform non-max suppression on a set of bounding boxes and corresponding scores.
    Inputs:
        boxes: a list of bounding boxes in the format [xmin, ymin, xmax, ymax]
        scores: a list of corresponding scores 
        threshold: the IoU (intersection-over-union) threshold for merging bounding boxes
    Outputs:
        boxes - non-max suppressed boxes
    """
    # Sort the boxes by score in descending order
    boxes = boxes[np.argsort(scores)[::-1]]

    # remove all contained bounding boxes and get ordered index
    order = remove_contained_bboxes(boxes)

    keep = []
    while order:
        i = order.pop(0)
        keep.append(i)
        for j in order:
            # Calculate the IoU between the two boxes
            intersection = max(0, min(boxes[i][2], boxes[j][2]) - max(boxes[i][0], boxes[j][0])) * \
                           max(0, min(boxes[i][3], boxes[j][3]) - max(boxes[i][1], boxes[j][1]))
            union = (boxes[i][2] - boxes[i][0]) * (boxes[i][3] - boxes[i][1]) + \
                    (boxes[j][2] - boxes[j][0]) * (boxes[j][3] - boxes[j][1]) - intersection
            iou = intersection / union

            # Remove boxes with IoU greater than the threshold
            if iou > threshold:
                order.remove(j)
                
    return boxes[keep]


def get_mask(frame1, frame2, kernel=np.array((9,9), dtype=np.uint8)):
    """ Obtains image mask
        Inputs: 
            frame1 - Grayscale frame at time t
            frame2 - Grayscale frame at time t + 1
            kernel - (NxN) array for Morphological Operations
        Outputs: 
            mask - Thresholded mask for moving pixels
        """

    frame_diff = cv2.subtract(frame2, frame1)

    # blur the frame difference
    frame_diff = cv2.medianBlur(frame_diff, 3)
    mask = cv2.adaptiveThreshold(frame_diff, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,cv2.THRESH_BINARY_INV, 11, 3)
    mask = cv2.medianBlur(mask, 3)

    # morphological operations
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)

    return mask


def get_contour_detections(mask, thresh=400):
    """ Obtains initial proposed detections from contours discoverd on the mask. 
        Scores are taken as the bbox area, larger is higher.
        Inputs:
            mask - thresholded image mask
            thresh - threshold for contour size
        Outputs:
            detectons - array of proposed detection bounding boxes and scores [[x1,y1,x2,y2,s]]
        """
    # get mask contours
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, # cv2.RETR_TREE, 
                                   cv2.CHAIN_APPROX_TC89_L1)
    detections = []
    for cnt in contours:
        x,y,w,h = cv2.boundingRect(cnt)
        area = w*h
        if area > thresh: 
            detections.append([x,y,x+w,y+h, area])

    return np.array(detections)


def get_detections(frame1, frame2, bbox_thresh=400, nms_thresh=1e-3, mask_kernel=np.array((9,9), dtype=np.uint8)):
    """ Main function to get detections via Frame Differencing
        Inputs:
            frame1 - Grayscale frame at time t
            frame2 - Grayscale frame at time t + 1
            bbox_thresh - Minimum threshold area for declaring a bounding box 
            nms_thresh - IOU threshold for computing Non-Maximal Supression
            mask_kernel - kernel for morphological operations on motion mask
        Outputs:
            detections - list with bounding box locations of all detections
                bounding boxes are in the form of: (xmin, ymin, xmax, ymax)
        """

    # get image mask for moving pixels
    mask = get_mask(frame1, frame2, mask_kernel)

    # get initially proposed detections from contours
    detections = get_contour_detections(mask, bbox_thresh)

    if detections.size > 0:
        # separate bboxes and scores
        bboxes = detections[:, :4]
        scores = detections[:, -1]

        # perform Non-Maximal Supression on initial detections
        return non_max_suppression(bboxes, scores, nms_thresh)
    return detections


def movement_detector_callback(request):
    with MappedArray(request, "main") as m:
        for det in detections:
            x1, y1, x2, y2 = det
            # cv2.rectangle(m.array, (100, 100), (500, 500), (0, 255, 0, 0))
            cv2.rectangle(m.array, (int(x1 * 3), int(y1 * 2.25)), (int(x2 * 3), int(y2 * 2.25)), (0, 255, 0, 0))


def init_cam():
    picam2 = Picamera2(0)
    config = picam2.create_video_configuration(main={"size": (1920, 1080)}, lores={"size": (640, 480), "format": "XRGB8888"})
    picam2.configure(config)
    picam2.post_callback = movement_detector_callback
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
    encoder = H264Encoder(framerate=60)
    cam.start_recording(encoder, FileOutput(stream))

    (w0, h0) = cam.stream_configuration("main")["size"]
    (w1, h1) = cam.stream_configuration("lores")["size"]

    while not is_exit:  # wait for the sigint
        np_array = cam.capture_array("lores")
        circular_frame_buf.append(cv2.cvtColor(np_array, cv2.COLOR_RGB2GRAY))
        if len(circular_frame_buf) > 1:
            detections = get_detections(circular_frame_buf[-2], circular_frame_buf[-1])

    cam.stop_recording()
    sock.close()
    print("cam closed")
