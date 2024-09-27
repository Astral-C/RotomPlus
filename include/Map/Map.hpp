#ifndef __MAP_H__
#define __MAP_H__
#include <vector>
#include <memory>
#include "Chunk.hpp"
#include "Matrix.hpp"
#include "NDS/System/Archive.hpp"
#include "NDS/System/Rom.hpp"

class MapManager {
    std::vector<std::unique_ptr<Matrix>> mMatrices;
    std::vector<std::shared_ptr<MapChunkHeader>> mChunkHeaders;

    std::shared_ptr<Palkia::Nitro::Archive> mMapTexArchive    = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMatrixArchive    = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mAreaDataArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mBuildingArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMapChunkArchive  = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mEventDataArchive = nullptr;

public:
    std::vector<std::shared_ptr<MapChunkHeader>>& GetChunkHeaders() { return mChunkHeaders; }
    
    void Init(Palkia::Nitro::Rom*); // load archives needed for map loading
    void LoadZone(uint32_t); // needs to pass some

    MapManager(){}
    ~MapManager(){}
};

#endif