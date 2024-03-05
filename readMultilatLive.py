from re import L
import serial
import csv
import time
from scipy.optimize import minimize
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import getDyamicNVals as getN

n = 3.8
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
line, = ax.plot(x_data, y_data, 'b-x', label = 'Raw')

x_data_av = deque(maxlen=10)  # Keep only the last 10 data points
y_data_av = deque(maxlen=10)
line_av, =  ax.plot(x_data_av, y_data_av, '-xm', label = 'Rolling Av')

displayCircle=  0
circleData = []
for i in range(4):
    circleData.append([0,0])

circle0, = ax.plot(circleData[0][0], circleData[0][1], '-')
circle1, = ax.plot(circleData[1][0], circleData[1][1], '-')
circle2, = ax.plot(circleData[2][0], circleData[2][1], '-')
circle3, = ax.plot(circleData[3][0], circleData[3][1], '-')

angles = np.linspace(0, 2*np.pi, 100)


wallThick = 0.125

room1 = {'xMin':0, 'xMax': 3, 'yMin':0, 'yMax':4.62}
room2 = {'xMax':-1.6-wallThick*2, 'xMin':-1.6-wallThick*2-4.3 , 'yMin':0, 'yMax':2.9}
room3 = {'xMax':-1.6-wallThick*2, 'xMin': -1.6-wallThick*2-4.3, 'yMin':2.9+1.8+0.59+wallThick, 'yMax':2.9+1.8+0.59+wallThick+3.2}
room0 = {'xMin':-0.72, 'xMax': -0.72+3.72, 'yMin':4.62+0.65, 'yMax':4.62+0.65+3.24}

bathM = {'xMax':-1.6-wallThick*3-0.87, 'xMin': -1.6-wallThick*2-4.3, 'yMin':2.9+wallThick, 'yMax':2.9+wallThick+1.8}
bathL = {'xMax':-wallThick, 'xMin':-wallThick-1.6, 'yMin':1.8+wallThick, 'yMax':1.8+wallThick+1.65}
bathB = {'xMax':-wallThick, 'xMin':-wallThick-1.6, 'yMin':0, 'yMax':1.8}

def setRoomsParams(roomsDic): 

    for room in roomsDic.keys():
        for key in roomsDic[room].keys():
            roomsDic[room][key] = round(roomsDic[room][key],2)
    return roomsDic

# roomsDic1 = setRoomsParams({"R1": room1})
# roomsDic2 = setRoomsParams({"R1": room1,"R0":room0})
roomsDic = setRoomsParams({"R1": room1, "R2": room2, "R3":room3, "R0":room0, "BM": bathM, "BL": bathL, "BB": bathB })

for key in roomsDic.keys():

    plt.plot([roomsDic[key]["xMin"],roomsDic[key]["xMax"]], [roomsDic[key]["yMin"],roomsDic[key]["yMin"]], 'k-')
    plt.plot([roomsDic[key]["xMin"],roomsDic[key]["xMax"]], [roomsDic[key]["yMax"],roomsDic[key]["yMax"]], 'k-')
    
    plt.plot([roomsDic[key]["xMin"],roomsDic[key]["xMin"]], [roomsDic[key]["yMin"],roomsDic[key]["yMax"]], 'k-')
    plt.plot([roomsDic[key]["xMax"],roomsDic[key]["xMax"]], [roomsDic[key]["yMin"],roomsDic[key]["yMax"]], 'k-')

colours = ['c', 'g', 'm']
i = 0

rx0 = (1.26, 1.95, 2.1)
rx0 = (rx0[0]-0.72, rx0[1]+4.62+0.65, rx0[2])

rx1 = (1.3,1.7,2.1)

rx2 = (2.15,1.56,2.1)
rx2 = (-rx2[0]-1.6-wallThick*2, rx2[1], rx2[2])

rx3 = (2.32, 1.9, 1.75)
rx3 = (-rx3[0]-1.6-wallThick*2, rx3[1]+2.9+1.8+0.59+wallThick, rx3[2])
known_points = np.array([rx0, rx1, rx2, rx3])

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
ty = 'Demo'
r = 'Room0'

csv_filename = f'ExperimentData21\Exp21_Multilat_Live_{ty}_{r}.csv'

csv_header = ['RxNID', 'TXNID', 'PacketNo', 'TxRSSI', 'RSSI0','RSSI1','RSSI2','RSSI3']

rxRssiDic = {}
noRx = 4

pointer = []
for i in range(noRx):
    rxRssiDic[i] = []
    pointer.append(0)

    for j in range(noRx):
        rxRssiDic[i].append(j)

avNo = 20
rollingAv = []
for i in range(noRx):
    averages = [0 for i in range(avNo)]
    rollingAv.append(averages)

with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)

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

                # rxRssi2 = {"KEY":rxRssiDic}
                # nDicS, n = getN.knownNValues(known_points,rxRssi2, limit = 2)

                # print(getN.knownNValues(known_points,rxRssi2, show = 0))
                # print(getN.weightedN(nDicS,rxNID, limit = 2))

                # weightedN = getN.weightedN(nDicS,rxNID, limit=2)

                # if weightedN>3:
                #     n = getN.weightedN(nDicS,rxNID, limit = 2)

                distancesList[int(rxNID)] = estimateDistance(rssiTotal/5, n)

                rollingAv[rxNID][pointer[rxNID]] = distancesList[rxNID]

                pointer[rxNID]+=1

                if pointer[rxNID] == avNo:
                    pointer[rxNID] = 0
                

                radius = distancesList[rxNID]
                x = known_points[rxNID][0] + radius * np.cos(angles)
                y = known_points[rxNID][1] + radius * np.sin(angles)

                # Update plot

                circleData[rxNID][0] = x
                circleData[rxNID][1] = y

                if int(rxNID) ==0:

                    coordinates = multilaterate(known_points, distancesList)
                    
                    x_data.append(coordinates[0])
                    y_data.append(coordinates[1])

                    # Update plot
                    line.set_xdata(x_data)
                    line.set_ydata(y_data)

                    # ax.plot(coordinates[0], coordinates[1],'bx')
                    if 0 not in rollingAv[0]:
                        coordinates_av =  multilaterate(known_points, [np.mean(dList) for dList in rollingAv])
                        x_data_av.append(coordinates_av[0])
                        y_data_av.append(coordinates_av[1])

                        
                        line_av.set_xdata(x_data_av)
                        line_av.set_ydata(y_data_av)

                    if displayCircle:

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