import serial
import csv
import time

# Serial port settings (modify the port name as needed)
ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

# CSV file settings
expNo = 13
hdis = 7.5
rxHeight1 = 2.5
txHeight1 = 0.9
shift1 = 0
degTx = 0
degRx = 0
orientation = 'vertical'

csv_filename = f'Ex{expNo}_hdis{hdis}_rxh{rxHeight1}_txh{txHeight1}_shift{shift1}_txDeg{degTx}_rxDeg{degRx}_vertical_vertical_wall.csv'
csv_header = ['NodeID', 'PacketNo',  'Iteration','RSSI', 'AbsTime', 'RecordedTime', 'Horizontal Distance (m)', 'Vertical Distance RX (m)', 'Vertical Distance TX (m)', 'Shift (m)', 'Tx angle (deg)', 'Rx Angle (deg)', 'Orientation']


# Create or open the CSV file for writing
with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)

    try:
        i =0
        while True:
            # Read data from UART
            uart_data = ser.readline().decode().strip().strip('\0')

            # Split the received data into fields
            data_fields = uart_data.split('_')

            # Check if the received data has the correct format
            if len(data_fields) == 9:
                node_id,packet_no, iteration, rssi, absTime, recordedTime, hdistance, rxHeight, txHeight = data_fields

                # Write data to the CSV file
                csv_writer.writerow([node_id,packet_no, iteration, rssi, absTime, recordedTime, hdis,rxHeight1, txHeight1, shift1, degTx, degRx, orientation])

                # # Flush the CSV file to ensure data is written immediately
                csvfile.flush()

                print(f"Data received: NodeID={node_id}, AbsoluteTime={absTime}, RSSI={rssi}, PacketNo={packet_no}, HDistance={hdistance}")
                if i > 100:
                    break
                i+=1
            else:
                print("Invalid data format received:", uart_data)

    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()
