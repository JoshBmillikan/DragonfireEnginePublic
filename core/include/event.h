//
// Created by josh on 11/27/22.
//

#pragma once
#include <ankerl/unordered_dense.h>

namespace df {

class Event {
    enum class Type {
        button,
    }k;
};

class EventBuss {
    ankerl::unordered_dense::set<Event> events;
};

}   // namespace df
