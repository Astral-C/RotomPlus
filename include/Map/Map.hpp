#ifndef __MAP_H__
#define __MAP_H__
#include <map>
#include <vector>
#include <memory>
#include "Area.hpp"
#include "Chunk.hpp"
#include "Matrix.hpp"
#include "NDS/System/Archive.hpp"
#include "NDS/System/Rom.hpp"
#include "NDS/Assets/NSBTX.hpp"

class MapManager {
    uint32_t mNameID { 1 };
    std::vector<Area> mAreas;
    uint32_t mActiveMatrix { 0 };
    std::vector<std::shared_ptr<Matrix>> mMatrices;
    std::vector<std::shared_ptr<MapChunkHeader>> mChunkHeaders;

    std::shared_ptr<Palkia::Nitro::Archive> mMapTexArchive    = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMatrixArchive    = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mAreaDataArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mBuildingArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMapChunkArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mEventDataArchive = nullptr;

public:
    std::vector<std::shared_ptr<Matrix>>& GetMatrices() { return mMatrices; }
    std::vector<std::shared_ptr<MapChunkHeader>>& GetChunkHeaders() { return mChunkHeaders; }
    
    void Init(Palkia::Nitro::Rom*,std::vector<std::string>); // load archives needed for map loading
    void LoadZone(uint32_t); // needs to pass some
    void SetActiveMatrix(uint32_t);
    void Draw(glm::mat4);

    MapManager(){}
    ~MapManager(){}
};

#endif