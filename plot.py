import csv
import os
import matplotlib.pyplot as plt
import numpy as np

# Dictionary to store averaged data
averaged_data = {}

# Function to calculate the average RSSI for a list of values
def calculate_average_rssi(rssi_values):
    if not rssi_values:
        return 0
    return sum(rssi_values) / len(rssi_values)

# Folder path containing the CSV files
folder_path = "C:\\Users\\mcpar\\Documents\\HProject\\\ExperimentData3and4\\"

# Get a list of all .csv files in the folder
csv_files = [f for f in os.listdir(folder_path) if f.endswith('.csv')]

# Loop through the CSV files
for csv_file in csv_files:
    file_path = os.path.join(folder_path, csv_file)
    with open(file_path, newline='') as csvfile:
        reader = csv.reader(csvfile)
        next(reader, None)
        for row in reader:
            if len(row) == 9:
                node_id, packet_no, iteration, rssi, absTime, recordedTime, horizontal_dist, vtx_tx, vtx_rx = map(float, row)
            elif len(row)==8:
                packet_no, iteration, rssi, absTime, recordedTime, horizontal_dist, vtx_tx, vtx_rx = map(float, row)

            vertical_diff = round(abs(vtx_tx - vtx_rx),3)


            # Create a unique key for the dictionary
            
            key = (horizontal_dist, vertical_diff)

            if key not in averaged_data:
                averaged_data[key] = []

            averaged_data[key].append(rssi)

# Calculate the average RSSI for each key and store in the final dictionary
averaged_data_result = {}
for key, rssi_values in averaged_data.items():
    avg_rssi = calculate_average_rssi(rssi_values)
    averaged_data_result[key] = avg_rssi

# Print the resulting dictionary
# for key, avg_rssi in averaged_data_result.items():
#     print(f"Horizontal Distance: {key[0]}, Vertical Differential: {key[1]}, Avg RSSI: {avg_rssi}")


# Calculate the average RSSI for each key and store in the final dictionary

def plot2d(averaged_data):

    averaged_data_result = {}
    for key, rssi_values in averaged_data.items():
        avg_rssi = calculate_average_rssi(rssi_values)
        averaged_data_result[key[0]] = (avg_rssi)

    # Extract the horizontal distances and average RSSI values
    horizontal_distances = list(averaged_data_result.keys())
    average_rssi_values = list(averaged_data_result.values())


    # Plot the data
    plt.figure(figsize=(10, 6))
    
    x, y = zip(*sorted(zip(horizontal_distances, average_rssi_values)))
    plt.plot(x, y,'-o')
    plt.title('Average RSSI vs. Horizontal Distance')
    plt.xlabel('Horizontal Distance (m)')
    plt.ylabel('Average RSSI')
    plt.grid(True)
    plt.show()

def plot3d(averaged_data):
        
    # Create lists to store the data for 3D plotting
    horizontal_distances = []
    vertical_differentials = []
    average_rssis = []

    # Calculate the average RSSI for each key and store in the final dictionary
    for key, rssi_values in averaged_data.items():
        avg_rssi = calculate_average_rssi(rssi_values)
        horizontal_distances.append(key[0])
        vertical_differentials.append(key[1])
        average_rssis.append(avg_rssi)

    # Create a 3D scatter plot
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    ax.scatter(horizontal_distances, vertical_differentials, average_rssis, c='r', marker='o')

    ax.set_xlabel('Horizontal Distance (m)')
    ax.set_ylabel('Vertical Differential')
    ax.set_zlabel('Average RSSI')

    plt.show()
def calcNVal(rssi, distance):
    n = -(rssi+20)/(np.log(distance)*10)
    print(n)
    return n

def calcDistance(rssi, n):
    distance = 10 ** ((-20 - rssi) / (10 * n))
    return distance

def calcAverageN(nList):
    return sum(nList)/len(nList)

def plotVdiffs(averaged_data):
    vdis = {}
    nVals = {}

    for key, rssi_values in averaged_data.items():
        avg_rssi = calculate_average_rssi(rssi_values)
        if key[1] not in vdis.keys():
            vdis[key[1]] = [[],[]]

        vdis[key[1]][0].append(key[0])
        vdis[key[1]][1].append(avg_rssi)
        
        if( key[0]!=1):
            nVal = calcNVal(avg_rssi, key[0])
            if key[1] not in nVals.keys():
                nVals[key[1]] = []


            nVals[key[1]].append(nVal)

        # print(f"Distance: {key[0]} Vdiff: {key[1]} n:{nVal}")

    averageN = {}
    newDistances = {}
    for key in nVals.keys():

        averageN[key] = calcAverageN(nVals[key])
        print(averageN[key])
        rssis = np.array(vdis[key][1])
        newDistances[key] = []
        for rssi in rssis:
            # newDistances[key].append(calcDistance(rssi, averageN[key]))
            newDistances[key].append(calcDistance(rssi, 4))

    # plt.figure(figsize=(10, 6))
    # plt.title('Average RSSI vs. Horizontal Distance')
    # plt.xlabel('Horizontal Distance (m)')
    # plt.ylabel('Average RSSI')
    # plt.grid(True)

    rssis = np.array(vdis[key][1])

    errors = {}
    for key in vdis:
        errors[key] = []
        for i in range(len(vdis[key][0])):
            errors[key].append(abs(vdis[key][0][i]-newDistances[key][i]))
    for key in vdis:
        plt.figure()
        plt.grid()
        x, y = zip(*sorted(zip(newDistances[key], vdis[key][1])))
        plt.plot(x, y,'-o',label = f'{key} metres:- error')

        x, y = zip(*sorted(zip(vdis[key][0], vdis[key][1])))
        plt.plot(x, y,'-x',label = f'{key} metres')
        # plt.plot(vdis[key][0], vdis[key][1],'-x', label = f'{key} metres') 
        # plt.plot(newDistances[key], vdis[key][1],'-o', label = f'{key} metres:- error')     

        plt.legend()
        plt.show()

        plt.figure()
        plt.grid()
        plt.title("Errors")
        plt.plot(vdis[key][0],errors[key],'o', label = f'{key} error') 
    plt.show()

plotVdiffs(averaged_data)

plot2d(averaged_data)
