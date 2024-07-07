import serial
import csv
import time

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

# Serial port settings (modify the port name as needed)
ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

# CSV file settings
expNo = 22
hdis = 1.5
rxHeight1 = 2.03
txHeight1 = 1
shift1 = 1
testNo =9
csv_filename = f'ExperimentData22\Ex{expNo}_{testNo}.csv'
csv_header = ['NodeID', 'PacketNo',  'RSSI','PacketNo', 'AbsTime']
def estimateDistance(rssi, n):
    distance = 10 ** ((-20 - rssi) / (10 * n))
    return distance

plt.ion()  # Turn on interactive mode
fig, ax = plt.subplots()
x_data = deque(maxlen=500)  # Keep only the last 100 data points
y_data = deque(maxlen=500)
line, = ax.plot(x_data, y_data, '-o', label = 'normal')

x_data_av = deque(maxlen=500)  # Keep only the last 10 data points
y_data_av = deque(maxlen=500)
line_av, =  ax.plot(x_data_av, y_data_av, '-', label = 'Rolling Av')

av_no = 10
rolling_av = [-20 for i in range(av_no)]
pointer = 0

plt.show()

# Create or open the CSV file for writing
with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)

    try:
        i =0
        while True:
            # Read data from UART
            uart_data = ser.readline().decode().strip().strip('\0')

            # Split the received data into fields
            data_fields = uart_data.split('_')

            # Check if the received data has the correct format
            if len(data_fields) == 6:
                node_id,ty,rssi, freq, packetNo, absTime = data_fields

                # Write data to the CSV file
                csv_writer.writerow([node_id,rssi, freq, packetNo, absTime])

                # # Flush the CSV file to ensure data is written immediately
                csvfile.flush()

                print(f"Data received: NodeID={node_id}, AbsoluteTime={absTime}, RSSI={rssi}, PacketNo={packetNo}")

                rssi = int(rssi)
                packetNo = int(packetNo)
                

                distance = estimateDistance(rssi, 3)

                x_data.append(distance)
                y_data.append(rssi)

                # line.set_xdata(x_data)
                # line.set_ydata(y_data)

                rolling_av[pointer] = rssi
                pointer+=1
                if pointer == av_no:
                    pointer = 0

                distance_av = estimateDistance(np.mean(rolling_av), 2)
                x_data_av.append(i)
                y_data_av.append(np.mean(rolling_av))

                
                line_av.set_xdata(x_data_av)
                line_av.set_ydata(y_data_av)

                ax.relim()
                ax.autoscale_view()

                # Update plot
                fig.canvas.draw()
                fig.canvas.flush_events()
                i+=1
               
            else:
                print("Invalid data format received:", uart_data)

    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()
