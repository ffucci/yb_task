#include <cstddef>
#include <iostream>
#include <algorithm>
#include "ieventprocessor.h"
#include "event.h"

using namespace yb::task;

int main()
{
    auto event_processor = std::make_unique<IEventProcessor>();

    // queue 1 event
    constexpr size_t NUM_EVENTS{1000000};
    size_t idx{0};
    while (idx < NUM_EVENTS) {
        auto reserved_event = event_processor->Reserve<Event>(static_cast<int>(idx));
        if (!reserved_event.IsValid()) {
            // ERROR: Reserve() failed ...
            std::cout << "Cannot reserve event..." << std::endl;
        } else {
            event_processor->Commit(reserved_event.GetSequenceNumber());
        }
        ++idx;
    }

    // queue multiple events
    // auto reserved_events_collection = event_processor->ReserveRange(
    //     2);  // It can reserve less items than requested! You should always check how many events have been reserved!
    // if (reserved_events_collection.empty()) {
    //     std::cout << "EVENTS EMPTY" << std::endl;
    // } else {
    //     std::for_each(
    //         reserved_events_collection.begin(), reserved_events_collection.end(),
    //         [&](IEventProcessor::ReservedEvents& reserved_events) {
    //             if (!reserved_events.IsValid()) {
    //                 // ERROR: Reserve() failed
    //             } else {
    //                 for (size_t i = 0; i < reserved_events.Count(); ++i)
    //                     reserved_events.Emplace<Event>(i, static_cast<int>(i + 3));
    //                 event_processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
    //             }
    //         });
    // }

    std::cout << "Published events: " << idx << std::endl;

    while (true) {
    }

    return 0;
}