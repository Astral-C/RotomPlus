#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "Chunk.hpp"
#include <memory>
#include <vector>
#include <map>

struct MatrixEntry {
    uint8_t mHeight;
    std::weak_ptr<MapChunk> mChunk;
    std::weak_ptr<MapChunkHeader> mChunkHeader;
};

class Matrix {
    std::string mName;
    int mWidth, mHeight;
    std::vector<MatrixEntry> mEntries;
    std::map<uint16_t, std::shared_ptr<MapChunk>> mChunks;
public:
    void Load(std::shared_ptr<Palkia::Nitro::File> matrixData, std::weak_ptr<Palkia::Nitro::Archive> fieldDataArchive, std::vector<std::shared_ptr<MapChunkHeader>>& mHeaders);

    Matrix();
    ~Matrix();
};

#endif