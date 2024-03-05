from re import L
import serial
import csv
import time
from scipy.optimize import minimize
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

def estimateDistance(rssi, n):
    distance = 10 ** ((-20 - rssi) / (10 * n))
    return distance

def distance(p1, p2):
    """Calculate the Euclidean distance between two 3D points."""
    return np.linalg.norm(np.array(p1) - np.array(p2))

def objective_function(x, known_points, distances):
    """Objective function for optimization."""
    return sum([(distance(x, known_points[i]) - distances[i])**2 for i in range(len(known_points))])

def multilaterate(known_points, distances):
    """Perform 3D trilateration."""
    initial_guess = np.mean(known_points, axis=0)  # Initial guess as the centroid of known points
    result = minimize(objective_function, initial_guess, args=(known_points, distances), method='Nelder-Mead')
    
    return result.x


# Set Up plot

plt.ion()  # Turn on interactive mode
fig, ax = plt.subplots()
x_data = deque(maxlen=5)  # Keep only the last 100 data points
y_data = deque(maxlen=5)
line, = ax.plot(x_data, y_data, 'b-x')


displayCircle=  0
circleData = []
for i in range(4):
    circleData.append([0,0])

circle0, = ax.plot(circleData[0][0], circleData[0][1], '-')
circle1, = ax.plot(circleData[1][0], circleData[1][1], '-')
circle2, = ax.plot(circleData[2][0], circleData[2][1], '-')
circle3, = ax.plot(circleData[3][0], circleData[3][1], '-')

noRx = 4

colours = ['c', 'g', 'm']
i = 0

rx0 = (0, 0.35, 0.9)

rx1 = (0,3.03, 0.8)

rx2 = (0 ,4.32,0.8)

rx3 = (0, 7.35, 0.95)
known_points = np.array([rx0, rx1, rx2, rx3])


known_points = known_points[:noRx]
for point in known_points:
    plt.plot(point[0], point[1], f'xr',markersize=10)
    if i == 2:
        plt.plot(point[0], point[1], f'xr',markersize=10, label = "Rx Nodes")
    i+=1

plt.show()




ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

txX = 1
txY = 1
txZ = 0.75

n = 2.8

testNo = 9

csv_filename = f'ExperimentData20\Exp20_Multilat_Live{testNo}.csv'

csv_header = ['RxNID', 'TXNID', 'PacketNo', 'TxRSSI', 'RSSI0','RSSI1','RSSI2','RSSI3']

rxRssiDic = {}
for i in range(noRx):
    rxRssiDic[i] = []
    for j in range(noRx):
        rxRssiDic[i].append(j)

with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)
    angles = np.linspace(0, 2*np.pi, 100)

    try:
        count = 0
        rssiList = [[],[],[],[]]
        distancesList = [0,0,0,0]
        while True:
            # Read data from UART
            uart_data = ser.readline().decode().strip().strip('\0')

            print(uart_data)
            # Split the received data into fields
            split_data = uart_data.split(',')
            rssiTotal = 0
            # Check if the received data has the correct format
            if len(split_data) >= 7:
                rxNID = int(split_data[0])
                rxRSSI = split_data[6]
                if rxNID != 0:
                    rxRssiDic[0][rxNID] = rxRSSI

                    rxRssiDic[rxNID] = split_data[7].split('_')

                    

                for i in range(1,6):
                    txNID, rssi, pNo =  split_data[i].split('_')

                    # Write data to the CSV file

                    output = [rxNID, txNID, pNo, rssi]
                    output.append(rxRssiDic[rxNID])
                    # print(output)
                    csv_writer.writerow(output)

                    # # Flush the CSV file to ensure data is written immediately
                    csvfile.flush()
                    rssiTotal+=int(rssi)

                rssiList[int(rxNID)] = rssiTotal/5
                distancesList[int(rxNID)] = estimateDistance(rssiTotal/5, n)

                radius = distancesList[int(rxNID)]
                x = known_points[int(rxNID)][0] + radius * np.cos(angles)
                y = known_points[int(rxNID)][1] + radius * np.sin(angles)
                circleData[int(rxNID)][0] = x
                circleData[int(rxNID)][1] = y

                if int(rxNID) ==0:

                    coordinates = multilaterate(known_points, distancesList[:noRx])
                    
                    x_data.append(coordinates[0])
                    y_data.append(coordinates[1])

                    # Update plot
                    line.set_xdata(x_data)
                    line.set_ydata(y_data)


                    for i in range(len(circleData)):
                        match i:
                            case 0:
                                circle0.set_xdata(circleData[i][0])
                                circle0.set_ydata(circleData[i][1])
                            case 1:
                                circle1.set_xdata(circleData[i][0])
                                circle1.set_ydata(circleData[i][1])
                            case 2:
                                circle2.set_xdata(circleData[i][0])
                                circle2.set_ydata(circleData[i][1])
                            case 3:
                                circle3.set_xdata(circleData[i][0])
                                circle3.set_ydata(circleData[i][1])

                    # ax.plot(coordinates[0], coordinates[1],'bx')

                    # Adjust plot limits if needed
                    ax.relim()
                    ax.autoscale_view()

                    # Update plot
                    fig.canvas.draw()
                    fig.canvas.flush_events()
                    # if count>20:
                    #     break
                    # count+=1

                    # if count>20:
                    #     break
                    # count+=1
            else:
                print("Invalid data format received:", uart_data)

    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()