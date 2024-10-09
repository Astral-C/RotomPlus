#include "Map/Event.hpp"

std::vector<std::shared_ptr<Event>> LoadEvents(std::shared_ptr<Palkia::Nitro::File> file){
    bStream::CMemoryStream eventStream(file->GetData(), file->GetSize(), bStream::Endianess::Little, bStream::OpenMode::In);

    std::vector<std::shared_ptr<Event>> events = {};

    std::size_t spawnables = static_cast<std::size_t>(eventStream.readUInt32());
    for(std::size_t i = 0; i < spawnables; i++){
        // 0x14
        auto spawn = std::make_shared<Spawnable>();
        spawn->eventType = EventType::Spawnable;
        spawn->scriptNum = eventStream.readUInt16();
        spawn->type = eventStream.readUInt16();
        spawn->x = eventStream.readUInt32();
        spawn->y = eventStream.readUInt32();
        spawn->z = eventStream.readUInt32();
        spawn->orientation = eventStream.readUInt32();
        events.push_back(spawn);
        
    }

    std::size_t overworlds = static_cast<std::size_t>(eventStream.readUInt32());
    for(std::size_t i = 0; i < overworlds; i++){
        // 0x20
        auto overworld = std::make_shared<Overworld>();
        overworld->eventType = EventType::Overworld;
        overworld->spriteID = eventStream.readUInt16();
        overworld->overlayID = eventStream.readUInt16();
        overworld->movementType = eventStream.readUInt16();
        overworld->type = eventStream.readUInt16();
        overworld->flag = eventStream.readUInt16();
        overworld->script = eventStream.readUInt16();
        overworld->orientation = eventStream.readUInt16();
        overworld->range = eventStream.readUInt16();
        overworld->unk1 = eventStream.readUInt16();
        overworld->unk2 = eventStream.readUInt16();

        overworld->xrange = eventStream.readUInt16();
        overworld->yrange = eventStream.readUInt16();

        overworld->x = eventStream.readUInt16();
        overworld->y = eventStream.readUInt16();
        overworld->z = eventStream.readUInt32();

        events.push_back(overworld);
    }

    std::size_t warps = static_cast<std::size_t>(eventStream.readUInt32());
    for(std::size_t i = 0; i < warps; i++){
        // 0xC
        auto warp = std::make_shared<Warp>();
        warp->eventType = EventType::Warp;
        warp->x = eventStream.readUInt16();
        warp->y = eventStream.readUInt16();
        warp->targetHeader = eventStream.readUInt16();
        warp->anchor = eventStream.readUInt16();
        warp->height = eventStream.readUInt32();
        events.push_back(warp);
    }

    std::size_t triggers = static_cast<std::size_t>(eventStream.readUInt32());
    for(std::size_t i = 0; i < triggers; i++){
        // 0x10
        auto trigger = std::make_shared<Trigger>();
        trigger->eventType = EventType::Trigger;
        trigger->scriptNum = eventStream.readUInt16();
        trigger->x = eventStream.readUInt16();
        trigger->y = eventStream.readUInt16();
        trigger->w = eventStream.readUInt16();
        trigger->h = eventStream.readUInt16();
        trigger->z = eventStream.readUInt16();
        trigger->expect = eventStream.readUInt16();
        trigger->watch = eventStream.readUInt16();
        events.push_back(trigger);
    }

    return std::move(events);
}

void SaveEvents(std::shared_ptr<Palkia::Nitro::File> file, std::vector<std::shared_ptr<Event>> events){
    bStream::CMemoryStream eventStream(0x10, bStream::Endianess::Little, bStream::OpenMode::Out);

    std::vector<std::shared_ptr<Spawnable>> spawnables;
    std::vector<std::shared_ptr<Overworld>> overworlds;
    std::vector<std::shared_ptr<Warp>> warps;
    std::vector<std::shared_ptr<Trigger>> triggers;

    for(std::shared_ptr<Event> event : events){
        switch (event->eventType){
        case EventType::Spawnable:
            spawnables.push_back(std::dynamic_pointer_cast<Spawnable>(event));
            break;
        case EventType::Overworld:
            overworlds.push_back(std::dynamic_pointer_cast<Overworld>(event));
            break;
        case EventType::Warp:
            warps.push_back(std::dynamic_pointer_cast<Warp>(event));
            break;
        case EventType::Trigger:
            triggers.push_back(std::dynamic_pointer_cast<Trigger>(event));
            break;
        }
    }

    eventStream.writeUInt32(spawnables.size());
    for(auto spawnable : spawnables){
        eventStream.writeUInt16(spawnable->scriptNum);
        eventStream.writeUInt16(spawnable->type);
        eventStream.writeUInt32(spawnable->x);
        eventStream.writeUInt32(spawnable->y);
        eventStream.writeUInt32(spawnable->z);
        eventStream.writeUInt32(spawnable->orientation);
    }


    eventStream.writeUInt32(overworlds.size());
    for(auto overworld : overworlds){
        eventStream.writeUInt16(overworld->spriteID);
        eventStream.writeUInt16(overworld->overlayID);
        eventStream.writeUInt16(overworld->movementType);
        eventStream.writeUInt16(overworld->type);
        eventStream.writeUInt16(overworld->flag);
        eventStream.writeUInt16(overworld->script);
        eventStream.writeUInt16(overworld->orientation);
        eventStream.writeUInt16(overworld->range);
        eventStream.writeUInt16(overworld->unk1);
        eventStream.writeUInt16(overworld->unk2);
        eventStream.writeUInt16(overworld->xrange);
        eventStream.writeUInt16(overworld->yrange);
        eventStream.writeUInt16(overworld->x);
        eventStream.writeUInt16(overworld->y);
        eventStream.writeUInt32(overworld->z);
    }


    eventStream.writeUInt32(warps.size());
    for(auto warp : warps){
        eventStream.writeUInt16(warp->x);
        eventStream.writeUInt16(warp->y);
        eventStream.writeUInt16(warp->targetHeader);
        eventStream.writeUInt16(warp->anchor);
        eventStream.writeUInt32(warp->height);
    }


    eventStream.writeUInt32(triggers.size());
    for(auto trigger : triggers){
        eventStream.writeUInt16(trigger->scriptNum);
        eventStream.writeUInt16(trigger->x);
        eventStream.writeUInt16(trigger->y);
        eventStream.writeUInt16(trigger->w);
        eventStream.writeUInt16(trigger->h);
        eventStream.writeUInt16(trigger->z);
        eventStream.writeUInt16(trigger->expect);
        eventStream.writeUInt16(trigger->watch);
    }

    file->SetData(eventStream.getBuffer(), eventStream.getSize());
}