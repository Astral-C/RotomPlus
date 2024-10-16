#include "IDManager.hpp"

uint32_t mCurrentID = 1;

uint32_t GetID(){
    return mCurrentID++;
}