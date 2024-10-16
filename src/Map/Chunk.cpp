#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <NDS/Assets/NSBMD.hpp>
#include <NDS/Assets/NSBTX.hpp>
#include "IDManager.hpp"
#include "Map/Chunk.hpp"
#include "Util.hpp"

namespace MapGraphicsHandler {
    std::map<uint16_t, Palkia::Formats::NSBMD*> mLoadedChunkModels;
    std::map<uint16_t, Palkia::Formats::NSBMD*> mLoadedModels;
    void ClearModelCache(){
        for(auto [id, model] : mLoadedChunkModels){
            delete model;
        }

        for(auto [id, model] : mLoadedModels){
            delete model;
        }

        mLoadedChunkModels.clear();
        mLoadedModels.clear();
    }
}

void MapChunkHeader::Read(bStream::CStream& stream){
    mAreaID = stream.readUInt8();
    mMoveModelID = stream.readUInt8();
    mMatrixID = stream.readUInt16();
    mScriptID = stream.readUInt16();
    mSpScriptID = stream.readUInt16();
    mMsgID = stream.readUInt16();
    mBgmDayID = stream.readUInt16();
    mBgmNightID = stream.readUInt16();
    mEncDataID = stream.readUInt16();
    mEventDataID = stream.readUInt16();
    mPlaceNameID = stream.readUInt8();
    mTextBoxType = stream.readUInt8();
    mWeatherID = stream.readUInt8();
    mCameraID = stream.readUInt8();
    mMapType = stream.readUInt8();

    uint8_t byte = stream.readUInt8();
    mBattleBgType = byte & 0b11110000;
    mBicycleFlag = byte & 0b00001000;
    mDashFlag = byte & 0b00000100;
    mEscapeFlag = byte & 0b00000010;
    mFlyFlag = byte & 0b00000001;
}

