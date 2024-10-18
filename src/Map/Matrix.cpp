#include <Map/Map.hpp>
#include <Map/Matrix.hpp>
#include <map>
#include <set>

Matrix::Matrix(){}
Matrix::~Matrix(){}

void Matrix::LoadGraphics(std::shared_ptr<Palkia::Nitro::Archive> buildingArchive, std::shared_ptr<Palkia::Nitro::Archive> mapTex, std::vector<Area>& areas, uint32_t placeNameID){
    std::set<std::pair<uint32_t, uint32_t>> loaded = {};
    for (uint8_t y = 0; y < mHeight; y++){
        for (uint8_t x = 0; x < mWidth; x++){
            if(loaded.count({x, y}) != 0) continue; // loaded already, skip
            bool neighborHasPlace = false;
            uint8_t placeArea = 0xFF;
            for(int ly = -1; ly <= 1; ly++){
                for(int lx = -1; lx <= 1; lx++){
                    if((lx == 0 && ly == 0) || x + lx >= mWidth || y + ly >= mHeight || y + ly < 0 || x + lx < 0) continue;
                    if(mEntries[((ly + y) * mWidth) + (x + lx)].mChunkHeader.lock() && mEntries[((ly + y) * mWidth) + (x + lx)].mChunkHeader.lock()->mPlaceNameID == placeNameID){
                        neighborHasPlace = true;
                        placeArea = mEntries[((ly + y) * mWidth) + (x + lx)].mChunkHeader.lock()->mAreaID;
                        break;
                    }
                }
            }

            if(mEntries[(y * mWidth) + x].mChunk != nullptr&& mEntries[(y * mWidth) + x].mChunkHeader.lock() && (mEntries[(y * mWidth) + x].mChunkHeader.lock()->mPlaceNameID == placeNameID || neighborHasPlace)){
                loaded.insert({x, y});
                int texSet = areas[mEntries[(y * mWidth) + x].mChunkHeader.lock()->mAreaID].mMapTileset;
                if(mEntries[(y * mWidth) + x].mChunkHeader.lock()->mAreaID == 0){
                    texSet = areas[placeArea].mMapTileset;
                }
                mEntries[(y * mWidth) + x].mChunk->LoadGraphics(mapTex->GetFileByIndex(texSet), buildingArchive);
            }
        }
    }    
}

void Matrix::Draw(glm::mat4 v, uint32_t placeNameID){
    for (uint8_t y = 0; y < mHeight; y++){
        for (uint8_t x = 0; x < mWidth; x++){
            // do a check here to see if its directly next to the current chunk and if it is (and hasnt been drawn) - draw it so we can see the border of the current map
            bool neighborHasPlace = false;
            for(int ly = -1; ly <= 1; ly++){
                for(int lx = -1; lx <= 1; lx++){
                    if((lx == ly && ly == 0) || x + lx >= mWidth || y + ly >= mHeight || y + ly < 0 || x + lx < 0) continue;
                    if(mEntries[((ly + y) * mWidth) + (x + lx)].mChunkHeader.lock() && mEntries[((ly + y) * mWidth) + (x + lx)].mChunkHeader.lock()->mPlaceNameID == placeNameID){
                        neighborHasPlace = true;
                        break;
                    }
                }
            }

            if(mEntries[(y * mWidth) + x].mChunk != nullptr&& mEntries[(y * mWidth) + x].mChunkHeader.lock() && (mEntries[(y * mWidth) + x].mChunkHeader.lock()->mPlaceNameID == placeNameID || neighborHasPlace)){
                mEntries[(y * mWidth) + x].mChunk->Draw(x, y, mEntries[(y * mWidth) + x].mHeight * 8, v);
            }
        }
    }
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
