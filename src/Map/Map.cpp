#include <format>
#include "Map/Map.hpp"

std::vector<std::string> locationNames;

void MapManager::Init(Palkia::Nitro::Rom* rom, std::vector<std::string> locations){
    locationNames = locations;
    auto arm9 = rom->GetFile("@arm9.bin");
    bStream::CMemoryStream armStream(arm9->GetData(), arm9->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    std::cout << "ARM9 Binary Size is 0x" << std::hex << arm9->GetSize() << std::dec << std::endl;
    armStream.seek(0xE601C);

    uint8_t areaCount = 0;
    for(int i = 0; i < 570; i++){
        std::shared_ptr<MapChunkHeader> header = std::make_shared<MapChunkHeader>();
        header->Read(armStream);
        //std::cout << "Loading header with map name " << locations[header->mPlaceNameID] << " " << (uint)header->mPlaceNameID << " Area " << std::dec << (uint)header->mAreaID << std::endl;
        areaCount = std::max(header->mAreaID, areaCount);
        mChunkHeaders.push_back(header);
    }

    // Load area data
    auto areaDataFile = rom->GetFile("fielddata/areadata/area_data.narc");
    auto areaDataStream = bStream::CMemoryStream(areaDataFile->GetData(), areaDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mAreaDataArchive = std::make_shared<Palkia::Nitro::Archive>(areaDataStream);
    for(int i = 0; i  < areaCount; i++){
        auto areaStream = bStream::CMemoryStream(mAreaDataArchive->GetFileByIndex(i)->GetData(), mAreaDataArchive->GetFileByIndex(i)->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
        mAreas.push_back({.mBuildingSet = areaStream.readUInt16(), .mMapTileset = areaStream.readUInt16(), .mUnknown = areaStream.readUInt16(), .mLightType = areaStream.readUInt16()});
    }

    auto mapChunkFile = rom->GetFile("fielddata/land_data/land_data.narc");
    auto mapTexFile = rom->GetFile("fielddata/areadata/area_map_tex/map_tex_set.narc");
    auto buildModelFile = rom->GetFile("fielddata/build_model/build_model.narc");
    auto mapMatrixFile = rom->GetFile("fielddata/mapmatrix/map_matrix.narc");
    auto eventDataFile = rom->GetFile("fielddata/eventdata/zone_event.narc");
    auto encounterDataFile = rom->GetFile("fielddata/encountdata/pl_enc_data.narc");

    auto stream = bStream::CMemoryStream(mapChunkFile->GetData(), mapChunkFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mMapChunkArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

    stream = bStream::CMemoryStream(mapTexFile->GetData(), mapTexFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mMapTexArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

    stream = bStream::CMemoryStream(buildModelFile->GetData(), buildModelFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mBuildingArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

    stream = bStream::CMemoryStream(areaDataFile->GetData(), areaDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mAreaDataArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

    stream = bStream::CMemoryStream(mapMatrixFile->GetData(), mapMatrixFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mMatrixArchive = std::make_shared<Palkia::Nitro::Archive>(stream);
    
    stream = bStream::CMemoryStream(eventDataFile->GetData(), eventDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mEventDataArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

    stream = bStream::CMemoryStream(encounterDataFile->GetData(), encounterDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mEncounterDataArchive = std::make_shared<Palkia::Nitro::Archive>(stream);

}

void MapManager::SetActiveMatrix(uint32_t index){
    uint16_t encounterID = 0xFFFF;
    
    mActiveMatrix = index;
    MapGraphicsHandler::ClearModelCache();
    if(mMatrices.size() == 0) return;
    for(auto chunk : mMatrices[index]->GetEntries()){
        auto chunkLocked = chunk.mChunk.lock();
        auto chunkHeaderLocked = chunk.mChunkHeader.lock();
        if(chunk.mChunk.lock() && chunk.mChunkHeader.lock() && chunkHeaderLocked->mPlaceNameID == mNameID){
            //mAreas[chunkHeaderLocked->mAreaID] .mMapTileset)
            
            if(encounterID == 0xFFFF && chunkHeaderLocked->mEncDataID != 0xFFFF) encounterID = chunkHeaderLocked->mEncDataID;
            int texSet = mAreas[chunkHeaderLocked->mAreaID].mMapTileset;
            chunkLocked->LoadGraphics(mMapTexArchive->GetFileByIndex(texSet), mBuildingArchive);
        }
    }

    // load encounter data
    if(encounterID != 0xFFFF){
        auto encounterFile = mEncounterDataArchive->GetFileByIndex(encounterID);
        mEncounters = LoadEncounterFile(encounterFile);
    }
}

//Building* MapManager::Select(uint32_t id){
//    return mMatrices[mActiveMatrix]->Select(id);   
//}

void MapManager::LoadZone(uint32_t nameID){
    // check if a map is already loaded, then save + clear
    mMatrices.clear();
    mNameID = nameID;

    MapGraphicsHandler::ClearModelCache();

    std::vector<std::pair<uint32_t, std::shared_ptr<MapChunkHeader>>> matrixIndices = {};
    for(auto header : mChunkHeaders){
        std::cout << locationNames[header->mPlaceNameID] << " " << mNameID << std::endl;
        if(header->mPlaceNameID == mNameID){
            matrixIndices.push_back({header->mMatrixID, header});
        }
    }

    //if(matrixIndices.)

    for(auto [matrixIndex, header] : matrixIndices){
        mMatrices.push_back(std::make_shared<Matrix>());
        mMatrices.back()->Load(mMatrixArchive->GetFileByIndex(matrixIndex), mMapChunkArchive, mChunkHeaders, header);
    }

    SetActiveMatrix(0);

}

void MapManager::Draw(glm::mat4 v){
    if(mMatrices.size() > 0) mMatrices[mActiveMatrix]->Draw(v, mNameID);
}