#!/usr/bin/env python3
"""Runs a trained YOLO model on a live camera feed (e.g. a connected depth
camera's RGB stream, accessed as a plain UVC webcam via OpenCV) and shows
detections in a window in real time.

Usage:
    python3 scripts/live_detect.py [--model runs/detect/train/weights/best.pt] \
        [--camera 0] [--conf 0.4]

Press 'q' in the video window to quit.
"""
import argparse

import cv2
from ultralytics import YOLO


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="runs/detect/train/weights/best.pt")
    parser.add_argument("--camera", type=int, default=0, help="OpenCV camera index (/dev/videoN)")
    parser.add_argument("--conf", type=float, default=0.4)
    args = parser.parse_args()

    model = YOLO(args.model)
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(
            f"could not open camera index {args.camera} - try --camera 1, "
            f"or check `ls /dev/video*`"
        )

    print("press 'q' to quit")
    while True:
        ok, frame = cap.read()
        if not ok:
            print("failed to read frame from camera")
            break

        results = model.predict(frame, conf=args.conf, verbose=False)
        annotated = results[0].plot()
        cv2.imshow("live detection", annotated)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
