#ifndef __MAP_H__
#define __MAP_H__
#include <map>
#include <vector>
#include <memory>
#include "Area.hpp"
#include "Chunk.hpp"
#include "Matrix.hpp"
#include "Event.hpp"
#include "Encounters.hpp"
#include "NDS/System/Archive.hpp"
#include "NDS/System/Rom.hpp"
#include "NDS/Assets/NSBTX.hpp"

class MapManager {
    uint32_t mNameID { 1 };
    std::vector<Area> mAreas;
    uint32_t mActiveMatrix { 0 };
    std::vector<std::shared_ptr<Matrix>> mMatrices;
    std::vector<std::shared_ptr<MapChunkHeader>> mChunkHeaders;

    std::shared_ptr<Palkia::Nitro::Archive> mMapTexArchive        = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMatrixArchive        = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mAreaDataArchive      = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mBuildingArchive      = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mMapChunkArchive      = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mEventDataArchive     = nullptr;
    std::shared_ptr<Palkia::Nitro::Archive> mEncounterDataArchive = nullptr;

    std::vector<std::shared_ptr<Event>> mEvents;
public:
    Encounter mEncounters;
    std::vector<std::shared_ptr<Matrix>>& GetMatrices() { return mMatrices; }
    std::vector<std::shared_ptr<MapChunkHeader>>& GetChunkHeaders() { return mChunkHeaders; }
    
    void Init(Palkia::Nitro::Rom*,std::vector<std::string>); // load archives needed for map loading
    void Save(Palkia::Nitro::Rom*);
    
    void SaveMatrix();

    std::shared_ptr<Matrix> GetActiveMatrix() { if(mMatrices.size() > 0) { return mMatrices[mActiveMatrix]; } else { return nullptr; } };
    void SetActiveMatrix(uint32_t);
    void LoadZone(uint32_t); // needs to pass some
    void Draw(glm::mat4);

    MapManager(){}
    ~MapManager(){}
};

#endif