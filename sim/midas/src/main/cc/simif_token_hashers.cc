#include "simif_token_hashers.h"

#include <iostream>

simif_token_hashers::simif_token_hashers() {
    std::cout << "in simif_token_hashers()\n";
    TOKENHASHMASTER_0_substruct_create;
    trigger0 = TOKENHASHMASTER_0_substruct->triggerDelay0_TokenHashMaster;
    trigger1 = TOKENHASHMASTER_0_substruct->triggerDelay1_TokenHashMaster;
    period0 = TOKENHASHMASTER_0_substruct->triggerPeriod0_TokenHashMaster;
    period1 = TOKENHASHMASTER_0_substruct->triggerPeriod1_TokenHashMaster;
    free(TOKENHASHMASTER_0_substruct);

    
    
    info();
}


void simif_token_hashers::info() {
    std::cout << "trigger0 " << trigger0 << "\n";
    std::cout << "trigger1 " << trigger1 << "\n";
    std::cout << "period0 " << period0 << "\n";
    std::cout << "period1 " << period1 << "\n";
}
