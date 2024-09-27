#include "Map/Chunk.hpp"

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
    
}

MapChunk::~MapChunk(){

}