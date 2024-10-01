#ifndef __POKEDATA_H__
#define __POKEDATA_H__
#include <vector>
#include <string>
#include <NDS/System/Archive.hpp>

extern std::vector<std::string> PokemonNames;

void LoadPokemonNames(std::shared_ptr<Palkia::Nitro::File> msgFile);

#endif