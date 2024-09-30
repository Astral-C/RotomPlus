#include "Map/Map.hpp"

void MapManager::Init(Palkia::Nitro::Rom* rom){
    auto arm9 = rom->GetFile("arm9.bin");
    bStream::CMemoryStream armStream(arm9->GetData(), arm9->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    std::cout << "ARM9 Binary Size is 0x" << std::hex << arm9->GetSize() << std::dec << std::endl;
    armStream.seek(0xE601C);

    uint8_t areaCount = 0;
    for(int i = 0; i < 500; i++){
        std::shared_ptr<MapChunkHeader> header = std::make_shared<MapChunkHeader>();
        header->Read(armStream);
        areaCount = std::max(header->mAreaID, areaCount);
        mChunkHeaders.push_back(header);
    }

    // Load area data
    auto areaDataFile = rom->GetFile("fielddata/areadata/area_data.narc");
    bStream::CMemoryStream areaStream(areaDataFile->GetData(), areaDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    for(int i = 0; i  < areaCount; i++){
        mAreas.push_back({.mBuildingSet = areaStream.readUInt16(), .mMapTileset = areaStream.readUInt16(), .mUnknown = areaStream.readUInt16(), .mLightType = areaStream.readUInt16()});
    }

    auto mapChunkFile = rom->GetFile("fielddata/land_data/land_data.narc");
    auto mapTexFile = rom->GetFile("fielddata/areadata/area_map_tex/map_tex_set.narc");
    auto buildModelFile = rom->GetFile("fielddata/build_model/build_model.narc");
    auto mapMatrixFile = rom->GetFile("fielddata/mapmatrix/map_matrix.narc");
    auto eventDataFile = rom->GetFile("fielddata/eventdata/zone_event.narc");

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

}

void MapManager::SetActiveMatrix(uint32_t index){
    mActiveMatrix = index;
    MapGraphicsHandler::ClearModelCache();

    for(auto chunk : mMatrices[index]->GetEntries()){
        auto chunkLocked = chunk.mChunk.lock();
        auto chunkHeaderLocked = chunk.mChunkHeader.lock();
        if(chunk.mChunk.lock() && chunk.mChunkHeader.lock() && chunkHeaderLocked->mPlaceNameID == mNameID){
            //mAreas[chunkHeaderLocked->mAreaID].mMapTileset)
            int texSet = mAreas[chunkHeaderLocked->mAreaID].mMapTileset;
            chunkLocked->LoadGraphics(mMapTexArchive->GetFileByIndex((texSet < mMapTexArchive->GetFileCount() ? texSet : 6)), mBuildingArchive);
        }
    }

}

void MapManager::LoadZone(uint32_t nameID){
    // check if a map is already loaded, then save + clear
    mMatrices.clear();
    mNameID = nameID;

    std::vector<uint32_t> matrixIndices = {};
    for(auto header : mChunkHeaders){
        if(header->mPlaceNameID == mNameID){
            matrixIndices.push_back(header->mMatrixID);
        }
    }

    //if(matrixIndices.)

    for(auto matrixIndex : matrixIndices){
        mMatrices.push_back(std::make_shared<Matrix>());
        mMatrices.back()->Load(mMatrixArchive->GetFileByIndex(matrixIndex), mMapChunkArchive, mChunkHeaders);
    }

}

void MapManager::Draw(glm::mat4 v){
    if(mMatrices.size() > 0) mMatrices[mActiveMatrix]->Draw(v);
}