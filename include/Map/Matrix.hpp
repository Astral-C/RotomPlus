#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <glm/glm/glm.hpp>
#include "Chunk.hpp"
#include <memory>
#include <vector>
#include <map>

struct MatrixEntry {
    uint8_t mHeight;
    std::shared_ptr<MapChunk> mChunk; // I wish this could be weak BUT there is a weird issue with duplicates chunk IDs
    std::weak_ptr<MapChunkHeader> mChunkHeader;
};

class Matrix {
    std::string mName { "[unset]" };
    int mWidth, mHeight;
    std::vector<MatrixEntry> mEntries;
    std::weak_ptr<MapChunkHeader> mMainChunkHeader; // this is horrible, used for maps with only one header!
    std::map<uint16_t, std::shared_ptr<MapChunk>> mChunks;
public:
    std::string GetName() { return mName; }
    std::vector<MatrixEntry>& GetEntries() { return mEntries; }

    int GetWidth() { return mWidth; }
    int GetHeight() { return mHeight; }
    void LoadGraphics(std::shared_ptr<Palkia::Nitro::Archive>, std::shared_ptr<Palkia::Nitro::Archive>, std::vector<Area>&, uint32_t);

    Building* MoveBuilding(Building*, uint32_t, uint32_t, uint32_t, uint32_t);
    std::pair<Building*, std::pair<uint8_t, uint8_t>> Select(uint32_t);
    void Draw(glm::mat4, uint32_t);
    void Load(std::shared_ptr<Palkia::Nitro::File>, std::weak_ptr<Palkia::Nitro::Archive>, std::vector<std::shared_ptr<MapChunkHeader>>&, std::shared_ptr<MapChunkHeader>, uint32_t);

    Matrix();
    ~Matrix();
};

#endif