#include "Map/Encounters.hpp"

Encounter LoadEncounterFile(std::shared_ptr<Palkia::Nitro::File> encounterFile){
    bStream::CMemoryStream encounterStream(encounterFile->GetData(), encounterFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    // Reall shut put this all into a read encounters func...
    
    Encounter mEncounters;
    
    mEncounters.mWalkingEncounterRate = encounterStream.readInt32();
    for(int i = 0; i < 12; i++){
        mEncounters.mWalkingLevel[i] = encounterStream.readUInt32();
        mEncounters.mWalking[i] = encounterStream.readUInt32();
    }

    mEncounters.mSwarmPokemon[0] = encounterStream.readUInt32();
    mEncounters.mSwarmPokemon[1] = encounterStream.readUInt32();

    mEncounters.mMorningPokemon[0] = encounterStream.readUInt32();
    mEncounters.mMorningPokemon[1] = encounterStream.readUInt32();

    mEncounters.mNightPokemon[0] = encounterStream.readUInt32();
    mEncounters.mNightPokemon[1] = encounterStream.readUInt32();

    mEncounters.mRadarPokemon[0] = encounterStream.readUInt32();
    mEncounters.mRadarPokemon[1] = encounterStream.readUInt32();
    mEncounters.mRadarPokemon[2] = encounterStream.readUInt32();
    mEncounters.mRadarPokemon[3] = encounterStream.readUInt32();

    encounterStream.seek(0xA4); // I don't like this... Figure out if there is a better way to handle this

    mEncounters.mRuby[0] = encounterStream.readUInt32();
    mEncounters.mRuby[1] = encounterStream.readUInt32();

    mEncounters.mSapphire[0] = encounterStream.readUInt32();
    mEncounters.mSapphire[1] = encounterStream.readUInt32();

    mEncounters.mEmerald[0] = encounterStream.readUInt32();
    mEncounters.mEmerald[1] = encounterStream.readUInt32();

    mEncounters.mFireRed[0] = encounterStream.readUInt32();
    mEncounters.mFireRed[1] = encounterStream.readUInt32();

    mEncounters.mLeafGreen[0] = encounterStream.readUInt32();
    mEncounters.mLeafGreen[1] = encounterStream.readUInt32();

    mEncounters.mSurfEncounterRate = encounterStream.readInt32();
    for(int i = 0; i < 5; i++){
        mEncounters.mSurfMaxLevels[i] = encounterStream.readUInt8();
        mEncounters.mSurfMinLevels[i] = encounterStream.readUInt8();
        encounterStream.skip(2); // is this padding? or is something here???
        mEncounters.mSurf[i] = encounterStream.readUInt32();
    }

    encounterStream.seek(0x214); //again, is there a better way to do this

    mEncounters.mOldRodRate = encounterStream.readInt32();
    for(int i = 0; i < 5; i++){
        mEncounters.mOldMaxLevels[i] = encounterStream.readUInt8();
        mEncounters.mOldMinLevels[i] = encounterStream.readUInt8();
        encounterStream.skip(2); // is this padding? or is something here???
        mEncounters.mOldRod[i] = encounterStream.readUInt32();
    }

    mEncounters.mGoodRodRate = encounterStream.readInt32();
    for(int i = 0; i < 5; i++){
        mEncounters.mGoodRodMaxLevels[i] = encounterStream.readUInt8();
        mEncounters.mGoodRodMinLevels[i] = encounterStream.readUInt8();
        encounterStream.skip(2); // is this padding? or is something here???
        mEncounters.mGoodRod[i] = encounterStream.readUInt32();
    }

    mEncounters.mSuperRodRate = encounterStream.readInt32();
    for(int i = 0; i < 5; i++){
        mEncounters.mSuperRodMaxLevels[i] = encounterStream.readUInt8();
        mEncounters.mSuperRodMinLevels[i] = encounterStream.readUInt8();
        encounterStream.skip(2); // is this padding? or is something here???
        mEncounters.mSuperRod[i] = encounterStream.readUInt32();
    }

    return mEncounters;
}

void SaveEncounterFile(std::shared_ptr<Palkia::Nitro::File> file, Encounter mEncounters){
    bStream::CMemoryStream encounterStream(file->GetData(), file->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    encounterStream.writeInt32(mEncounters.mWalkingEncounterRate);
    for(int i = 0; i < 12; i++){
        encounterStream.writeUInt32(mEncounters.mWalkingLevel[i]);
        encounterStream.writeUInt32(mEncounters.mWalking[i]);
    }

    encounterStream.writeUInt32(mEncounters.mSwarmPokemon[0]);
    encounterStream.writeUInt32(mEncounters.mSwarmPokemon[1]);

    encounterStream.writeUInt32(mEncounters.mMorningPokemon[0]);
    encounterStream.writeUInt32(mEncounters.mMorningPokemon[1]);

    encounterStream.writeUInt32(mEncounters.mNightPokemon[0]);
    encounterStream.writeUInt32(mEncounters.mNightPokemon[1]);

    encounterStream.writeUInt32(mEncounters.mRadarPokemon[0]);
    encounterStream.writeUInt32(mEncounters.mRadarPokemon[1]);
    encounterStream.writeUInt32(mEncounters.mRadarPokemon[2]);
    encounterStream.writeUInt32(mEncounters.mRadarPokemon[3]);

    encounterStream.seek(0xA4); // I don't like this... Figure out if there is a better way to handle this

    encounterStream.writeUInt32(mEncounters.mRuby[0]);
    encounterStream.writeUInt32(mEncounters.mRuby[1]);

    encounterStream.writeUInt32(mEncounters.mSapphire[0]);
    encounterStream.writeUInt32(mEncounters.mSapphire[1]);

    encounterStream.writeUInt32(mEncounters.mEmerald[0]);
    encounterStream.writeUInt32(mEncounters.mEmerald[1]);

    encounterStream.writeUInt32(mEncounters.mFireRed[0]);
    encounterStream.writeUInt32(mEncounters.mFireRed[1]);

    encounterStream.writeUInt32(mEncounters.mLeafGreen[0]);
    encounterStream.writeUInt32(mEncounters.mLeafGreen[1]);

    encounterStream.writeInt32(mEncounters.mSurfEncounterRate);
    for(int i = 0; i < 5; i++){
        encounterStream.writeUInt8(mEncounters.mSurfMaxLevels[i]);
        encounterStream.writeUInt8(mEncounters.mSurfMinLevels[i]);
        encounterStream.skip(2); // is this padding? or is something here???
        encounterStream.writeUInt32(mEncounters.mSurf[i]);
    }

    encounterStream.seek(0x214); //again, is there a better way to do this

    encounterStream.writeInt32(mEncounters.mOldRodRate);
    for(int i = 0; i < 5; i++){
        encounterStream.writeUInt8(mEncounters.mOldMaxLevels[i]);
        encounterStream.writeUInt8(mEncounters.mOldMinLevels[i]);
        encounterStream.skip(2); // is this padding? or is something here???
        encounterStream.writeUInt32(mEncounters.mOldRod[i]);
    }

    encounterStream.writeInt32(mEncounters.mGoodRodRate);
    for(int i = 0; i < 5; i++){
        encounterStream.writeUInt8(mEncounters.mGoodRodMaxLevels[i]);
        encounterStream.writeUInt8(mEncounters.mGoodRodMinLevels[i]);
        encounterStream.skip(2); // is this padding? or is something here???
        encounterStream.writeUInt32(mEncounters.mGoodRod[i]);
    }

    encounterStream.writeInt32(mEncounters.mSuperRodRate);
    for(int i = 0; i < 5; i++){
        encounterStream.writeUInt8(mEncounters.mSuperRodMaxLevels[i]);
        encounterStream.writeUInt8(mEncounters.mSuperRodMinLevels[i]);
        encounterStream.skip(2); // is this padding? or is something here???
        encounterStream.writeUInt32(mEncounters.mSuperRod[i]);
    }
}