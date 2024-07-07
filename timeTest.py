import serial
import time

ser = serial.Serial('COM4', baudrate=115200)

startTime = time.time
prev = time.time()

i = 0
try:
    start_time = time.time()

    while(1):

        prev = time.time()
        uart_data = ser.readline().decode().strip().strip('\0')
        # print(uart_data)

        if i %4 == 0:

            end_time = time.time()
            execution_time = end_time - start_time
            start_time = end_time
            print("Execution time:", execution_time, "seconds")
        i+=1

except KeyboardInterrupt:
    print("Data collection stopped.")