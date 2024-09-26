#include "Chunk.hpp"
#include <memory>
#include <vector>

struct MatrixEntry {
    uint8_t mHeight;
    std::weak_ptr<MapChunk> mChunk;
    std::weak_ptr<MapChunkHeader> mChunkHeader;
};

class Matrix
{
private:
    std::vector<std::shared_ptr<MapChunk>> mChunks;
public:
    Matrix(/* args */);
    ~Matrix();
};
