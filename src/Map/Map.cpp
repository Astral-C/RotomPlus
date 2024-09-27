#include "Map/Map.hpp"

void MapManager::Init(Palkia::Nitro::Rom* rom){
    auto arm9 = rom->GetFile("arm9.bin");
    bStream::CMemoryStream armStream(arm9->GetData(), arm9->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    std::cout << "ARM9 Binary Size is 0x" << std::hex << arm9->GetSize() << std::dec << std::endl;
    armStream.seek(0xE601C);

    for(int i = 0; i < 500; i++){
        std::shared_ptr<MapChunkHeader> header = std::make_shared<MapChunkHeader>();
        header->Read(armStream);
        mChunkHeaders.push_back(header);
    }

    auto mapChunkFile = rom->GetFile("fielddata/land_data/land_data.narc");
    auto mapTexFile = rom->GetFile("fielddata/areadata/area_map_tex/map_tex_set.narc");
    auto buildModelFile = rom->GetFile("fielddata/build_model/build_model.narc");
    auto areaDataFile = rom->GetFile("fielddata/areadata/area_data.narc");
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

void MapManager::LoadZone(uint32_t nameID){
    // check if a map is already loaded, then save + clear
    mMatrices.clear();

    std::vector<uint32_t> matrixIndices = {};
    for(auto header : mChunkHeaders){
        if(header->mPlaceNameID == nameID){
            matrixIndices.push_back(header->mMatrixID);
        }
    }

    //if(matrixIndices.)

    for(auto matrixIndex : matrixIndices){
        mMatrices.push_back(std::make_unique<Matrix>());
        mMatrices.back()->Load(mMatrixArchive->GetFileByIndex(matrixIndex), mMapChunkArchive, mChunkHeaders);
    }

}