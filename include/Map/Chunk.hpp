#include <NDS/System/Archive.hpp>

struct MapChunkHeader {

};

class MapChunk {


public:
    void LoadGraphics(std::shared_ptr<Palkia::Nitro::File> fieldData);

    MapChunk();

};