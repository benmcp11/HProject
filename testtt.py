import math

def estimate_distance(RSSI, A, n):
    # Calculate distance based on RSSI, A, and n
    distance = 10 ** ((A - RSSI) / (10 * n))
    return distance

def estimate_path_loss_exponent(RSSI, A, distance):
    # Calculate path loss exponent (n) based on RSSI, A, and distance
    n = -(RSSI - A) / (10 * math.log10(distance))
    return n

# Example usage:
RSSI = -60 # Received Signal Strength Indicator in dBm
A = -50    # Reference RSSI at a known distance in dBm
distance = 10  # Estimated distance in meters
n = 2  # Path loss exponent

estimated_distance = estimate_distance(RSSI, -20, n)
estimated_n = estimate_path_loss_exponent(RSSI, -20, distance)

print(f"Estimated Distance: {estimated_distance} meters")
print(f"Estimated Path Loss Exponent (n): {estimated_n}")