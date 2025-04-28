import serial
import serial.tools.list_ports
import threading
import time
import sys
import re
import struct
import numpy as np
import cv2

MAX_DISTANCE_MM = 4000

class Log():
    class Level:
        NONE = 0
        ERROR = 1
        WARNING = 2
        INFO = 3
        DEBUG = 4

    def __init__(self, level=Level.DEBUG):
        self._level = level

    def i(self, msg):  
        if self._level >= self.Level.INFO:  print("I:", msg)
    def d(self, msg):  
        if self._level >= self.Level.DEBUG: print("D:", msg)
    def e(self, msg):  
        if self._level >= self.Level.ERROR: print("E:", msg)

log = Log(Log.Level.DEBUG)

class Tof:
    def __init__(self):
        self.serial = None
        self.data_thread = None
        self.running = False
        self.device_vidpid = None
        self._lock = threading.Lock()
        self.port = None

    def find_ports(self):
        ports = []
        pattern = re.compile(r'VID:PID=350E:(3723|3727|3729|3801)', re.IGNORECASE)
        for port in serial.tools.list_ports.comports():
            if pattern.search(port.hwid):
                vidpid = re.search(r'VID:PID=350E:(\d+)', port.hwid).group(1)
                ports.append((port.device, vidpid))
        return ports

    def try_open(self, port, vidpid):
        try:
            ser = serial.Serial(port, baudrate=115200, timeout=1)
            log.i(f"Opened serial port: {port}")
            if vidpid == "3801":
                log.i("Detected 3801 device, only sending 'settofconf'")
                ser.write(b"settofconf\r\n")
                resp = ser.readline().strip()
                log.d(f"Response: {resp}")
                if resp == b"OK":
                    self.serial = ser
                    self.device_vidpid = vidpid
                    self.port = port
                    return True
            else:
                for cmd in ["settofpwr 0", "settofpwr 1", "settofmode 1", "settofconf 0"]:
                    ser.write((cmd + "\r\n").encode())
                    resp = ser.readline().strip()
                    log.d(f"Response: {resp}")
                    if resp != b"OK":
                        ser.close()
                        return False
                    time.sleep(0.2)
                self.serial = ser
                self.device_vidpid = vidpid
                self.port = port
                return True
        except Exception as e:
            log.e(f"Failed to open {port}: {e}")
            return False

    def initialize(self):
        ports = self.find_ports()
        if not ports:
            log.e("No TOF device found.")
            sys.exit(1)

        log.i(f"Found {len(ports)} possible TOF device(s). Trying...")

        for port, vidpid in ports:
            log.i(f"Trying {port} (VIDPID={vidpid})...")
            if self.try_open(port, vidpid):
                log.i(f"Successfully initialized ToF on {port}")
                return True

        log.e("All ports failed to initialize.")
        sys.exit(1)

    def start_data_thread(self):
        self.running = True
        self.data_thread = threading.Thread(target=self._data_worker, daemon=True)
        self.data_thread.start()

    def stop(self):
        self.running = False
        if self.data_thread and self.data_thread.is_alive():
            self.data_thread.join()
        if self.serial and self.serial.is_open:
            self.serial.close()

    def _data_worker(self):
        while self.running:
            try:
                with self._lock:
                    data = self.serial.read_until(b'ED\r\n')
                if data.startswith(b'DATA') and data.endswith(b'ED\r\n') and len(data) >= 594:
                    handle_data(data, self.device_vidpid)
            except Exception as e:
                log.e(f"Data read error: {e}")

# === OpenCV Utilities ===
def getColorMap(dist_map, resolution=8, max_range=4000):
    colormap = np.zeros((resolution * 8, resolution * 8), dtype=np.uint8)
    for i in range(resolution):
        for j in range(resolution):
            for k in range(8):
                for l in range(8):
                    val = np.clip((dist_map[i, j] / max_range) * 255, 0, 255)
                    colormap[i * 8 + k, j * 8 + l] = val
    return cv2.applyColorMap(colormap, cv2.COLORMAP_JET)

def display(frame_name, frame, dist):
    resized = cv2.resize(frame, (480, 480), interpolation=cv2.INTER_NEAREST)
    for i in range(8):
        for j in range(8):
            text = f"{dist[i,j]:.0f}"
            cv2.putText(resized, text, (j * 60 + 15, i * 60 + 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255,255,255), 1)
    cv2.imshow(frame_name, resized)
    cv2.waitKey(1)

# === Gesture Map and Serial Output ===
GESTURE_MAP = {
    0: "UP", 1: "DOWN", 2: "LEFT", 3: "RIGHT",
    4: "PULL", 5: "PUSH", 6: "HALT"
}

SERIAL_OUTPUT = {
    "UP":    b"keyup\r\n",
    "DOWN":  b"keydown\r\n",
    "LEFT":  b"keyleft\r\n",
    "RIGHT": b"keyright\r\n",
    "PUSH":  b"keyback\r\n",
    "PULL":  b"keyenter\r\n"
}

# === Data Receiver ===
count = 0
last_gesture_map = 0

def handle_data(data, device_vidpid):
    global count, last_gesture_map
    count += 1
    if len(data) < 594:
        return

    checksum = int.from_bytes(data[-8:-4], byteorder='little')
    gesture_map = (checksum >> 8) & 0xFF

    dist_map = np.zeros((8,8), dtype=np.float32)
    for i in range(64):
        base = 10 + i*9
        target_status = data[base+8]
        if 5 <= target_status <= 9:
            distance_raw = struct.unpack("<h", data[base+5:base+7])[0]
            distance_mm = distance_raw if device_vidpid == "3801" else distance_raw // 4
            if distance_mm < 0:
                distance_mm = MAX_DISTANCE_MM
            dist_map[i//8, i%8] = min(distance_mm, MAX_DISTANCE_MM)
        else:
            dist_map[i//8, i%8] = MAX_DISTANCE_MM

    display("ToF Depth (mm)", getColorMap(dist_map), dist_map)

    # === Only non-3801 devices handle gestures
    if device_vidpid != "3801":
        if gesture_map != last_gesture_map:
            last_gesture_map = gesture_map
            for i in range(7):
                if gesture_map & (1 << i):
                    gesture = GESTURE_MAP.get(i, f"UNKNOWN_{i}")
                    if gesture in SERIAL_OUTPUT:
                        log.i(f"Detected gesture: {gesture}")
                        try:
                            with tof._lock:
                                tof.serial.write(SERIAL_OUTPUT[gesture])
                        except Exception as e:
                            log.e(f"Send key event failed: {e}")

# === Main ===
try:
    tof = Tof()
    tof.initialize()
    tof.start_data_thread()

    print("\n***** Press Ctrl+C to stop *****\n")
    start_time = time.time()
    last_time = start_time
    last_count = count

    while True:
        time.sleep(1)
        now = time.time()
        fps = (count - last_count) / (now - last_time)
        print(f"FPS: {fps:.2f}", end="\r")
        last_count = count
        last_time = now

except KeyboardInterrupt:
    print("\n\n[Ctrl+C detected]")

finally:
    tof.stop()
    cv2.destroyAllWindows()
