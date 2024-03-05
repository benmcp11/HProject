from matplotlib.animation import FuncAnimation
from collections import deque
import time
from re import L
import os
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



plt.ion()  # Turn on interactive mode
fig, ax = plt.subplots()
x_data = deque(maxlen=10)  # Keep only the last 10 data points
y_data = deque(maxlen=10)
line, = ax.plot(x_data, y_data, '-xb')

x_data_av = deque(maxlen=10)  # Keep only the last 10 data points
y_data_av = deque(maxlen=10)
line_av, =  ax.plot(x_data_av, y_data_av, '-xm')

displayCircle=  1
circleData = []
for i in range(4):
    circleData.append([0,0])

circle0, = ax.plot(circleData[0][0], circleData[0][1], '-')
circle1, = ax.plot(circleData[1][0], circleData[1][1], '-')
circle2, = ax.plot(circleData[2][0], circleData[2][1], '-')
circle3, = ax.plot(circleData[3][0], circleData[3][1], '-')

rx0 = (0, 0.35, 0.9)

rx1 = (0,3.03, 0.8)

rx2 = (0 ,4.32,0.8)

rx3 = (0, 7.35, 0.95)

known_points = np.array([rx0, rx1, rx2, rx3])

for point in known_points:
    plt.plot(point[0], point[1], f'xr',markersize=10)
    if i == 2:
        plt.plot(point[0], point[1], f'xr',markersize=10, label = "Rx Nodes")
    i+=1


angles = np.linspace(0, 2*np.pi, 100)


n = 3.8

folder_path = "/Users/benmcpartlin/Desktop/Uni/Year4/Project/Code/HProject/ExperimentData20"
csv_files = [f for f in os.listdir(folder_path) if f.endswith('.csv')]

# Loop through the CSV files

dic = {}
rxRssiDic = {}
distancesList = []
pointer = []


rssiList = [[],[],[],[]]

# RxNID, TXNID, PacketNo, RSSI
for csv_file in csv_files:
    file_path = os.path.join(folder_path, csv_file)
    print(file_path)
    with open(file_path, newline='') as csvfile:
        reader = csv.reader(csvfile)
        next(reader, None)

        splitLine = file_path.split("_")

        key = splitLine[-1]
        dic[key] = [[],[],[],[]]

        rx = [[],[],[],[]] 
        rxRssiDic[key] = {}
        noRx = 4
        for i in range(noRx):
            rxRssiDic[key][i] = []
            pointer.append(0)
            distancesList.append(0)
            for j in range(noRx):
                rxRssiDic[key][i].append(j)

        print(rxRssiDic)
        j = 0
    

        avNo = 20
        rollingAv = []
        for i in range(noRx):
            averages = [0 for i in range(avNo)]
            rollingAv.append(averages)

        for row in reader:

            if len(row) == 5:

                rxNID = int(row[0])

                rx[rxNID].append(int(row[3]))
                rxRssiDic[key][rxNID]  = []
                r4 = row[4].strip("[").strip("]").split(", ")

            
                for i, r in enumerate(r4):
                    if r[1:-1] != '':
                        rxRssiDic[key][rxNID].append(int(r[1:-1]))
                    else:
                        rxRssiDic[key][rxNID].append(int(r))
                
                nDicS, n = getN.knownNValues(known_points,rxRssiDic, limit = 2)

                print(getN.knownNValues(known_points,rxRssiDic, show = 0))
                print(getN.weightedN(nDicS,rxNID, limit = 2))

                weightedN = getN.weightedN(nDicS,rxNID, limit=2)

                # if weightedN>2:
                #     n = getN.weightedN(nDicS,rxNID, limit = 2)

                distancesList[rxNID] = estimateDistance(int(row[3]), n)

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
                # Adjust plot limits if needed
               
                
                if rxNID== 0:
                    coordinates = multilaterate(known_points, distancesList)

                    x_data.append(coordinates[0])
                    y_data.append(coordinates[1])

                    # Update plot
                    line.set_xdata(x_data)
                    line.set_ydata(y_data)
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
                    time.sleep(0.10)


        i = 0
        # for l in rx:
        #     dic[key][i] = int(np.mean(rx[i]))
        #     i+=1
