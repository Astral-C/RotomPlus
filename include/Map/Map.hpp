#include <vector>
#include <memory>
#include "Matrix.hpp"


// Equivilent to "Zone"
class Map {
private:
    std::vector<Matrix> mMatrices;
    std::vector<std::shared_ptr<MapChunk>> mChunks;
public:
    Map(/* args */);
    ~Map();
};
