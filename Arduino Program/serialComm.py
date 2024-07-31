import serial

ser = serial.Serial('/dev/cu.usbmodem1421')  # open serial port
print(ser.name)         # check which port was really used
ser.write(b'<home_135>')     # write a string
ser.close() 