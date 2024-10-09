#ifndef __EVENT_H__
#define __EVENT_H__

#include <vector>
#include <memory>
#include <NDS/System/FileSystem.hpp>

enum class EventType {
    Spawnable,
    Overworld,
    Warp,
    Trigger
};

class Event {
public:
    EventType eventType;
};

// this one seemed off in the docs... weird!
class Spawnable : public Event {
public:
    uint16_t scriptNum;
    uint16_t type; // enum?
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t orientation;
};

class Overworld : public Event {
public:
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
    
    uint16_t x;
    uint16_t y;
    uint32_t z;
};

class Warp : public Event {
public:
    uint16_t x;
    uint16_t y;
    
    uint16_t targetHeader;
    uint16_t anchor;
    uint32_t height;
};

class Trigger : public Event {   
public:
    uint16_t scriptNum;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t z;
    uint16_t expect;
    uint16_t watch;
};

std::vector<std::shared_ptr<Event>> LoadEvents(std::shared_ptr<Palkia::Nitro::File>);
void SaveEvents(std::shared_ptr<Palkia::Nitro::File>, std::vector<std::shared_ptr<Event>>);

#endif