void MapChunk::LoadGraphics(std::shared_ptr<Palkia::Nitro::File> mapTex, std::shared_ptr<Palkia::Nitro::Archive> buildModels){
    bStream::CMemoryStream mapModelStream(mModelData.data(), mModelData.size(), bStream::Endianess::Little, bStream::OpenMode::In);
    Palkia::Formats::NSBMD* mapModel = new Palkia::Formats::NSBMD();
    mapModel->Load(mapModelStream);
    
    bStream::CMemoryStream mapTexStrm(mapTex->GetData(), mapTex->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    Palkia::Formats::NSBTX mapTextureSet;
    mapTextureSet.Load(mapTexStrm);

    mapModel->AttachNSBTX(&mapTextureSet);

    MapGraphicsHandler::mLoadedChunkModels[mID] = mapModel;

    std::cout << "Building Count " << mBuildings.size() << std::endl;
    for(auto building : mBuildings){
        std::cout << "Building ID " << building.mModelID << std::endl;
        if(!MapGraphicsHandler::mLoadedModels.contains(building.mModelID) && building.mModelID < buildModels->GetFileCount()){
            auto buildingModel = buildModels->GetFileByIndex(building.mModelID);
            bStream::CMemoryStream buildingModelStream(buildingModel->GetData(), buildingModel->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
            MapGraphicsHandler::mLoadedModels[building.mModelID] = new Palkia::Formats::NSBMD();
            MapGraphicsHandler::mLoadedModels[building.mModelID]->Load(buildingModelStream);
        }
    }

    

}

void MapChunk::Draw(uint8_t cx, uint8_t cy, uint8_t cz, glm::mat4 v){
    glm::mat4 chunkTransform = v * glm::translate(glm::mat4(1.0f), glm::vec3(cx * 512, cz, cy * 512));
    if(MapGraphicsHandler::mLoadedChunkModels[mID] != nullptr){
        MapGraphicsHandler::mLoadedChunkModels[mID]->Render(chunkTransform, 0);
    }

    for(auto building : mBuildings){
        glm::mat4 modelTransform = v * glm::translate(glm::mat4(1.0f), glm::vec3((cx * 512) + building.x, cz + building.y, (cy * 512) + building.z));
        if(MapGraphicsHandler::mLoadedModels[building.mModelID] != nullptr) MapGraphicsHandler::mLoadedModels[building.mModelID]->Render(modelTransform, building.mPickID);
    }

}

Building* MapChunk::Select(uint32_t id){
    std::cout << "Selecting building in chunk ID " << mID << std::endl;
    for(int i = 0; i < mBuildings.size(); i++){
        if(id != 0 && id == mBuildings[i].mPickID){
            return &mBuildings[i];
        }
    }
    return nullptr;
}

MapChunk::MapChunk(uint16_t id, bStream::CStream& stream){
    mID = id;
    uint32_t permissionsSize = stream.readUInt32();
    uint32_t buildingsSize = stream.readUInt32();
    uint32_t modelSize = stream.readUInt32();
    uint32_t bdhcSize = stream.readUInt32();

    std::cout << "Reading chunk " << id << " with " << buildingsSize << " size building chunk" << std::endl;

    for(int i = 0; i < 1024; i++){
        mMovementPermissions[i].first = stream.readUInt8();
        mMovementPermissions[i].second = stream.readUInt8();
    }
    
    stream.seek(permissionsSize + 0x10);
    while(stream.tell() < permissionsSize + buildingsSize + 0x10){
        Building b;
        b.mModelID = stream.readUInt32();
        b.x = Palkia::fixed(stream.readInt32());
        b.y = Palkia::fixed(stream.readInt32());
        b.z = Palkia::fixed(stream.readInt32());
        b.rx = Palkia::fixed(stream.readInt32());
        b.ry = Palkia::fixed(stream.readInt32());
        b.rz = Palkia::fixed(stream.readInt32());
        b.l = Palkia::fixed(stream.readUInt32());
        b.w = Palkia::fixed(stream.readUInt32());
        b.h = Palkia::fixed(stream.readUInt32());
        b.unk1 = stream.readUInt32();
        b.unk2 = stream.readUInt32();

        b.mPickID = GetID();
        mBuildings.push_back(b);
    }

    std::cout << "Read " << mBuildings.size() << " buildings" << std::endl; 

    stream.seek(permissionsSize + buildingsSize + 0x10);
    // read model buffer into memory so we can load this as and when we need to!
    mModelData = {};
    mModelData.resize(modelSize);
    stream.readBytesTo(mModelData.data(), modelSize);

    // TODO: figure out bdhc stuff! 
    mBDHCData = {};
    mBDHCData.resize(bdhcSize);
    stream.seek(modelSize + permissionsSize + buildingsSize + 0x10);
    stream.readBytesTo(mBDHCData.data(), bdhcSize);
}

void MapChunk::Save(std::shared_ptr<Palkia::Nitro::Archive> archive){
    uint32_t dataSize = (mMovementPermissions.size() * 2) + (0x30 * mBuildings.size()) + mModelData.size() + mBDHCData.size();
    bStream::CMemoryStream stream(dataSize, bStream::Endianess::Little, bStream::OpenMode::Out);

    std::cout << "Saving Chunk ID " << mID << std::endl;

    stream.writeUInt32(mMovementPermissions.size() * 2);
    stream.writeUInt32(0x30 * mBuildings.size());
    stream.writeUInt32(mModelData.size());
    stream.writeUInt32(mBDHCData.size());

    for(int i = 0; i < 1024; i++){
        stream.writeUInt8(mMovementPermissions[i].first);
        stream.writeUInt8(mMovementPermissions[i].second);
    }
    
    for(auto building : mBuildings){
        stream.writeUInt32(building.mModelID);
        
        stream.writeUInt32((uint32_t)(building.x * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.y * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.z * (1 << 12)));

        stream.writeUInt32((uint32_t)(building.rx * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.ry * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.rz * (1 << 12)));

        stream.writeUInt32((uint32_t)(building.l * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.w * (1 << 12)));
        stream.writeUInt32((uint32_t)(building.h * (1 << 12)));

        stream.writeUInt32(building.unk1);
        stream.writeUInt32(building.unk2);
    }

    stream.writeBytes(mModelData.data(), mModelData.size());
    stream.writeBytes(mBDHCData.data(), mBDHCData.size());

    archive->GetFileByIndex(mID)->SetData(stream.getBuffer(), stream.getSize());
}

MapChunk::~MapChunk(){

}