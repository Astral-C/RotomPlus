#include "PokemonData.hpp"
#include "Text.hpp"

std::vector<std::string> PokemonNames = {"NAMES NOT LOADED"};

void LoadPokemonNames(std::shared_ptr<Palkia::Nitro::File> msgFile){
    bStream::CMemoryStream memStream(msgFile->GetData(), msgFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    PokemonNames = Text::DecodeStringList(memStream);
}