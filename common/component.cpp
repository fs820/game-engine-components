//-------------------------------------
//
//　コンポーネントの基底 [component.cpp]
// Author: Fuma Sato
//
//-------------------------------------
#include "component.h"
std::atomic<uint64_t> Component::s_nextId{ 0 };
