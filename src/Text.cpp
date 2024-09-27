#include "Text.hpp"
#include "CharDict.hpp"
#include <format>

namespace Text {

std::string DecodeString(bStream::CStream& stream, uint32_t key, uint32_t offset, uint16_t size){
    bool compressed = false;
    bool special = false;

    uint32_t r = stream.tell();
    stream.seek(offset);

    std::string decoded = "";

    for(int i = 0; i < size; i++){
        uint16_t chr = stream.readUInt16() ^ key;
        if(chr == 0xE000){
            decoded += "\n";
        } else if(chr == 0x25BC){
            decoded += "\r";
        } else if(chr == 0x25BD){
            decoded += "\f";
        } else if(chr == 0xF100){
            compressed = true;
        } else if(chr == 0xFFFE){
            decoded += "\v";
            special = true;
        } else {
            if(special){
                decoded += std::format("{0:04x}", chr);
                special = false;
            } else if(compressed) {
                uint16_t shift = 0;
                uint16_t trans = 0;
                while(true){
                    uint16_t cmp = chr >> shift;
                    if(shift >= 0xF){
                        shift -= 0xF;
                        if(shift > 0){
                            cmp  = (trans | ((chr << (9 - shift)) & 0x1FF));
                            if((cmp & 0xFF) == 0xFF){
                                break;
                            }
                            if(cmp != 0x0 and cmp != 0x1) {
                                decoded += Characters[cmp];
                            }
                        }
                    } else {
                        cmp = (chr >> shift) & 0x1FF;
                        if((cmp & 0xFF) == 0xFF){
                            break;
                        }
                        if(cmp != 0x0 && cmp != 0x1) {
                            decoded += Characters[cmp];
                        }
                        shift += 9;
                        if(shift < 0xF){
                            trans = (chr >> shift) & 0x1FF;
                            shift += 9;
                        }
                        key += 0x493D;
                        key &= 0xFFFF;
                        chr = stream.readUInt16() ^ key;
                        i += 1;
                    }
                }
            } else {
                if(Characters.contains(chr)){
                    decoded += Characters[chr];
                } else {
                    break;
                }
            }
        }

        key += 0x493D;
        key &= 0xFFFF;
    }

    stream.seek(r);
    return decoded;
}

std::vector<std::string> DecodeStringList(bStream::CStream& stream){
    std::vector<std::string> strings = {};

    uint16_t stringCount = stream.readUInt16();
    uint32_t initialKey = (stream.readUInt16() * 0x2FD) & 0xFFFF;

    for(int s = 0; s < stringCount; s++){
        uint32_t key = (initialKey * (s + 1)) & 0xFFFF;
        key = key | (key << 16);
        uint32_t offset = stream.readUInt32() ^ key;
        uint32_t size = stream.readUInt32() ^ key;
        strings.push_back(DecodeString(stream, (0x91BD3 * (s + 1)) & 0xFFFF, offset, size));
    }

    return strings;
}

}