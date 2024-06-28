#pragma once

#include <bit>
#include <cstddef>
#include <cstdlib>
#include <utility>
#include <vector>

namespace yb::task {

class IEventProcessor
{
   public:
    // Single event
    struct ReservedEvent
    {
        ReservedEvent(void);
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

       private:
        const size_t sequence_number_;
        const void* event_;
    };

    // Multiple Events
    struct ReservedEvents
    {
        ReservedEvents(void);
        ReservedEvents(const size_t sequence_number, void* const event, const size_t count, const size_t event_size)
            : sequence_number_(sequence_number), events_{nullptr}, count_(count), event_size_(event_size)
        {
        }

        ReservedEvents(const ReservedEvents&) = delete;
        ReservedEvent& operator=(const ReservedEvents&) = delete;

        ReservedEvents(ReservedEvents&&) = delete;
        ReservedEvents& operator=(ReservedEvents&&) = delete;

        auto GetSequenceNumber(void) const noexcept -> size_t
        {
            return sequence_number_;
        }

        auto GetEvent(const size_t index) const noexcept -> void* const
        {
            return std::bit_cast<void*>(std::bit_cast<std::byte*>(events_) + index * event_size_);
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
        // template <class TEvent, class... Args>
        // void Emplace(const size_t index, Args&&... args)
        // {
        //     void* const event = GetEvent(index);

        //     if (event) NewPlacementConstructor<TEvent>::Construct(event, std::forward<Args>(args)...);
        // }

       private:
        const size_t sequence_number_;
        void* const events_{nullptr};
        const size_t count_;
        const size_t event_size_;
    };

    auto ReserveEvent() -> std::pair<size_t, void* const>
    {
        return {0, nullptr};
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    template <class T, template <class> class Constructor, class... Args>
    ReservedEvent Reserve(Args&&... args)
    {
        const auto reservation = ReserveEvent();
        if (!reservation.second) return ReservedEvent();

        Constructor<T>::Construct(reservation.second, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    // template <class T, class... Args>
    // ReservedEvent Reserve(Args&&... args)
    // {
    //     const auto reservation = ReserveEvent();
    //     if (!reservation.second) return ReservedEvent();

    //     NewPlacementConstructor<T>::Construct(reservation.second, std::forward<Args>(args)...);
    //     return ReservedEvent(reservation.first, reservation.second);
    // }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    std::vector<ReservedEvents> ReserveRange(const size_t size)
    {
        return {};
    }

    //////////////////////////////////////////////////////////////////////////
    /// ---
    void Commit(const size_t sequence_number)
    {
    }
};

}  // namespace yb::task