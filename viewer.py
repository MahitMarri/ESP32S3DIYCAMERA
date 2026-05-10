import serial
import struct
import numpy as np
import cv2
import time

PORT = "/dev/cu.usbmodem2101" 
BAUD = 115200 # Synced to match the fast data pipeline

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)
ser.reset_input_buffer()

print("Waiting for camera stream...")

while True:
    header = ser.read(5)
    if header != b'FRAME':
        continue
        
    size_data = ser.read(4)
    if len(size_data) != 4:
        continue
        
    size = struct.unpack("<I", size_data)[0]
    frame = ser.read(size)
    if len(frame) != size:
        continue
        
    img = cv2.imdecode(np.frombuffer(frame, np.uint8), cv2.IMREAD_COLOR)
    if img is not None:
        cv2.imshow("ESP32-S3 OV3660 Live Feed", img)
        
    if cv2.waitKey(1) == 27:
        break

cv2.destroyAllWindows()
ser.close()
