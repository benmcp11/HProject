import csv
import os
from collections import defaultdict
import glob

# Function to calculate the vertical distance difference
def calculate_vertical_distance_diff(vtx, vrx):
    return (vtx - vrx)

# Function to average data by RSSI
def average_data(data):
    rssi_sum = 0
    vtx_diff_sum = 0
    count = 0

    for row in data:
        rssi, vtx, vrx = row[0], row[1], row[2]
        rssi_sum += rssi
        vtx_diff_sum += calculate_vertical_distance_diff(vtx, vrx)
        count += 1

    avg_rssi = rssi_sum / count
    avg_vtx_diff = vtx_diff_sum / count

    return (avg_rssi, calculate_vertical_distance_diff(vtx, vrx))

# Folder containing CSV files
folder_path = "C:\\Users\\mcpar\\Documents\\HProject\\\ExperimentData1\\"

# Create a dictionary to store the averaged data
averaged_data = defaultdict(list)

# Process all CSV files in the folder
for csv_filename in glob.glob(os.path.join(folder_path, "*.csv")):
    with open(csv_filename, 'r') as csvfile:
        csv_reader = csv.reader(csvfile)
        next(csv_reader)  # Skip the header row

        for row in csv_reader:
            if len(row) == 9:
                node_id, packet_no, iteration, rssi, absTime, recordedTime, horizontal_dist, vtx_tx, vtx_rx = map(float, row)
            elif len(row)==8:
                packet_no, iteration, rssi, absTime, recordedTime, horizontal_dist, vtx_tx, vtx_rx = map(float, row)

            key = horizontal_dist
            data = (rssi, vtx_tx, vtx_rx)
            
            averaged_data[key].append(data)

# Calculate and store the averaged data
averaged_results = {}
for horizontal_dist, data in averaged_data.items():
    avg_rssi, avg_vtx_diff = average_data(data)
    averaged_results[horizontal_dist] = [avg_rssi, avg_vtx_diff]

# Print the dictionary with averaged data
for key, value in averaged_results.items():
    print(f"Horizontal Distance: {key}, Avg RSSI: {value[0]}, Avg Vertical Distance Diff: {value[1]}")
