#ifndef RX_DATA_STRUCTS_H_
#define RX_DATA_STRUCTS_H_

#include <stdint.h>
#include <stdbool.h>

#define FREQ_MAX            868000000
#define FREQ_MIN            865000000
#define FREQ_DEFAULT        868000000

#define MAX_N_FREQUENCIES   11

typedef struct rxPacket_info{
    uint8_t nodeID;
    int refFlag;
    int txFrequency;
    int packetNum;
    uint32_t abs_time;
    int numberOfPacketOverflows;
    uint8_t prevPacketNumber;
    int iteration;
    uint8_t timeTaken;
    float rcvDistance;
}rxPacketInfo;

typedef struct rxFrequencies{
    int freqList[MAX_N_FREQUENCIES];
    int noFreqUsed;
    bool debug;
    int type;
    int setup;
    float distanceMetres;
    float vDistance;
}rxFrequencies;

void setFreqDefaults(rxFrequencies *rxF);

#define DATA_REWRITE_FLAG   1

int checkFreqData(rxFrequencies *rxF);

#endif
