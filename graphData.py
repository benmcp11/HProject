import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
file_path = 'uart_data2.csv'
df = pd.read_csv(file_path)

# Extract data from the DataFrame
abs_time = df['AbsTime']
rssi = df['RSSI']

# Create the graph
plt.figure(figsize=(10, 6))
plt.plot(abs_time, rssi,'-')
plt.title('RSSI vs Absolute Time')
plt.xlabel('Absolute Time')
plt.ylabel('RSSI')
plt.grid(True)
# plt.yscale('log')  # Change the y-axis to a logarithmic scale
# plt.xscale('log')
# Show the graph
plt.show()
