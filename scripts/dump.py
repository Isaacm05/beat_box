import serial

ser = serial.Serial("COM3", 115200)
out = open("waveform.csv", "w")

while True:
    try:
        line = ser.readline().decode().strip()
        out.write(line + "\n")
    except KeyboardInterrupt:
        break

out.close()
