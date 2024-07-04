#include <chrono>
#include <cstddef>
#include <iostream>
#include <algorithm>
#include <stop_token>
#include <string>
#include <thread>
#include "ieventprocessor.h"
#include "event.h"
#include "clock.h"

using namespace yb::task;
using namespace yb::measure;
int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cout << "Application takes 1 param = number of writers." << std::endl;
        return 0;
    }

    Clock::init();

    auto event_processor = std::make_unique<IEventProcessor>();

    // queue 1 event
    constexpr size_t NUM_EVENTS{5000};
    size_t number_writers = std::stol(argv[1]);
    std::cout << "NUMBER WRITERS: " << number_writers << std::endl;
    std::cout << "NUMBER EVENTS: " << NUM_EVENTS << std::endl;

    std::vector<std::jthread> threads(number_writers);
    for (size_t tidx = 0; tidx < threads.size(); ++tidx) {
        threads[tidx] = std::jthread([&event_processor, tidx](std::stop_token token) {
            size_t idx{0};
            // queue multiple events
            auto reserved_events_collection =
                event_processor->ReserveRange<Event>(2);  // It can reserve less items than requested! You should always
                                                          // check how many events have been reserved!
            if (reserved_events_collection.empty()) {
                std::cout << "EVENTS EMPTY" << std::endl;
            } else {
                std::for_each(
                    reserved_events_collection.begin(), reserved_events_collection.end(),
                    [&](IEventProcessor::ReservedEvents& reserved_events) {
                        if (!reserved_events.IsValid()) {
                            // ERROR: Reserve() failed
                        } else {
                            for (size_t i = 0; i < reserved_events.Count(); ++i)
                                reserved_events.Emplace<Event>(i, i, static_cast<int>(i + 3));
                            event_processor->Commit(reserved_events.GetSequenceNumber(), reserved_events.Count());
                        }
                    });
            }

            std::cout << "JOINED " << tidx << std::endl;
        });
    }

    return 0;
}