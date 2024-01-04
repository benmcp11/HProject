/* NVS Struct */
#ifndef NVS_DATA_H_
#define NVS_DATA_H_

#include <stdint.h>

typedef struct tx_info{
    uint8_t nodeID;
    int refFlag;
    int txFrequency;
    int8_t txPower;
    bool debug;
    bool packetFlag;


}TX_Info;


#endif
