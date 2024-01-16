from typing import List, Tuple
from xmlrpc.client import boolean
import serial
import struct
import threading
import numpy as np
import cv2
import socket
import sys
import time

outCV = True

class Tof:

    def __init__(self, port="COM1"):
        # Initialize serial
        self.port = port
        self.serial = self._init_serial_port()
        # Initialize buffer
        self.buffer = bytearray()
        # Run/Stop flag
        self.run_flag = True

        # Start transmitting data
        self.start()
    

    def _init_serial_port(self) -> serial.Serial:
        """Initialize serial port"""
        ser = serial.Serial()
        ser.port = self.port
        # ser.baudrate = tof.static.BAUDRATE
        ser.timeout = 1
        try:
            ser.open()
        except:
            print(f"Failed to open port: {self.port}.")
            exit(1)
        return ser

    
    def  start(self) -> None:
        """Start transmitting data, creates a parser thread."""
        # self.send_command(command="settofpwr 0")
        # self.send_command(command="settofpwr 1")
        # self.send_command(command="settofconf 0")
        pass


    def stop(self) -> None:
        """Stop transmitting data"""
        # self.send_command(command="settofpwr 0")
        pass

    
    def is_running(self) -> bool:
        return self.run_flag


    def send_command(self, command=None) -> None:
        """Send command to module, prompt if variable 'command' is left empty."""
        if command is None:
            command = input("Command to Module: ")
        print("Sending command: " + str(command))
        bytes_written = self.serial.write(bytes(command+chr(0x0d)+chr(0x0a),"ASCII"))
        print("Bytes written: " + str(bytes_written))

        self._get_response()
    

    def _get_response(self) -> None:
        """Catch response after command is sent."""
        response = self.serial.read_until(expected="\r\n")
        # print(f"[RESPONSE] {response}")

    def getColorMap(self, Map: np.ndarray, Resolution: int):
        colormap = np.zeros((Resolution*8, Resolution*8), dtype=np.uint8)
        for i in range(0, Resolution):
            for j in range(0, Resolution):
                for k in range(0, 8):
                    for l in range(0, 8):
                        colormap.itemset((i*8+k, j*8+l), Map.item(i, j)/4000*255)
        color_depth = cv2.applyColorMap(colormap, cv2.COLORMAP_JET)
        # print("[{}] Show".format(round(time.time()*1000)))
        return color_depth
    def display(self, frame_name: str, frame: np.ndarray):
        cv2.imshow(frame_name, cv2.resize(frame, (640,640), interpolation=cv2.INTER_NEAREST))

    def get_frame(self) -> Tuple[np.ndarray, np.ndarray]:
        """Get a frame, tuple of lists of ndarray: (signal, range, status).
        Returns (None, None) if parsing failed.

        signal (peak rate per SPAD): ndarray of unsigned int (np.uintc)
        range (median range): ndarray of short (np.short)
        status (target status): ndarray of char (np.ubyte)"""

        # Read data
        self.buffer += self.serial.read(size=593)
        if len(self.buffer) < 593:
            return (None, None)
        # Cleanup previous tail
        ind = self.buffer.find(bytearray("DATA", encoding="ASCII"))
        self.buffer = self.buffer[ind:]
        # Align data
        self.buffer += self.serial.read(size=ind)
        if len(self.buffer) < 593:
            return (None, None)

        # Little Endian
        # 0-3: DATA
        # 4: Number of Zone
        # 5: Die Temperature
        # 6-9: Timestamp

        # 10-585: Signal data
        # 10 (+9*N): Number of Target
        # 11 - 14 (+9*N): peak_rate_kcps_per_spad(4 Bytes, little-endian)
        peak_rate_kcps_per_spad = np.zeros((8, 8), dtype=np.uintc)
        for i in range(64):
            peak_rate_kcps_per_spad[i//8][i%8] = struct.unpack("<I", self.buffer[9*i+11:9*i+15])[0]

        # 15 - 16 (+9*N): median_range
        target_status = np.zeros((8, 8), dtype=np.ubyte)
        median_range = np.zeros((8, 8), dtype=np.short)
        # ori_median_range = np.zeros((8, 8), dtype=np.short)
        for i in range(64):
            target_status[i//8][i%8] = struct.unpack("<B", self.buffer[9*i+18:9*i+19])[0]
            if target_status[i//8][i%8] > 9 or target_status[i//8][i%8] < 5:
                median_range[i//8][i%8] = 400
                # ori_median_range[i//8][i%8] = 4000
            else:
                median_range[i//8][i%8] = struct.unpack("<h", self.buffer[9*i+15:9*i+17])[0]
                median_range[i//8][i%8] = median_range[i//8][i%8] / 10
                # print(f"Ori:{median_range[i//8][i%8]}, mult:{median_range[i//8][i%8]*2.5}")

        # 17 (+9*N): Reflectance
        # 18 (+9*N): Target Status
        # target_status = np.zeros((8, 8), dtype=np.ubyte)
        # for i in range(64):
        #     target_status[i//8][i%8] = struct.unpack("<B", self.buffer[9*i+18:9*i+19])[0]
            
        # Clear buffer
        self.buffer = bytearray()

        return (target_status, median_range)


if __name__ == "__main__":
    
    # socket
    host, port = "127.0.0.1", 25001
    data = "1,2,3"
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connect = False   

    comport = sys.argv[1]
    tof = Tof(comport)
    while True:
        start_time = time.time()
        signal, dist = tof.get_frame()
        if dist is None:
            continue
        # # print(f"range(mm): {dist.mean() / 4}")
        # print(f"Min_Raw: {dist}")
        # print(f"Ori dist:{signal.min()}")
        # print(f"dist:{dist}")
        # # print(f"status:{status}")

        # print(f"FPS:{(1/(time.time()-start_time))}")
        # dist = np.flip(dist, axis=0)
        if outCV:
            color_depth = tof.getColorMap(dist, 8)
            # cv2.imshow("Depth", color_depth)
            tof.display('Depth', color_depth)
            if cv2.waitKey(1)==ord('q'):
                break

        else:
            try:
                if not connect:
                    sock.connect((host, port))
                    connect = True

                sock.sendall(dist.tobytes())
                # print(f"range(mm): {dist.mean() / 4}")
                # response = sock.recv(1024).decode("utf-8")
                # print (response)

            except:
                print("fail to send data")
                connect = False

