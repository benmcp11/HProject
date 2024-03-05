from re import L
import serial
import csv
import time

ser = serial.Serial('COM4', baudrate=115200)  # Replace '/dev/tty.usbserial' with your UART port and configure other settings as needed

txX = 2.29
txY = round(0.58-0.63,1)
txZ = 0.8

room = 'BathM'
csv_filename = f'ExperimentData18\Exp18_Multilat_Tx(x{txX}m, y{txY}m, z{txZ}m,)_{room}.csv'

csv_header = ['RxNID', 'TXNID', 'PacketNo', 'TxRSSI', 'RSSI0','RSSI1','RSSI2','RSSI3']

rxRssiDic = {}
noRx = 4
for i in range(noRx):
    rxRssiDic[i] = []
    for j in range(noRx):
        rxRssiDic[i].append(j)

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
            if len(split_data) >= 7:
                rxNID = int(split_data[0])
                rxRSSI = split_data[6]
                if rxNID != 0:
                    rxRssiDic[0][rxNID] = rxRSSI

                    rxRssiDic[rxNID] = split_data[7].split('_')
            

                for i in range(1,6):
                    txNID, rssi, pNo =  split_data[i].split('_')

                    # Write data to the CSV file

                    output = [rxNID, txNID, pNo, rssi]
                    output.append(rxRssiDic[rxNID])
                    # print(output)
                    csv_writer.writerow(output)

                    # # Flush the CSV file to ensure data is written immediately
                    csvfile.flush()

                if count>20:
                    break
                count+=1
            else:
                print("Invalid data format received:", uart_data)

    except KeyboardInterrupt:
        print("Data collection stopped.")

    finally:
        # Close the serial port and CSV file
        ser.close()