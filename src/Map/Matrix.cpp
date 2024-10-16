#include <Map/Matrix.hpp>

Matrix::Matrix(){}
Matrix::~Matrix(){}

void Matrix::Draw(glm::mat4 v, uint32_t placeNameID){
    for (uint8_t y = 0; y < mHeight; y++){
        for (uint8_t x = 0; x < mWidth; x++){
            // do a check here to see if its directly next to the current chunk and if it is (and hasnt been drawn) - draw it so we can see the border of the current map
            if(mEntries[(y * mWidth) + x].mChunk != nullptr && mEntries[(y * mWidth) + x].mChunkHeader.lock() && mEntries[(y * mWidth) + x].mChunkHeader.lock()->mPlaceNameID == placeNameID){
                mEntries[(y * mWidth) + x].mChunk->Draw(x, y, mEntries[(y * mWidth) + x].mHeight, v);
            }
        }
    }
}

Building* Matrix::MoveBuilding(Building* building, uint32_t cx, uint32_t cy, uint32_t ncx, uint32_t ncy){
    auto oldChunk = mEntries[(cy * mWidth) + cx].mChunk;
    auto newChunk = mEntries[(ncy * mWidth) + ncx].mChunk;

    Building b = *building;

    if(oldChunk) oldChunk->RemoveBuilding(building);
    if(newChunk){
        return newChunk->AddBuilding(b);
    }
    return nullptr; // this shouldnt happen
}

std::pair<Building*, std::pair<uint8_t, uint8_t>> Matrix::Select(uint32_t id){
    for (uint8_t y = 0; y < mHeight; y++){
        for (uint8_t x = 0; x < mWidth; x++){
            if(mEntries[(y * mWidth) + x].mChunk != nullptr){
                Building* ptr = mEntries[(y * mWidth) + x].mChunk->Select(id);
                if(ptr != nullptr){
                    return {ptr, {x, y}};
                }
            }
        }
    }
    return {nullptr, {0,0}};
}

void Matrix::Load(std::shared_ptr<Palkia::Nitro::File> matrixData, std::weak_ptr<Palkia::Nitro::Archive> fieldDataArchive, std::vector<std::shared_ptr<MapChunkHeader>>& mHeaders, std::shared_ptr<MapChunkHeader> matrixHeader){
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
                uint16_t header = stream.readUInt16();
                if(header != 0xFFFF){
                    mEntries[(y * mWidth) + x].mChunkHeader = mHeaders[header];
                } else {
                    mEntries[(y * mWidth) + x].mChunk = {};
                }
            }
        }
    } else {
        for (uint8_t y = 0; y < mHeight; y++){
            for (uint8_t x = 0; x < mWidth; x++){
                mEntries[(y * mWidth) + x].mChunkHeader = matrixHeader;
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
            if(mapChunkID != 0xFFFF){
                auto chunkData = fieldDataArchive.lock()->GetFileByIndex(mapChunkID);
                bStream::CMemoryStream chunkStream(chunkData->GetData(), chunkData->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

                mEntries[(y * mWidth) + x].mChunk = std::make_shared<MapChunk>(mapChunkID, chunkStream);
            } else {
                mEntries[(y * mWidth) + x].mChunk = {};
            }
        }
    }
    

}
