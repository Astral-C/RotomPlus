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
    "fielddata/mm_list/move_model_list.narc",
    "(none)",
    433,
    412,
    false
};

const GameConfig SoulSilver {
    0xF6BE0,
    "a/0/4/2", //area data
    "fielddata/maptable/mapname.bin", //???
    "a/0/6/5", //landdata
    "a/0/4/4", // tex set
    "a/0/4/0", // build models
    "a/0/4/1", //matrix
    "a/0/3/2", // zone event
    "a/1/3/6", //encounters
    "a/0/2/7", // message
    "a/0/8/1", //move model
    "(none)",
    "a/1/4/8",
    279,
    237,
    true
};

std::map<uint32_t, const GameConfig> Configs {
    { (uint32_t)'EUPC', Platinum },
    { (uint32_t)'EGPI', SoulSilver }
};
