#include "Map/Chunk.hpp"
#include "Util.hpp"

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

MapChunk::MapChunk(bStream::CStream& stream){
    uint32_t permissionsSize = stream.readUInt32();
    uint32_t buildingsSize = stream.readUInt32();
    uint32_t modelSize = stream.readUInt32();
    uint32_t bdhcSize = stream.readUInt32();

    for(int i = 0; i < 1024; i++){
        mMovementPermissions[i].first = stream.readUInt8();
        mMovementPermissions[i].second = stream.readUInt8();
    }
    
    for(int i = 0; i < (buildingsSize / 0x30); i++){
        Building b;
        b.mModelID = stream.readUInt32();
        b.x = Palkia::fixed(stream.readUInt32());
        b.y = Palkia::fixed(stream.readUInt32());
        b.z = Palkia::fixed(stream.readUInt32());
        b.rx = Palkia::fixed(stream.readUInt32());
        b.ry = Palkia::fixed(stream.readUInt32());
        b.rz = Palkia::fixed(stream.readUInt32());
        mBuildings.push_back(b);        
    }

    // read model buffer into memory so we can load this as and when we need to!
        mModelData = {};
        mModelData.resize(modelSize);
        stream.readBytesTo(mModelData.data(), modelSize);

    // TODO: figure out bdhc stuff! 
}

MapChunk::~MapChunk(){

}