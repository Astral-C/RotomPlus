#ifndef __ENCOUNTER_H__
#define __ENCOUNTER_H__
#include <cstdint>

struct Encounter {
    uint32_t mWalkingEncounterRate;
    uint32_t mWalkingLevel[12];
    uint32_t mWalkingEncounters[12];
};


#endif