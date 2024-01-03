import serial
import csv
import time

# Serial port settings (modify the port name as needed)
ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

# CSV file settings
expNo = 9
hdis = 100
rxHeight1 = 100
txHeight1 = 100
degTx = 0
degRx = 0
variation = "FullRecord"
orientation = 'flat_rightstart'
rotationPeriod = 10000 # m Seconds


csv_filename = f'Ex{expNo}Rev_hdis{hdis}_rxh{rxHeight1}_txh{txHeight1}_{orientation}_TT_variation{variation}.csv'
csv_header = ['NodeID', 'PacketNo',  'Iteration','RSSI', 'AbsTime', 'RecordedTime', 'Horizontal Distance (m)', 'Vertical Distance RX (m)', 'Vertical Distance TX (m)', 'Shift (m)', 'Tx angle (deg)', 'Rx Angle (deg)', 'Orientation']


# Create or open the CSV file for writing
with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)
    
    startTime = 0
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
                if i == 0:
                    startTime = int(absTime)

                degTx = (int(absTime)-startTime)*(rotationPeriod/(360*1000))

                if degTx >= 360:
                    
                    degTx -=360
                    startTime = int(absTime)
                # Write data to the CSV file
                csv_writer.writerow([node_id,packet_no, iteration, rssi, absTime, recordedTime, hdis,rxHeight1, txHeight1, degTx, degRx, orientation])

                # # Flush the CSV file to ensure data is written immediately
                csvfile.flush()

                print(f"Data received: NodeID={node_id}, AbsoluteTime={absTime}, RSSI={rssi}, PacketNo={packet_no}, Angle={degTx}")
                
                
            else:
                print("Invalid data format received:", uart_data)
            i+=1
    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()
