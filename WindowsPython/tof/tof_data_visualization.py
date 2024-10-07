import serial
import struct
import numpy as np
import cv2
import socket
import sys
import time
import threading
from typing import Optional
from enum import Enum


# Enum to define output modes for ToF data
class outPutMode(Enum):
    outCV = 0  # Display data using OpenCV visualization
    outTCP = 1  # Send data over TCP connection
    noOutput = 2  # No output mode


# Class to handle ST VL53L8CX Time-of-Flight (ToF) sensor with a maximum range of 4000 mm
class Tof:
    def __init__(self, port="COM1", max_range=4000, invalid_value=400):
        """
        Initializes the VL53L8CX ToF sensor on a specified serial port.

        Parameters:
        - port: The serial port connected to the VL53L8CX sensor (e.g., "COM1").
        - max_range: The maximum detection range in mm (default 4000 mm).
        - invalid_value: The value (in cm) to represent invalid or out-of-range data, default is 400 cm.
        """
        self.port = port
        self.serial = self._init_serial_port()

        if self.serial is None:
            print(f"Unable to initialize the serial port {self.port}. Exiting.")
            sys.exit(1)

        self.buffer = bytearray()
        self.run_flag = True
        self.max_range = max_range
        self.invalid_value = invalid_value  # Invalid range value is set to 400 cm
        self.lock = threading.Lock()  # Protect access to serial port across threads
        self.stop_flag = False  # Stop flag for controlling thread execution
        self.start()

    def _init_serial_port(self) -> Optional[serial.Serial]:
        """
        Initializes the serial port for communication with the ToF sensor.
        Returns None if the port cannot be opened.
        """
        ser = serial.Serial()
        ser.port = self.port
        ser.timeout = 1  # Set a 1-second timeout for reading
        try:
            ser.open()
            if ser.is_open:
                print(f"Successfully opened serial port: {self.port}")
            else:
                print(f"Failed to open port: {self.port}")
                return None
        except serial.SerialException as e:
            print(f"Failed to open port: {self.port}. Error: {e}")
            return None
        return ser

    def start(self) -> None:
        """Starts the communication with the ToF sensor by sending a configuration command."""
        self.send_command(command="settofconf")

    def send_command(self, command=None) -> None:
        """
        Sends a command to the ToF sensor via the serial interface.
        If no command is provided, prompts the user to input one.
        """
        if self.serial is None:
            print("Serial port is not initialized. Cannot send command.")
            return

        if command is None:
            command = input("Command to Module: ")

        try:
            self.serial.write(bytes(command + "\r\n", "ASCII"))
            self._get_response()
        except serial.SerialException as e:
            print(f"Failed to send command over serial port. Error: {e}")

    def _get_response(self) -> None:
        """Reads and handles the response from the ToF sensor after sending a command."""
        if self.serial is None or not self.serial.is_open:
            print("Serial port is not initialized or not open. Cannot read response.")
            return

        try:
            self.serial.read_until(expected=b"\r\n")  # Read response until newline
        except serial.SerialException as e:
            print(f"Failed to read response from serial port. Error: {e}")

    def getColorMap(self, Map: np.ndarray, Resolution: int):
        """
        Generates a color map based on the distance data received from the ToF sensor.
        The values are mapped to a color range for visualization.

        Parameters:
        - Map: The 8x8 distance map array (in cm).
        - Resolution: The grid resolution (e.g., 8x8).

        Returns:
        - A color-mapped visualization using OpenCV's COLORMAP_JET.
        """
        colormap = np.zeros((Resolution * 8, Resolution * 8), dtype=np.uint8)
        for i in range(Resolution):
            for j in range(Resolution):
                for k in range(8):
                    for l in range(8):
                        # Normalize the value between 0 and 255 for color mapping
                        value = np.clip((Map[i, j] / self.max_range) * 255, 0, 255)
                        colormap[i * 8 + k, j * 8 + l] = value
        return cv2.applyColorMap(colormap, cv2.COLORMAP_JET)

    def display(self, frame_name: str, frame: np.ndarray, fps: float, dist: np.ndarray):
        """
        Displays the color map with distance values and FPS overlay.

        Parameters:
        - frame_name: Name of the display window.
        - frame: The color-mapped frame to display.
        - fps: Frames per second for the display.
        - dist: The distance data (in cm) to overlay on the frame.
        """
        print(f"FPS: {fps:.2f}")
        resized_frame = cv2.resize(frame, (640, 640), interpolation=cv2.INTER_NEAREST)
        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 0.5
        color = (255, 255, 255)
        thickness = 1
        for i in range(dist.shape[0]):
            for j in range(dist.shape[1]):
                text = f"{dist[i, j]:.1f}"  # Display distance in cm
                position = (j * 80 + 20, i * 80 + 40)
                cv2.putText(
                    resized_frame, text, position, font, font_scale, color, thickness
                )
        cv2.imshow(frame_name, resized_frame)
        if cv2.waitKey(1) == ord("q"):
            return False
        return True

    def get_frame(self) -> Optional[np.ndarray]:
        """
        Reads a frame of distance data from the ToF sensor. The raw data is in millimeters (mm),
        but is converted to centimeters (cm) by dividing by 10.

        Returns:
        - A numpy array of distances (in cm).
        """
        with self.lock:
            if self.stop_flag:
                return None
            if self.serial is None or not self.serial.is_open:
                print("Serial port is not open or not available.")
                return None

            try:
                self.buffer += self.serial.read(
                    size=593
                )  # Read a frame of data (593 bytes)
                if len(self.buffer) < 593:
                    return None
            except serial.SerialException as e:
                print(f"Failed to read data from serial port. Error: {e}")
                return None
            except TypeError as e:
                print(f"Type error encountered: {e}")
                return None

            ind = self.buffer.find(bytearray("DATA", encoding="ASCII"))
            self.buffer = self.buffer[ind:]
            self.buffer += self.serial.read(size=ind)
            if len(self.buffer) < 593:
                return None

            peak_rate_kcps_per_spad = np.zeros((8, 8), dtype=np.uintc)
            for i in range(64):
                peak_rate_kcps_per_spad[i // 8][i % 8] = struct.unpack(
                    "<I", self.buffer[9 * i + 11 : 9 * i + 15]
                )[0]

            target_status = np.zeros((8, 8), dtype=np.ubyte)
            median_range = np.zeros((8, 8), dtype=np.short)
            for i in range(64):
                target_status[i // 8][i % 8] = struct.unpack(
                    "<B", self.buffer[9 * i + 18 : 9 * i + 19]
                )[0]
                if 5 <= target_status[i // 8][i % 8] <= 9:
                    # Convert the distance from mm to cm by dividing by 10
                    median_range[i // 8][i % 8] = (
                        struct.unpack("<h", self.buffer[9 * i + 15 : 9 * i + 17])[0]
                        / 10
                    )
                else:
                    # Assign invalid value in cm (default 400 cm)
                    median_range[i // 8][i % 8] = self.invalid_value
            self.buffer = bytearray()  # Clear the buffer after reading
            return median_range

    def close(self):
        """
        Safely closes the serial port. Ensures no other thread is using it.
        """
        with self.lock:
            if self.serial is not None and self.serial.is_open:
                self.serial.close()
                print(f"Serial port {self.port} closed successfully.")


def display_thread_func(tof, prev_time):
    """
    Thread function to handle OpenCV display of sensor data.
    Continuously reads frames from the sensor and updates the display window.
    """
    while not tof.stop_flag:
        dist = tof.get_frame()
        if dist is None:
            continue

        current_time = time.time()
        fps = 1 / (current_time - prev_time)
        prev_time = current_time

        color_depth = tof.getColorMap(dist, 8)
        if not tof.display("Depth", color_depth, fps, dist):
            tof.stop_flag = True
            break
    cv2.destroyAllWindows()


def tcp_thread_func(tof, host, port):
    """
    Thread function for sending sensor data over TCP.
    Connects to a remote server and transmits the distance data continuously.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connect = False
    try:
        sock.connect((host, port))
        connect = True
    except Exception as e:
        print(f"Failed to connect to {host}:{port}. Error: {e}")
        return

    while connect and not tof.stop_flag:
        dist = tof.get_frame()
        if dist is None:
            continue
        try:
            sock.sendall(dist.tobytes())
        except Exception as e:
            print(f"Failed to send data. Error: {e}")
            connect = False
            break
    sock.close()


if __name__ == "__main__":
    host, port = "127.0.0.1", 25001
    comport = sys.argv[1]
    tof = Tof(comport)

    output_mode = outPutMode.outCV
    prev_time = time.time()

    try:
        if output_mode == outPutMode.outCV:
            display_thread = threading.Thread(
                target=display_thread_func, args=(tof, prev_time)
            )
            display_thread.start()

        elif output_mode == outPutMode.outTCP:
            tcp_thread = threading.Thread(
                target=tcp_thread_func, args=(tof, host, port)
            )
            tcp_thread.start()

        # Main thread waits for KeyboardInterrupt
        display_thread.join()
        if output_mode == outPutMode.outTCP:
            tcp_thread.join()

    except KeyboardInterrupt:
        print("Program interrupted.")
    finally:
        tof.stop_flag = True  # Signal all threads to stop
        tof.close()  # Close serial port after threads are done
        cv2.destroyAllWindows()
