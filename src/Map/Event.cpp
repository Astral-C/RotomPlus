#include "NDS/Assets/NSBMD.hpp"
#include "NDS/Assets/NSBTX.hpp"
#include "NDS/System/Archive.hpp"
#include "UPointSpriteManager.hpp"
#include "Map/Event.hpp"
#include <format>
#include <vector>

std::vector<std::vector<uint8_t>> mOverworldTextures;

void LoadEventModel(Palkia::Nitro::Archive& arc, CPointSpriteManager& manager){
    auto evModel = std::make_shared<Palkia::Formats::NSBMD>();
    bStream::CMemoryStream stream(arc.GetFileByIndex(421)->GetData(), arc.GetFileByIndex(421)->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    evModel->Load(stream);

    manager.Init(32, 421);

    for(int x = 0; x <= 420; x++){
        bStream::CMemoryStream texStrm(arc.GetFileByIndex(x)->GetData(), arc.GetFileByIndex(x)->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
        Palkia::Formats::NSBTX tex;
        tex.Load(texStrm);
        std::vector<uint8_t> texDataConverted = tex.GetTextures().Items()[0].second->Convert(*tex.GetPalettes().Items()[0].second);
        manager.SetBillboardTextureData(texDataConverted.data(), x);
    }
}

void RenderEvent(glm::mat4 m, uint32_t id, uint32_t sprite){

}

EventData LoadEvents(std::shared_ptr<Palkia::Nitro::File> file, std::shared_ptr<Palkia::Nitro::File> moveModelFile){
    bStream::CMemoryStream eventStream(file->GetData(), file->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);
    bStream::CMemoryStream moveModelStream(moveModelFile->GetData(), moveModelFile->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

    std::vector<uint16_t> moveModelIDs;
    while(moveModelStream.getSize() - moveModelStream.tell() >= 2){
        uint16_t id = moveModelStream.readUInt16();
        std::cout << "read movemodel ID " << id << std::endl;
        moveModelIDs.push_back(id);
    }

    EventData events = {};

    std::size_t spawnables = static_cast<std::size_t>(eventStream.readUInt32());
    events.spawnEvents.reserve(spawnables);
    for(std::size_t i = 0; i < spawnables; i++){
        // 0x14
        Spawnable spawn;
        spawn.eventType = EventType::Spawnable;
        spawn.id = GetID();
        spawn.scriptNum = eventStream.readInt16();
        spawn.type = eventStream.readInt16();
        spawn.x = eventStream.readInt16();
        spawn.unk1 = eventStream.readUInt16();
        spawn.y = eventStream.readInt16();
        spawn.z = eventStream.readInt32();
        spawn.unk2 = eventStream.readUInt16();
        spawn.orientation = eventStream.readUInt16();
        spawn.unk3 = eventStream.readUInt16();
        std::cout << std::format("Reading Spawn Evt w/ Position {},{},{}", spawn.x, spawn.y, spawn.z) << std::endl;
        events.spawnEvents.emplace_back(std::move(spawn));
    }

    std::size_t overworlds = static_cast<std::size_t>(eventStream.readUInt32());
    events.overworldEvents.reserve(overworlds);
    for(std::size_t i = 0; i < overworlds; i++){
        // 0x20
        Overworld overworld;
        overworld.eventType = EventType::Overworld;
        overworld.id = GetID();
        overworld.owID = eventStream.readUInt16();
        overworld.overlayID = eventStream.readUInt16();
        overworld.movementType = eventStream.readUInt16();
        overworld.type = eventStream.readUInt16();
        overworld.flag = eventStream.readUInt16();
        overworld.script = eventStream.readUInt16();
        overworld.orientation = eventStream.readUInt16();
        overworld.range = eventStream.readUInt16();
        overworld.unk1 = eventStream.readUInt16();
        overworld.unk2 = eventStream.readUInt16();

        overworld.xrange = eventStream.readUInt16();
        overworld.yrange = eventStream.readUInt16();

        overworld.x = eventStream.readInt16();
        overworld.y = eventStream.readInt16();
        overworld.z = eventStream.readUInt32();

        std::cout << std::format("Reading Overworld Evt w/ Position {},{},{}", overworld.x, overworld.y, overworld.z) << std::endl;

        events.overworldEvents.emplace_back(std::move(overworld));
    }

    std::size_t warps = static_cast<std::size_t>(eventStream.readUInt32());
    events.warpEvents.reserve(warps);
    for(std::size_t i = 0; i < warps; i++){
        // 0xC
        Warp warp;
        warp.eventType = EventType::Warp;
        warp.id = GetID();
        warp.x = eventStream.readUInt16();
        warp.y = eventStream.readUInt16();
        warp.targetHeader = eventStream.readUInt16();
        warp.anchor = eventStream.readUInt16();
        warp.height = eventStream.readUInt32();
        std::cout << std::format("Reading Warp Evt w/ Position {},{}", warp.x, warp.y) << std::endl;
        events.warpEvents.emplace_back(std::move(warp));
    }

    std::size_t triggers = static_cast<std::size_t>(eventStream.readUInt32());
    events.triggerEvents.reserve(triggers);
    for(std::size_t i = 0; i < triggers; i++){
        // 0x10
        Trigger trigger;
        trigger.eventType = EventType::Trigger;
        trigger.id = GetID();
        trigger.scriptNum = eventStream.readUInt16();
        trigger.x = eventStream.readInt16();
        trigger.y = eventStream.readInt16();
        trigger.w = eventStream.readUInt16();
        trigger.h = eventStream.readUInt16();
        trigger.z = eventStream.readUInt16();
        trigger.expect = eventStream.readUInt16();
        trigger.watch = eventStream.readUInt16();
        std::cout << std::format("Reading Trigger Evt w/ Position {},{},{}", trigger.x, trigger.y, trigger.z) << std::endl;
        events.triggerEvents.emplace_back(std::move(trigger));
    }
    events.triggerEvents.shrink_to_fit();

    return events;
}

void SaveEvents(std::shared_ptr<Palkia::Nitro::File> file, EventData events){
    bStream::CMemoryStream eventStream(0x10, bStream::Endianess::Little, bStream::OpenMode::Out);

    eventStream.writeUInt32(events.spawnEvents.size());
    for(auto spawnable : events.spawnEvents){
        eventStream.writeUInt16(spawnable.scriptNum);
        eventStream.writeUInt16(spawnable.type);
        eventStream.writeUInt32(spawnable.x);
        eventStream.writeUInt32(spawnable.y);
        eventStream.writeUInt32(spawnable.z);
        eventStream.writeUInt32(spawnable.orientation);
    }


    eventStream.writeUInt32(events.overworldEvents.size());
    for(auto overworld : events.overworldEvents){
        eventStream.writeUInt16(overworld.owID);
        eventStream.writeUInt16(overworld.overlayID);
        eventStream.writeUInt16(overworld.movementType);
        eventStream.writeUInt16(overworld.type);
        eventStream.writeUInt16(overworld.flag);
        eventStream.writeUInt16(overworld.script);
        eventStream.writeUInt16(overworld.orientation);
        eventStream.writeUInt16(overworld.range);
        eventStream.writeUInt16(overworld.unk1);
        eventStream.writeUInt16(overworld.unk2);
        eventStream.writeUInt16(overworld.xrange);
        eventStream.writeUInt16(overworld.yrange);
        eventStream.writeUInt16(overworld.x);
        eventStream.writeUInt16(overworld.y);
        eventStream.writeUInt32(overworld.z);
    }


    eventStream.writeUInt32(events.warpEvents.size());
    for(auto warp : events.warpEvents){
        eventStream.writeUInt16(warp.x);
        eventStream.writeUInt16(warp.y);
        eventStream.writeUInt16(warp.targetHeader);
        eventStream.writeUInt16(warp.anchor);
        eventStream.writeUInt32(warp.height);
    }


    eventStream.writeUInt32(events.triggerEvents.size());
    for(auto trigger : events.triggerEvents){
        eventStream.writeUInt16(trigger.scriptNum);
        eventStream.writeUInt16(trigger.x);
        eventStream.writeUInt16(trigger.y);
        eventStream.writeUInt16(trigger.w);
        eventStream.writeUInt16(trigger.h);
        eventStream.writeUInt16(trigger.z);
        eventStream.writeUInt16(trigger.expect);
        eventStream.writeUInt16(trigger.watch);
    }

    file->SetData(eventStream.getBuffer(), eventStream.getSize());
}
