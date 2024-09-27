#include "Chunk.hpp"
#include <memory>
#include <vector>

struct MatrixEntry {
    uint8_t mHeight;
    std::weak_ptr<MapChunk> mChunk;
    std::weak_ptr<MapChunkHeader> mChunkHeader;
};

class Matrix {
    int mWidth, mHeight;
    std::vector<std::shared_ptr<MapChunk>> mChunks;
public:
    void Load(std::shared_ptr<Palkia::Nitro::File> matrixData, std::shared_ptr<Palkia::Nitro::Archive> fieldDataArchive);

    Matrix();
    ~Matrix();
};
