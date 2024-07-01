#pragma once

#include <iostream>

#include "ievent.h"

namespace yb::task {
class Event : public IEvent
{
   public:
    explicit Event(const int value) : value_(value)
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
        std::cout << value_ * value_ << std::endl;
    }

   private:
    int value_;
};
}  // namespace yb::task