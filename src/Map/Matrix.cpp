#include <Map/Matrix.hpp>

Matrix::Matrix(){}
Matrix::~Matrix(){}

void Matrix::Load(std::shared_ptr<Palkia::Nitro::File> matrixData, std::weak_ptr<Palkia::Nitro::Archive> fieldDataArchive, std::vector<std::shared_ptr<MapChunkHeader>>& mHeaders){
    bStream::CMemoryStream stream(matrixData->GetData(), matrixData->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    
    mWidth = stream.readUInt8();
    mHeight = stream.readUInt8();

    uint8_t hasHeaders = stream.readUInt8();
    uint8_t hasAltitude = stream.readUInt8();
    uint8_t nameLength = stream.readUInt8();

    mName = stream.readString(nameLength);

    mEntries.resize(mWidth * mHeight);

    if(hasHeaders){
        for (uint8_t y = 0; y < mHeight; y++){
            for (uint8_t x = 0; x < mWidth; x++){
                mEntries[(y * mWidth) + x].mChunkHeader = mHeaders[stream.readUInt16()];
            }
        }
    } else {
        for (uint8_t y = 0; y < mHeight; y++){
            for (uint8_t x = 0; x < mWidth; x++){
                mEntries[(y * mWidth) + x].mChunkHeader = mHeaders.front();
            }
        }
    }
    
    if(hasAltitude){
        for (uint8_t y = 0; y < mHeight; y++){
            for (uint8_t x = 0; x < mWidth; x++){
                mEntries[(y * mWidth) + x].mHeight = stream.readUInt8();
            }
        }
    }

    for (uint8_t y = 0; y < mHeight; y++){
        for (uint8_t x = 0; x < mWidth; x++){
            
            uint16_t mapChunkID = stream.readUInt16();
            if(!mChunks.contains(mapChunkID)){
                auto chunkData = fieldDataArchive.lock()->GetFileByIndex(mapChunkID);
                bStream::CMemoryStream chunkStream(chunkData->GetData(), chunkData->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
                mChunks[mapChunkID] = std::make_shared<MapChunk>(chunkStream);
            }

            mEntries[(y * mWidth) + x].mChunk = mChunks[mapChunkID];
        }
    }
    

}
