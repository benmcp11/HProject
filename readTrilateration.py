import serial
import csv
import time

ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

txX = 2
txY = 7
txZ = 0.75

csv_filename = f'ExperimentData16\Exp16_Trilateration_Tx(x{txX}m, y{txY}m, z{txZ}m,).csv'

csv_header = ['RxNID', 'TXNID', 'PacketNo', 'RSSI']

with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(csv_header)

    try:
        count = 0
        while True:
            # Read data from UART
            uart_data = ser.readline().decode().strip().strip('\0')

            print(uart_data)
            # Split the received data into fields
            split_data = uart_data.split(',')

            # Check if the received data has the correct format
            if len(split_data) == 7:
                rxNID = split_data[0]
                rxRSSI = split_data[6]

                for i in range(1,6):
                    txNID, rssi, pNo =  split_data[i].split('_')

                    # Write data to the CSV file
                    csv_writer.writerow([rxNID, txNID, pNo, rssi])

                    # # Flush the CSV file to ensure data is written immediately
                    csvfile.flush()

                if count>21:
                    break
                count+=1
            else:
                print("Invalid data format received:", uart_data)

    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()