#include "rxDataStructs.h"

int freqDefaults[] = {865000000, 865300000, 865600000, 865900000, 866200000, 866500000, 866800000, 867100000, 867400000, 867700000, 868000000};

void setFreqDefaults(rxFrequencies *rxF) {
    int i;
    for (i = 0; i < MAX_N_FREQUENCIES; i++) {
        rxF->freqList[i] = freqDefaults[i];
    }
}


int checkFreqData(rxFrequencies *rxF) {
    /* Check each parameter
     * Returns a flag indicating if the data has been changed, in which case it must be rewritten over nvs 
     */

    int rewrite = 0;

    if (rxF->noFreqUsed > MAX_N_FREQUENCIES || rxF->noFreqUsed < 1) {
        rxF->noFreqUsed = 1;
        rewrite = 1;
    }
    
    int i;
    for (i = 0; i < MAX_N_FREQUENCIES; i++) {
        if (rxF->freqList[i] < FREQ_MIN || rxF->freqList[i] > FREQ_MAX) {
            rxF->freqList[i] = FREQ_DEFAULT;
            rewrite = 1;
        }
    }

    if (rxF->debug != 0 && rxF->debug != 1){
        rxF->debug = 1;
        rewrite = 1;
    }

    if(rxF->type !=0 && rxF->type!=1){
        rxF->type = 0;
        rewrite =1;
    }

    if(rxF->setup <0 || rxF->setup>5){
            rxF->setup = 0;
            rewrite =1;
    }

    if(rxF->distanceMetres <0 || rxF->setup>1100){
                rxF->distanceMetres = 1;
                rewrite =1;
    }

    if(rxF->vDistance <0 || rxF->vDistance>100){
        rxF->vDistance = 1;
        rewrite =1;
    }

    return rewrite;
}
