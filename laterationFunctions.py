import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import minimize

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

def estimateN(distance, rssi):
    n = (-20 - rssi) / (10 *np.log10(distance))
    return n