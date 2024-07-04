#pragma once

#include <boost/container/static_vector.hpp>

#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stop_token>
#include <thread>
#include <utility>

#include "event.h"
#include "lockfree_polymorphic.h"
#include "stats_collector.h"
#include "task_utility.h"
#include "clock.h"

namespace yb::task {
class IEventProcessor
{
   public:
    IEventProcessor()
    {
        reader_ = std::jthread([this](std::stop_token token) {
            while (!token.stop_requested()) {
                auto [sequence, current] = polymorphic_.Read<Event>();
                current->Process();
                auto measure_timepoint = measure::Clock::now() - current->GetTimestamp();
                stats_collector_.collect(measure::Clock::from_cycles(measure_timepoint).count());
                polymorphic_.Free(sequence);
            }
            std::cout << "REQUESTED STOPPED..." << std::endl;
        });
    }

    ~IEventProcessor()
    {
        stats_collector_.print();
        reader_.request_stop();
        reader_.join();
        std::cout << "EVENT PROCESSOR DESTROYED" << std::endl;
    }

    // Single event
    struct ReservedEvent
    {
        ReservedEvent(void) = default;
        ReservedEvent(const size_t sequence_number, const void* event)
            : sequence_number_(sequence_number), event_(event)
        {
        }

        ReservedEvent(const ReservedEvent&) = delete;
        ReservedEvent& operator=(const ReservedEvent&) = delete;

        ReservedEvent(ReservedEvent&&) = delete;
        ReservedEvent& operator=(ReservedEvent&&) = delete;

        auto GetSequenceNumber(void) const noexcept -> size_t
        {
            return sequence_number_;
        }

        auto GetEvent(void) const noexcept -> const void*
        {
            return event_;
        }

        bool IsValid()
        {
            return event_ != nullptr;
        }

       private:
        const size_t sequence_number_{ULLONG_MAX};
        const void* event_{nullptr};
    };

    // Multiple Events
    struct ReservedEvents
    {
        ReservedEvents(const size_t sequence_number, void* const event, const size_t count, const size_t event_size)
            : sequence_number_(sequence_number), events_{nullptr}, count_(count), event_size_(event_size)
        {
        }

        // ReservedEvents(const ReservedEvents&) = delete;
        // ReservedEvent& operator=(const ReservedEvents&) = delete;

        // ReservedEvents(ReservedEvents&&) = delete;
        // ReservedEvents& operator=(ReservedEvents&&) = delete;

        auto GetSequenceNumber(void) const noexcept -> size_t
        {
            return sequence_number_;
        }

        auto GetEvent(const size_t index) const noexcept -> void* const
        {
            return std::bit_cast<void*>(std::bit_cast<std::byte*>(events_) + index * event_size_);
        }

        bool IsValid()
        {
            return events_ != nullptr;
        }

        auto Count() -> size_t
        {
            return count_;
        }

        template <class TEvent, template <class> class Constructor, class... Args>
        void Emplace(const size_t index, Args&&... args)
        {
            void* const event = GetEvent(index);
            if (event) Constructor<TEvent>::Construct(event, std::forward<Args>(args)...);
        }

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args&&... args)
        {
            void* const event = GetEvent(index);

            if (event) NewPlacementConstructor<TEvent>::Construct(event, std::forward<Args>(args)...);
        }

       private:
        const size_t sequence_number_;
        void* const events_{nullptr};
        const size_t count_;
        const size_t event_size_;
    };

    template <typename T>
    auto ReserveEvent() -> std::pair<size_t, void* const>
    {
        return polymorphic_.Reserve<T>();
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    template <class T, template <class> class Constructor, class... Args>
    ReservedEvent Reserve(Args&&... args)
    {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second) return ReservedEvent();

        Constructor<T>::Construct(reservation.second, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    template <class T, class... Args>
    ReservedEvent Reserve(Args&&... args)
    {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second) return ReservedEvent();

        NewPlacementConstructor<T>::Construct(reservation.second, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    template <typename T>
    boost::container::static_vector<ReservedEvents, 2> ReserveRange(const size_t size)
    {
        const auto [sequence_number, events, count] = polymorphic_.ReserveMultiple<T>(size);
        return {{sequence_number, events, count, sizeof(T)}, {0, nullptr, 0, 0}};
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    void Commit(const size_t sequence_number)
    {
        polymorphic_.Commit(sequence_number);
    }

    void Commit(const size_t sequence_number, const size_t count)
    {
        polymorphic_.Commit(sequence_number);
    }

    static constexpr uint8_t POW_SIZE{25};
    LockFreePolymorphic polymorphic_{POW_SIZE};

    StatsCollector<uint64_t> stats_collector_;
    std::jthread reader_;
};

}  // namespace yb::task