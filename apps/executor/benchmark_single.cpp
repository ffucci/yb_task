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
    constexpr size_t NUM_EVENTS{1000};
    size_t number_writers = std::stol(argv[1]);
    std::cout << "NUMBER WRITERS: " << number_writers << std::endl;
    std::cout << "NUMBER EVENTS: " << NUM_EVENTS << std::endl;

    std::vector<std::jthread> threads(number_writers);
    for (size_t tidx = 0; tidx < threads.size(); ++tidx) {
        threads[tidx] = std::jthread([&event_processor, tidx](std::stop_token token) {
            size_t idx{0};
            while (idx < NUM_EVENTS) {
                auto time = Clock::now();
                auto reserved_event =
                    event_processor->Reserve<Event>(time, static_cast<int>((tidx * NUM_EVENTS) + idx));
                if (!reserved_event.IsValid()) {
                    // ERROR: Reserve() failed ...
                    std::cout << "Cannot reserve event..." << std::endl;
                } else {
                    event_processor->Commit(reserved_event.GetSequenceNumber());
                    std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                    ++idx;
                }
            }
        });
    }

    return 0;
}