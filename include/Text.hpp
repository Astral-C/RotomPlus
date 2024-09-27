#define BSTREAM_IMPLEMENTATION
#include "bstream/bstream.h"
#include <vector>
#include <string>

namespace Text {

std::string DecodeString(bStream::CStream&, uint16_t&, uint32_t, uint16_t);
std::vector<std::string> DecodeStringList(bStream::CStream&);

}