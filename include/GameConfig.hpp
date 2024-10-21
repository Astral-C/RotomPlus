#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__
#include <cstdint>
#include <string>
#include <map>

struct GameConfig {
    uint32_t mChunkHeaderPtr;
    std::string mAreaDataPath;
    std::string mMapTablePath;
    std::string mLandDataPath;
    std::string mMapTexSetPath;
    std::string mBuildModelPath;
    std::string mMapMatrixPath;
    std::string mZoneEventPath;
    std::string mEncounterDataPath;
    std::string mMsgPath;
    std::string mMoveModel;
    std::string mIndoorBuildingPath;
    uint32_t mLocationNamesFileID;
    uint32_t mPokeNamesFileID;
    bool compressedArm9;
};

extern const GameConfig Platinum;
extern const GameConfig SoulSilver;

extern std::map<uint32_t, const GameConfig> Configs;


#endif