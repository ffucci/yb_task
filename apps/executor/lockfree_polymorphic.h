#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <array>

#include <iostream>

namespace yb::task {
class LockFreePolymorphic
{
   public:
    LockFreePolymorphic(uint8_t size) : data_(1 << size, std::byte{0x00})
    {
        write_ptr_ = data_.data();
    }

    template <typename T>
    auto Reserve() -> std::pair<size_t, void* const>
    {
        // write pointer increments and commit ptr

        auto total_size = sizeof(T);
        auto commit_ptr = write_ptr_.fetch_add(total_size, std::memory_order_acq_rel);
        auto curr_wr_seq = write_sequence_number_.fetch_add(1, std::memory_order_acq_rel);
        auto& node = nodes_[curr_wr_seq & (nodes_.size() - 1)];

        uint8_t expected = EMPTY;
        while (!node.written_.compare_exchange_weak(expected, RESERVED, std::memory_order_acq_rel)) {
            expected = EMPTY;
        }

        // Store the commit pointer in the node
        node.node_ptr_ = commit_ptr;

        return {curr_wr_seq, std::bit_cast<void* const>(commit_ptr)};
    }

    void Commit(const size_t sequence_number)
    {
        auto& current = nodes_[sequence_number & (nodes_.size() - 1)];
        uint8_t expected = RESERVED;
        while (!current.written_.compare_exchange_weak(expected, COMMIT, std::memory_order_acq_rel)) {
            expected = RESERVED;
        }
    }

    template <typename T>
    auto Read() -> T*
    {
        auto curr_rd_seq = read_sequence_number_.load(std::memory_order_acquire);
        auto& node = nodes_[curr_rd_seq & (nodes_.size() - 1)];

        auto result = COMMIT;
        auto desired = EMPTY;
        while (!node.written_.compare_exchange_weak(result, desired, std::memory_order_acq_rel)) {
            result = COMMIT;
        }
        read_sequence_number_.fetch_add(1, std::memory_order_acq_rel);

        return std::bit_cast<T*>(node.node_ptr_);
    }

   private:
    static constexpr uint8_t EMPTY{1 << 1};
    static constexpr uint8_t RESERVED{1 << 2};
    static constexpr uint8_t COMMIT{1 << 3};
    // (WRITE_PTR, READ_PTR)
    // write_ptr -> (seq, size, payload)
    alignas(64) std::atomic<std::byte*> write_ptr_;
    alignas(64) std::atomic_size_t write_sequence_number_{0};
    alignas(64) std::atomic_size_t read_sequence_number_{0};

    std::vector<std::byte> data_;
    struct Node
    {
        std::atomic<uint8_t> written_{EMPTY};
        std::byte* node_ptr_{};
    };

    static constexpr size_t NUM_NODES{1024};
    std::array<Node, NUM_NODES> nodes_{};
};
}  // namespace yb::task