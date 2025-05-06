#include <format>
#include "Map/Map.hpp"
#include "GameConfig.hpp"
#include "NDS/System/Compression.hpp"
#include <set>

std::vector<std::string> locationNames;
std::vector<std::string> chunkHeaderNames;

std::string MapManager::GetChunkName(uint32_t idx){
    return chunkHeaderNames[idx];
}

void MapManager::Init(Palkia::Nitro::Rom* rom, std::vector<std::string>& locations){
    mActiveMatrix = 0;
    mMatrices.clear();
    mChunkHeaders.clear();
    auto arm9 = rom->GetFile("@arm9.bin");

    if(Configs[rom->GetHeader().gameCode].compressedArm9){
        Palkia::Nitro::Compression::BLZDecompress(arm9);
    }

    mGameCode = rom->GetHeader().gameCode;

    bStream::CMemoryStream armStream(arm9->GetData(), arm9->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    std::cout << "ARM9 Binary Size is 0x" << std::hex << arm9->GetSize() << std::dec << std::endl;
    armStream.seek(Configs[rom->GetHeader().gameCode].mChunkHeaderPtr);

    auto mapTable = rom->GetFile(Configs[rom->GetHeader().gameCode].mMapTablePath);
    auto stream = bStream::CMemoryStream(mapTable->GetData(), mapTable->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    for(int x = 0; x < mapTable->GetSize() / 16; x++){
        chunkHeaderNames.push_back(stream.readString(16));
    }

    std::set<uint8_t> uniqueNames;

    uint8_t areaCount = 0;
    for(int i = 0; i < chunkHeaderNames.size(); i++){
        std::shared_ptr<MapChunkHeader> header = std::make_shared<MapChunkHeader>();
        header->Read(armStream, rom->GetHeader().gameCode);
        std::cout << "Loading header with map name " << locations[header->mPlaceNameID] << " " << (uint)header->mPlaceNameID << " Area " << std::dec << (uint)header->mAreaID << std::endl;
        uniqueNames.insert(header->mPlaceNameID);
        areaCount = std::max(header->mAreaID, areaCount);
        mChunkHeaders.push_back(header);
    }
    mChunkHeaders.shrink_to_fit();

    locationNames = locations;
    // Load area data
    auto areaDataFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mAreaDataPath);
    auto areaDataStream = bStream::CMemoryStream(areaDataFile->GetData(), areaDataFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    mAreaDataArchive = std::make_shared<Palkia::Nitro::Archive>(areaDataStream);
    for(int i = 0; i  < mAreaDataArchive->GetFileCount(); i++){
        auto areaStream = bStream::CMemoryStream(mAreaDataArchive->GetFileByIndex(i)->GetData(), mAreaDataArchive->GetFileByIndex(i)->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
        if(mGameCode == (uint32_t)'EGPI'){
            mAreas.push_back({ .mBuildingSet = areaStream.readUInt16(), .mMapTileset = areaStream.readUInt16(), .mDynamicTextureType = areaStream.readUInt16(), .mAreaType = areaStream.readUInt8(), .mLightType = areaStream.readUInt16() });
        } else {
            mAreas.push_back({ .mBuildingSet = areaStream.readUInt16(), .mMapTileset = areaStream.readUInt16(), .mUnknown = areaStream.readUInt16(), .mLightType = areaStream.readUInt16() });
        }
    }
    mAreas.shrink_to_fit();

    auto mapChunkFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mLandDataPath);
    auto mapTexFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mMapTexSetPath);
    auto buildModelFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mBuildModelPath);
    auto mapMatrixFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mMapMatrixPath);
    auto eventDataFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mZoneEventPath);
    auto encounterDataFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mEncounterDataPath);
    auto mmodelListFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mMoveModelList);//Configs[rom->GetHeader().gameCode].mEncounterDataPath);

    stream = bStream::CMemoryStream(mapChunkFile->GetData(), mapChunkFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
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

    if(mmodelListFile != nullptr){
        stream = bStream::CMemoryStream(mmodelListFile->GetData(), mmodelListFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
        mMoveModelList = std::make_shared<Palkia::Nitro::Archive>(stream);
    }

    if(mGameCode == (uint32_t)'EGPI'){
        auto indoorBuildingFile = rom->GetFile(Configs[rom->GetHeader().gameCode].mIndoorBuildingPath);

        stream = bStream::CMemoryStream(indoorBuildingFile->GetData(), indoorBuildingFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
        mIndoorBuildingArchive = std::make_shared<Palkia::Nitro::Archive>(stream);
    }

}

void MapManager::Save(Palkia::Nitro::Rom* rom){
    bStream::CMemoryStream mapChunkStream(1, bStream::Endianess::Little, bStream::OpenMode::Out);
    mMapChunkArchive->SaveArchive(mapChunkStream);

    bStream::CMemoryStream encounterStream(1, bStream::Endianess::Little, bStream::OpenMode::Out);
    mEncounterDataArchive->SaveArchive(encounterStream);

    bStream::CMemoryStream eventStream(1, bStream::Endianess::Little, bStream::OpenMode::Out);
    mEventDataArchive->SaveArchive(eventStream);

    rom->GetFile(Configs[rom->GetHeader().gameCode].mLandDataPath)->SetData(mapChunkStream.getBuffer(), mapChunkStream.getSize());
    rom->GetFile(Configs[rom->GetHeader().gameCode].mEncounterDataPath)->SetData(encounterStream.getBuffer(), encounterStream.getSize());
    rom->GetFile(Configs[rom->GetHeader().gameCode].mZoneEventPath)->SetData(eventStream.getBuffer(), eventStream.getSize());
}

void MapManager::SaveMatrix(){
    uint16_t encounterID = 0xFFFF;
    uint16_t eventDataID = 0xFFFF;

    if(mMatrices.size() == 0) return;
    int x = 0, y = 0;
    for(auto chunk : mMatrices[mActiveMatrix]->GetEntries()){
        if(x + 1 == mMatrices[mActiveMatrix]->GetWidth()) y++;
        x = (x + 1) % mMatrices[mActiveMatrix]->GetWidth();

        auto chunkHeaderLocked = chunk.mChunkHeader.lock();
        if(chunk.mChunk != nullptr && chunk.mChunkHeader.lock() && chunkHeaderLocked->mPlaceNameID == mNameID){
            for(int i = 0; i < chunk.mChunk->GetBuildings().size(); i++){
                auto building = chunk.mChunk->GetBuildings()[i];
                if(building.x > 256 || building.x < -256 || building.z > 256 || building.z < -256){
                    int moveChunkX = floor(abs(building.x) / 256);
                    int moveChunkY = floor(abs(building.z) / 256);
                    std::cout << "Move Amount " << moveChunkX << " " << moveChunkY << std::endl;

                    if(building.x < 0) moveChunkX = -moveChunkX;
                    if(building.z < 0) moveChunkY = -moveChunkY;

                    std::cout << "Old building pos is " << building.x << "  " << building.z << std::endl;

                    building.x = fmod(building.x, 256) - building.x / 256;
                    building.z = fmod(building.z, 256) - building.z / 256;

                    std::cout << "New building pos is " << building.x << "  " << building.z << std::endl;

                    std::cout << "Adding building to to Chunk " << (x + moveChunkX) << " " << (y + moveChunkY) << " from " << x << " " << y << std::endl;

                    mMatrices[mActiveMatrix]->GetEntries()[(y + moveChunkY) * mMatrices[mActiveMatrix]->GetWidth() + (x + moveChunkX)].mChunk->AddBuilding(building);
                    mMatrices[mActiveMatrix]->GetEntries()[(y + moveChunkY) * mMatrices[mActiveMatrix]->GetWidth() + (x + moveChunkX)].mChunk->Save(mMapChunkArchive);
                    chunk.mChunk->RemoveBuilding(i);
                }
            }

            // save chunk
            chunk.mChunk->Save(mMapChunkArchive);

            if(encounterID == 0xFFFF && chunkHeaderLocked->mEncDataID != 0xFFFF) encounterID = chunkHeaderLocked->mEncDataID;
            if(eventDataID == 0xFFFF && chunkHeaderLocked->mEventDataID != 0xFFFF) eventDataID = chunkHeaderLocked->mEventDataID;
        }
    }

    // Save encounter data
    if(encounterID != 0xFFFF){
        SaveEncounterFile(mEncounterDataArchive->GetFileByIndex(encounterID), mEncounters);
    }

    // Save event data
    if(eventDataID != 0xFFFF){
        SaveEvents(mEventDataArchive->GetFileByIndex(eventDataID), mEvents);
    }

    //SetActiveMatrix(mActiveMatrix); // reload

}

void MapManager::SetActiveMatrix(uint32_t index){
    uint16_t encounterID = 0xFFFF;
    uint16_t eventDataID = 0xFFFF;
    uint8_t moveModelID = 0xFF;

    mActiveMatrix = index;
    MapGraphicsHandler::ClearModelCache();

    bool exterior = true;

    if(mMatrices.size() == 0) return;
    for(auto chunk : mMatrices[index]->GetEntries()){
        auto chunkHeaderLocked = chunk.mChunkHeader.lock();
        if(chunk.mChunk != nullptr && chunkHeaderLocked != nullptr && locationNames[chunkHeaderLocked->mPlaceNameID] == locationNames[mNameID]){
            std::cout << "Chunk Locked and Loading..." << std::endl;

            if(encounterID == 0xFFFF && chunkHeaderLocked->mEncDataID != 0xFFFF) encounterID = chunkHeaderLocked->mEncDataID;
            if(eventDataID == 0xFFFF && chunkHeaderLocked->mEventDataID != 0xFFFF) eventDataID = chunkHeaderLocked->mEventDataID;
            if(moveModelID == 0xFF && chunkHeaderLocked->mMoveModelID != 0xFF) moveModelID = chunkHeaderLocked->mMoveModelID;
            std::cout << std::dec << "Move Model List ID on chunk is  " << (int)chunkHeaderLocked->mMoveModelID << std::endl;
            if(mAreas[chunkHeaderLocked->mAreaID].mAreaType == 0) exterior = false;
            //int texSet = mAreas[chunkHeaderLocked->mAreaID].mMapTileset;
            //chunk.mChunk->LoadGraphics(mMapTexArchive->GetFileByIndex(texSet), mBuildingArchive);
        }
    }

    if(exterior){
        mMatrices[index]->LoadGraphics(mBuildingArchive, mMapTexArchive, mAreas, mNameID);
    } else {
        mMatrices[index]->LoadGraphics(mIndoorBuildingArchive, mMapTexArchive, mAreas, mNameID);
    }

    std::cout << std::dec << "Event data being loaded is :" << eventDataID << std::endl;
    if(eventDataID != 0xFFFF){
        // load events
        auto eventsFile = mEventDataArchive->GetFileByIndex(eventDataID);
        auto moveModelList = mMoveModelList->GetFileByIndex(moveModelID);
        std::cout << std::dec << "Move Model List ID is " << (int)moveModelID << std::endl;
        mEvents = LoadEvents(eventsFile, moveModelList);
    }

    // load encounter data
    std::cout << std::dec << "Encounter data being loaded is :" << encounterID << std::endl;
    if(encounterID != 0xFFFF && encounterID != 0xFF){
        auto encounterFile = mEncounterDataArchive->GetFileByIndex(encounterID);
        mEncounters = LoadEncounterFile(encounterFile, mGameCode);
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
        if(header->mPlaceNameID >= locationNames.size()) continue;
        std::cout << "checking chunk header " << std::dec << (uint32_t)header->mPlaceNameID << " - " << locationNames[header->mPlaceNameID] << " - " << mNameID << std::endl;
        if(header->mPlaceNameID == mNameID){
            matrixIndices.push_back({header->mMatrixID, header});
        }
    }
    matrixIndices.shrink_to_fit();

    //if(matrixIndices.)

    std::cout << "Chunks collected, loading matrices...." << std::endl;

    for(auto [matrixIndex, header] : matrixIndices){
        mMatrices.push_back(std::make_shared<Matrix>());
        if(matrixIndex < mMatrixArchive->GetFileCount()) mMatrices.back()->Load(mMatrixArchive->GetFileByIndex(matrixIndex), mMapChunkArchive, mChunkHeaders, header, mGameCode);
    }

    SetActiveMatrix(0);

}

void MapManager::Draw(glm::mat4 v){
    if(mMatrices.size() > 0) mMatrices[mActiveMatrix]->Draw(v, mNameID);
}
