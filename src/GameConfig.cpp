#include "GameConfig.hpp"

const GameConfig Platinum {
    0xE601C,
    "fielddata/areadata/area_data.narc",
    "fielddata/maptable/mapname.bin",
    "fielddata/land_data/land_data.narc",
    "fielddata/areadata/area_map_tex/map_tex_set.narc",
    "fielddata/build_model/build_model.narc",
    "fielddata/mapmatrix/map_matrix.narc",
    "fielddata/eventdata/zone_event.narc",
    "fielddata/encountdata/pl_enc_data.narc",
    "msgdata/pl_msg.narc",
    "data/mmodel/mmodel.narc",
    433,
    412
};

const GameConfig SoulSilver {
    0xF6BE0,
    "data/a/0/4/2", //area data
    "fielddata/maptable/mapname.bin", //???
    "data/a/0/6/5", //landdata
    "data/a/0/4/4", // tex set
    "data/a/0/4/0", // build models
    "data/a/0/4/1", //matrix
    "data/a/0/3/2", // zone event
    "data/a/1/3/6", //encounters
    "data/a/0/2/7", // message
    "data/a/0/8/1", //move model
    279,
    236
};

std::map<std::string, const GameConfig> Configs {
    { "CPUE", Platinum },
    { "IPGE", SoulSilver } 
};