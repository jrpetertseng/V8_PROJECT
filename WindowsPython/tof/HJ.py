import serial
import struct
import numpy as np
import cv2
import socket
import sys
import time
from enum import Enum

# Enum for output modes
class outPutMode(Enum):
    outCV = 0
    outTCP = 1
    noOutput = 2

class Tof:

    def __init__(self, port="COM1"):
        # Initialize serial port
        self.port = port
        self.serial = self._init_serial_port()
        self.buffer = bytearray()  # Data buffer
        self.run_flag = True  # Running flag
        self.start()  # Start transmission

    def _init_serial_port(self) -> serial.Serial:
        """Initialize serial port"""
        ser = serial.Serial()
        ser.port = self.port
        ser.timeout = 1
        try:
            ser.open()
        except:
            print(f"Failed to open port: {self.port}.")
            exit(1)
        return ser

    def start(self) -> None:
        """Start transmitting data"""
        self.send_command(command="settofconf")

    def send_command(self, command=None) -> None:
        """Send command to ToF module"""
        if command is None:
            command = input("Command to Module: ")
        self.serial.write(bytes(command + "\r\n", "ASCII"))
        self._get_response()

    def _get_response(self) -> None:
        """Catch response after command is sent"""
        self.serial.read_until(expected="\r\n")

    def getColorMap(self, Map: np.ndarray, Resolution: int):
        """Generate a color map for visualization"""
        colormap = np.zeros((Resolution * 8, Resolution * 8), dtype=np.uint8)
        for i in range(Resolution):
            for j in range(Resolution):
                for k in range(8):
                    for l in range(8):
                        colormap[i * 8 + k, j * 8 + l] = Map[i, j] / 4000 * 255
        color_depth = cv2.applyColorMap(colormap, cv2.COLORMAP_JET)
        return color_depth

    def display(self, frame_name: str, frame: np.ndarray, fps: float):
        """Display the frame without FPS on the image but print FPS to the console"""
        # Print the FPS to the console
        print(f"FPS: {fps:.2f}")

        # Display the frame in its original form
        cv2.imshow(frame_name, cv2.resize(frame, (640, 640), interpolation=cv2.INTER_NEAREST))

    def get_frame(self) -> np.ndarray:
        """Get a frame from ToF sensor"""
        try:
            self.buffer += self.serial.read(size=593)
            if len(self.buffer) < 593:
                return None
        except:
            print("Failed to read data.")
            return None
        
        ind = self.buffer.find(bytearray("DATA", encoding="ASCII"))
        self.buffer = self.buffer[ind:]
        self.buffer += self.serial.read(size=ind)
        if len(self.buffer) < 593:
            return None

        peak_rate_kcps_per_spad = np.zeros((8, 8), dtype=np.uintc)
        for i in range(64):
            peak_rate_kcps_per_spad[i // 8][i % 8] = struct.unpack("<I", self.buffer[9 * i + 11:9 * i + 15])[0]

        target_status = np.zeros((8, 8), dtype=np.ubyte)
        median_range = np.zeros((8, 8), dtype=np.short)
        for i in range(64):
            target_status[i // 8][i % 8] = struct.unpack("<B", self.buffer[9 * i + 18:9 * i + 19])[0]
            if 5 <= target_status[i // 8][i % 8] <= 9:
                median_range[i // 8][i % 8] = struct.unpack("<h", self.buffer[9 * i + 15:9 * i + 17])[0] / 10
            else:
                median_range[i // 8][i % 8] = 400

        self.buffer = bytearray()  # Clear the buffer
        return median_range

if __name__ == "__main__":
    # Socket and communication parameters
    host, port = "127.0.0.1", 25001
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connect = False
    disconnect_count = 0
    frame_count = 0
    comport = sys.argv[1]
    tof = Tof(comport)

    total_time = time.time()
    output_mode = outPutMode.outCV
    prev_time = time.time()

    while True:
        start_time = time.time()
        dist = tof.get_frame()
        
        if disconnect_count > 20:
            print("Disconnect")
            break
        if dist is None:
            disconnect_count += 1
            continue
        else:
            disconnect_count = 0
            frame_count += 1

        # Calculate FPS
        current_time = time.time()
        fps = 1 / (current_time - prev_time)
        prev_time = current_time

        match output_mode:
            case outPutMode.noOutput:
                if frame_count % 20 == 0:
                    print(f"FPS: {fps:.2f}")
                    frame_count = 0
            case outPutMode.outCV:
                color_depth = tof.getColorMap(dist, 8)
                tof.display('Depth', color_depth, fps)
                if cv2.waitKey(1) == ord('q'):
                    break
            case outPutMode.outTCP:
                try:
                    if not connect:
                        sock.connect((host, port))
                        connect = True

                    sock.sendall(dist.tobytes())

                except:
                    print("Failed to send data")
                    connect = False

    print(f"Total_time: {time.time() - total_time}")
