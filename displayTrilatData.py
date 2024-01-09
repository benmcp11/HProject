import csv

def readData(file_name):

    # Create an empty list to store the data
    data = []


    # Open the CSV file for reading
    with open(file_name, 'r') as csvfile:
        # Create a CSV reader object
        csvreader = csv.DictReader(csvfile)
        
        # Iterate over each row in the CSV file
        for row in csvreader:
            # Append the row to the data list
            data.append(row)

    # # Print the data
    # for row in data:
    #     print(row)


file_name = 'TestReadTrilateration.csv'

data = readData(file_name)

rxPositions = [[0,0], [0,10], ]


import numpy as np
from scipy.optimize import minimize

def distance(p1, p2):
    """Calculate the Euclidean distance between two 3D points."""
    return np.linalg.norm(np.array(p1) - np.array(p2))

def objective_function(x, known_points, distances):
    """Objective function for optimization."""
    return sum([(distance(x, known_points[i]) - distances[i])**2 for i in range(len(known_points))])

def trilaterate(known_points, distances):
    """Perform 3D trilateration."""
    initial_guess = np.mean(known_points, axis=0)  # Initial guess as the centroid of known points
    result = minimize(objective_function, initial_guess, args=(known_points, distances), method='Nelder-Mead')
    
    return result.x

# Example usage:
    # Known points in 3D space (x, y, z)
p1 = [1, 1, 1]
p2 = [5, 1, 1]
p3 = [3, 4, 1]

# Distances from the unknown point to the known points
d1 = 3.74
d2 = 4.98
d3 = 4.98

known_points = np.array([p1, p2, p3])
distances = [d1, d2, d3]

result = trilaterate(known_points, distances)

print(f"The coordinates of the unknown point are: {result}")



