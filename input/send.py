import keyboard
import struct
import serial
import time
import psutil, os
p = psutil.Process(os.getpid())
p.nice(psutil.HIGH_PRIORITY_CLASS)

arduino = serial.Serial(port='COM10', baudrate=115200, timeout=1) 

def gen_output():
    up = keyboard.is_pressed('up')
    down = keyboard.is_pressed('down')
    left = keyboard.is_pressed('left')
    right = keyboard.is_pressed('right')
    a = keyboard.is_pressed('z')
    b = keyboard.is_pressed('c')
    start = keyboard.is_pressed('x')
    byte = 0
    byte += 1 if    up else 0
    byte += 2 if  down else 0
    byte += 4 if  left else 0
    byte += 8 if right else 0
    byte += 16 if     a else 0
    byte += 32 if     b else 0
    byte += 64 if start else 0
    return byte

while 1:
    data = arduino.readline()
    if data[:5] == b'input':
        output = gen_output()
        arduino.write(struct.pack('!B', output))
    elif len(data):
        print(data)