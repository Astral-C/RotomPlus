#ifndef __ENCOUNTER_H__
#define __ENCOUNTER_H__
#include <NDS/System/Archive.hpp>
#include <cstdint>

struct Encounter {
    uint32_t mWalkingEncounterRate { 0 };
    uint32_t mSurfEncounterRate { 0 };
    uint32_t mOldRodRate { 0 };
    uint32_t mGoodRodRate { 0 };
    uint32_t mSuperRodRate { 0 };
    uint32_t mRockSmashRate { 0 };

    uint32_t mWalkingLevel[12] { 0 };
    uint32_t mWalking[12] { 0 };
    uint32_t mSwarmPokemon[4] { 0 };
    uint32_t mMorningPokemon[12] { 0 };
    uint32_t mDayPokemon[12] { 0 };
    uint32_t mNightPokemon[12] { 0 };
    uint32_t mRadarPokemon[4] { 0 };
    //0xa4 apparently
    uint32_t mRuby[2] { 0 };
    uint32_t mSapphire[2] { 0 };
    uint32_t mEmerald[2] { 0 };
    uint32_t mFireRed[2] { 0 };
    uint32_t mLeafGreen[2] { 0 };

    uint32_t mSurfMaxLevels[5] { 0 };
    uint32_t mSurfMinLevels[5] { 0 };
    uint32_t mSurf[5] { 0 };

    //0x124?

    uint32_t mOldMaxLevels[5] { 0 };
    uint32_t mOldMinLevels[5] { 0 };
    uint32_t mOldRod[5] { 0 };

    uint32_t mRockSmashMaxLevels[5] { 0 };
    uint32_t mRockSmashMinLevels[5] { 0 };
    uint32_t mRockSmashPokemon[5] { 0 };

    uint32_t mGoodRodMaxLevels[5] { 0 };
    uint32_t mGoodRodMinLevels[5] { 0 };
    uint32_t mGoodRod[5] { 0 };

    uint32_t mSuperRodMaxLevels[5] { 0 };
    uint32_t mSuperRodMinLevels[5] { 0 };
    uint32_t mSuperRod[5] { 0 };

    uint16_t mHoennGearMusic[2] { 0 };
    uint16_t mSinnohGearMusic[2] { 0 };

};

Encounter LoadEncounterFile(std::shared_ptr<Palkia::Nitro::File>, uint32_t);
void SaveEncounterFile(std::shared_ptr<Palkia::Nitro::File>, Encounter);

#endif