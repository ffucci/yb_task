#pragma once

namespace yb::task {
class IEvent
{
   public:
    virtual ~IEvent() = default;
    virtual void Process(void) = 0;
};
}  // namespace yb::task
