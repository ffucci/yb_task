#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "ievent.h"

namespace yb::task {
class Event : public IEvent
{
   public:
    explicit Event(const uint64_t timestamp, const int value) : timestamp_{timestamp}, value_(value)
    {
    }
    virtual ~Event(void) override = default;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&&) = delete;
    Event& operator=(Event&&) = delete;

    virtual void Process(void) override
    {
        // do something with value_
    }

    auto GetTimestamp() -> uint64_t
    {
        return timestamp_;
    }

   private:
    uint64_t timestamp_;
    int value_;
};
}  // namespace yb::task