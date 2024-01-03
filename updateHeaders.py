import os
import pandas as pd

# Define the folder containing the CSV files
folder_path = "C:\\Users\\mcpar\\Documents\\HProject\\\ExperimentData1\\"

# Define the old and new column headers
old_headers = ['NodeID', 'PacketNo', 'Iteration', 'RSSI', 'AbsTime', 'RecordedTime', 'Horizontal Distance (m)', 'Vertical Distance (m)']
new_headers = ['NodeID', 'PacketNo', 'Iteration', 'RSSI', 'AbsTime', 'RecordedTime', 'Horizontal Distance (m)', 'Vertical Distance RX (m)', 'Vertical Distance TX (m)']

# Iterate through CSV files in the folder
for filename in os.listdir(folder_path):
    if filename.endswith('.csv'):
        file_path = os.path.join(folder_path, filename)

        # Read the CSV file into a DataFrame
        df = pd.read_csv(file_path)

        # Check if the old headers are present in the DataFrame
        if all(header in df.columns for header in old_headers):
            # Rename the columns with new headers
            df.rename(columns=dict(zip(old_headers, new_headers)), inplace=True)

            # Save the updated DataFrame back to the CSV file
            df.to_csv(file_path, index=False)

            print(f'Updated headers in {filename}')

print('Header replacement completed for all CSV files.')
