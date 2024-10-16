#ifndef __EVENT_H__
#define __EVENT_H__

#include <vector>
#include <memory>
#include <NDS/System/FileSystem.hpp>
#include "IDManager.hpp"

enum class EventType {
    Spawnable,
    Overworld,
    Warp,
    Trigger
};

struct Event {
    EventType eventType;   
};

// this one seemed off in the docs... weird!
struct Spawnable : Event {
    uint32_t id;
    uint16_t scriptNum;
    uint16_t type; // enum?
    int16_t x;
    uint16_t unk1;
    int16_t y;
    int32_t z;
    uint16_t unk2;
    uint16_t orientation;
    uint16_t unk3;
};

struct Overworld : Event {
    uint32_t id;
    uint16_t spriteID;
    uint16_t overlayID;
    uint16_t movementType;
    uint16_t type;
    uint16_t flag;
    uint16_t script;
    uint16_t orientation;
    uint16_t range;
    uint16_t unk1;
    uint16_t unk2;
    
    uint16_t xrange;
    uint16_t yrange;
    
    int16_t x;
    int16_t y;
    int32_t z;
};

struct Warp : Event {
    uint32_t id;
    int16_t x;
    int16_t y;
    
    uint16_t targetHeader;
    uint16_t anchor;
    uint32_t height;
};

struct Trigger : Event {
    uint32_t id;   
    uint16_t scriptNum;
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    int16_t z;
    uint16_t expect;
    uint16_t watch;
};

struct EventData {
    std::vector<Spawnable> spawnEvents;
    std::vector<Overworld> overworldEvents;
    std::vector<Warp> warpEvents;
    std::vector<Trigger> triggerEvents;
};

EventData LoadEvents(std::shared_ptr<Palkia::Nitro::File>);
void SaveEvents(std::shared_ptr<Palkia::Nitro::File>, EventData);

void LoadEventModel(std::shared_ptr<Palkia::Nitro::File>);
void RenderEvent(glm::mat4, uint32_t);

#endif