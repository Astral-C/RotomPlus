#ifndef __MAP_CHUNK_H__
#define __MAP_CHUNK_H__
#include <glm/glm/glm.hpp>
#include <NDS/System/Archive.hpp>

namespace MapGraphicsHandler {
    void ClearModelCache();
}

struct MapChunkHeader {
    uint8_t mAreaID;
    uint8_t mMoveModelID;
    uint16_t mMatrixID;
    uint16_t mScriptID;
    uint16_t mSpScriptID;
    uint16_t mMsgID;
    uint16_t mBgmDayID;
    uint16_t mBgmNightID;
    uint16_t mEncDataID;
    uint16_t mEventDataID;
    uint8_t mPlaceNameID;
    uint8_t mTextBoxType;
    uint8_t mWeatherID;
    uint8_t mCameraID;
    uint8_t mMapType;
    uint8_t mBattleBgType;
    uint8_t mBicycleFlag;
    uint8_t mDashFlag;
    uint8_t mEscapeFlag;
    uint8_t mFlyFlag;
    uint8_t mKantoFlag;
    void Read(bStream::CStream&, uint32_t);
};

struct Building {
    uint32_t mModelID;
    float x, y, z;
    float rx, ry, rz;
    float l, w, h;
    uint32_t unk1, unk2;
    uint32_t mPickID { 0 };
};

class MapChunk {
    uint16_t mID;
    std::array<std::pair<uint8_t, uint8_t>, 1024> mMovementPermissions {};
    std::vector<Building> mBuildings {};
    std::vector<uint8_t> mModelData {};
    std::vector<uint8_t> mBDHCData {};
public:

    std::vector<Building> GetBuildings() { return mBuildings; }

    std::array<std::pair<uint8_t, uint8_t>, 1024>& GetMovementPermissions() { return mMovementPermissions; }

    Building* Select(uint32_t id);

    void LoadGraphics(std::shared_ptr<Palkia::Nitro::File> mapTex, std::shared_ptr<Palkia::Nitro::Archive> buildModels);

    void RemoveBuilding(uint32_t i) {
        mBuildings.erase(mBuildings.begin() + (std::size_t)i);
    }

    Building* AddBuilding(Building b) {
        mBuildings.push_back(b);
        return &mBuildings.back();
    }

    void ImportChunkNSBMD(std::string path);

    void Draw(uint8_t cx, uint8_t cy, uint8_t cz, glm::mat4 v);
    void Save(std::shared_ptr<Palkia::Nitro::Archive> archive);

    MapChunk(uint16_t, bStream::CStream&);
    ~MapChunk();
};

#endif