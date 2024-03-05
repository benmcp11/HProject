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

displayCircle=  0
circleData = []
for i in range(4):
    circleData.append([0,0])

circle0, = ax.plot(circleData[0][0], circleData[0][1], '-')
circle1, = ax.plot(circleData[1][0], circleData[1][1], '-')
circle2, = ax.plot(circleData[2][0], circleData[2][1], '-')
circle3, = ax.plot(circleData[3][0], circleData[3][1], '-')


wallThick = 0.125

lenroom1 = 4.7
concWallThick = 60


room1 = {'xMin':0, 'xMax': 3, 'yMin':0, 'yMax':4.62}
room1 = [[0,3, 3, -0.72, -0.72, 0,0], [0,0,lenroom1,lenroom1,3+0.63+wallThick,3+0.63+wallThick,0]]


room2 = {'xMax':-1.6-wallThick*2, 'xMin':-1.6-wallThick*2-4.3 , 'yMin':0, 'yMax':2.9}
xList = [-1.6-wallThick*2,-1.6-wallThick*2-4.3,-1.6-wallThick*2-4.3, -1.6-wallThick*2-0.87, -1.6-wallThick*2-0.87 , -1.6-wallThick*2,  -1.6-wallThick*2]
yList = [0,0,2.9,2.9,4.7+wallThick,4.7+wallThick,0]
room2 = [xList,yList]


room3 = {'xMax':-1.6-wallThick*2, 'xMin': -1.6-wallThick*2-4.3, 'yMin':2.9+1.8+0.59+wallThick, 'yMax':2.9+1.8+0.59+wallThick+3.2}

xList = [room3['xMin'], room3['xMax'], room3['xMax'],  room3['xMin'], room3['xMin']]
yList = [room3['yMin'], room3['yMin'], room3['yMax'], room3['yMax'], room3['yMin']]
room3 = [xList, yList]


room0 = {'xMin':-0.72, 'xMax': -0.72+3.72, 'yMin':lenroom1+0.65, 'yMax':lenroom1+0.65+3.24}
xList = [room0['xMin'], room0['xMax'], room0['xMax'],  room0['xMin'], room0['xMin']]
yList = [room0['yMin'], room0['yMin'], room0['yMax'], room0['yMax'], room0['yMin']]
room0 = [xList, yList]


bathM = {'xMax':-1.6-wallThick*3-0.87, 'xMin': -1.6-wallThick*2-4.3, 'yMin':2.9+wallThick, 'yMax':2.9+wallThick+1.8}
xList = [bathM['xMin'], bathM['xMax'], bathM['xMax'],  bathM['xMin'], bathM['xMin']]
yList = [bathM['yMin'], bathM['yMin'], bathM['yMax'], bathM['yMax'], bathM['yMin']]
bathM = [xList, yList]

bathL = {'xMax':-wallThick, 'xMin':-wallThick-1.6, 'yMin':1.8+wallThick, 'yMax':1.8+wallThick+1.65}
xList = [bathL['xMin'], bathL['xMax'], bathL['xMax'],  bathL['xMin'], bathL['xMin']]
yList = [bathL['yMin'], bathL['yMin'], bathL['yMax'], bathL['yMax'], bathL['yMin']]
bathL = [xList, yList]

bathB = {'xMax':-wallThick, 'xMin':-wallThick-1.6, 'yMin':0, 'yMax':1.8}
xList = [bathB['xMin'], bathB['xMax'], bathB['xMax'],  bathB['xMin'], bathB['xMin']]
yList = [bathB['yMin'], bathB['yMin'],bathB['yMax'], bathB['yMax'], bathB['yMin']]
bathB = [xList, yList]

def setRoomsParams(roomsDic): 

    for room in roomsDic.keys():
        for key in roomsDic[room].keys():
            roomsDic[room][key] = round(roomsDic[room][key],2)
    return roomsDic

# roomsDic1 = setRoomsParams({"R1": room1})
# roomsDic1 = {"R1":room1, "R2":room2, "R3":room3, "R0":room0}
# roomsDic2 = setRoomsParams({"R1": room1,"R0":room0})
roomsDic = {"R1": room1, "R2": room2, "R3":room3, "R0":room0, "BM": bathM, "BL": bathL, "BB": bathB }

for key in roomsDic.keys():
    plt.plot(roomsDic[key][0],roomsDic[key][1], '-k' )

plt.plot([room2[0][0], room2[0][0]], [room2[1][0], room3[1][-2]], 'k')
plt.plot([room2[0][1], room2[0][1]], [room2[1][0], room3[1][-2]], 'k')

plt.plot([room1[0][1], room1[0][1]], [0, room0[1][-2]], 'k')
plt.plot([room1[0][1], room2[0][1]], [0,0], 'k')
plt.plot([0, room2[0][0]], [3+0.63+wallThick,3+0.63+wallThick], 'k')
plt.plot([0, room2[0][0]], [3+0.63+wallThick,3+0.63+wallThick], 'k')
plt.plot([0, room2[0][0]], [3+0.63+wallThick,3+0.63+wallThick], 'k')
plt.plot([room2[0][1], room2[0][0]], [bathM[1][2], bathM[1][2]], 'k')

plt.plot([room1[0][-3], room1[0][-3]], [3+0.63+wallThick,8], 'k')

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


angles = np.linspace(0, 2*np.pi, 100)



n = 3.8

folder_path = "/Users/benmcpartlin/Desktop/Uni/Year4/Project/Code/HProject/ExperimentData19"
csv_files = [f for f in os.listdir(folder_path) if f.endswith('.csv')]

# Loop through the CSV files

dic = {}
rxRssiDic = {}
distancesList = [[],[],[],[]]
rssiList = [[],[],[],[]]

# RxNID, TXNID, PacketNo, RSSI
for csv_file in csv_files:
    file_path = os.path.join(folder_path, csv_file)
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
            for j in range(noRx):
                rxRssiDic[key][i].append(j)
        
        j = 0
        pointer = 0

        avNo = 10
        rollingAv = []
        for i in range(noRx):
            averages = [0 for i in range(avNo)]
            rollingAv.append(averages)

        for row in reader:

            if len(row) == 5:
                rx[int(row[0])].append(int(row[3]))
                rxRssiDic[key][int(row[0])]  = []
                r4 = row[4].strip("[").strip("]").split(", ")

            
                for i, r in enumerate(r4):
                    if r[1:-1] != '':
                        rxRssiDic[key][int(row[0])].append(int(r[1:-1]))
                    else:
                        rxRssiDic[key][int(row[0])].append(int(r))

                distancesList[int(row[0])] = estimateDistance(int(row[3]), n)

                rollingAv[int(row[0])][pointer] = distancesList[int(row[0])]

                print(rollingAv[int(row[0])][pointer])
                pointer = (pointer+1)

                if pointer == avNo:
                    pointer = 0


                radius = distancesList[int(row[0])]
                x = known_points[int(row[0])][0] + radius * np.cos(angles)
                y = known_points[int(row[0])][1] + radius * np.sin(angles)

                # Update plot

                circleData[int(row[0])][0] = x
                circleData[int(row[0])][1] = y
                # Adjust plot limits if needed
               
                
                if int(row[0])== 0:

                    coordinates = multilaterate(known_points, distancesList)

                    x_data.append(coordinates[0])
                    y_data.append(coordinates[1])

                    # Update plot
                    line.set_xdata(x_data)
                    line.set_ydata(y_data)
                    print(rollingAv[0])
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
                    time.sleep(0.01)


        i = 0
        # for l in rx:
        #     dic[key][i] = int(np.mean(rx[i]))
        #     i+=1
