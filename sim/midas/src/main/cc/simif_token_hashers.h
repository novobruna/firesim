// See LICENSE for license details.

#ifndef __SIMIF_TOKEN_HASHERS_H
#define __SIMIF_TOKEN_HASHERS_H

#include <vector>


class simif_token_hashers {
public:
    simif_token_hashers();
    void info();

    uint32_t trigger0;
    uint32_t trigger1;
    uint32_t period0;
    uint32_t period1;

    std::vector<std::string> names;
    std::vector<bool> outputs;
    std::vector<uint32_t> queue_heads;
    std::vector<uint32_t> queue_occupancies;
    std::vector<uint32_t> tokencounts0;
    std::vector<uint32_t> tokencounts1;
};

#endif
