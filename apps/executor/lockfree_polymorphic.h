#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>

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
        auto total_size = sizeof(T);
        auto commit_ptr = std::bit_cast<uintptr_t>(write_ptr_.fetch_add(total_size, std::memory_order_acq_rel));
        auto curr_wr_seq = write_sequence_number_.fetch_add(1, std::memory_order_acq_rel);
        auto& node = nodes_[curr_wr_seq & (NUM_NODES - 1)];

        auto expected = EMPTY;
        do {
            expected = EMPTY;
        } while (!node.written_.compare_exchange_weak(expected, RESERVED, std::memory_order_acq_rel));

        // Store the commit pointer in the node
        node.node_ptr_ = commit_ptr;
        return {curr_wr_seq, std::bit_cast<void* const>(commit_ptr)};
    }

    void Commit(const size_t sequence_number)
    {
        auto& current = nodes_[sequence_number & (NUM_NODES - 1)];
        auto expected = RESERVED;

        do {
            expected = RESERVED;
        } while (!current.written_.compare_exchange_weak(expected, COMMIT, std::memory_order_acq_rel));
    }

    template <typename T>
    auto Read() -> std::pair<size_t, T*>
    {
        auto curr_rd_seq = read_sequence_number_.fetch_add(1, std::memory_order_acq_rel);
        auto& node = nodes_[curr_rd_seq & (NUM_NODES - 1)];
        auto result = COMMIT;
        do {
            result = COMMIT;
        } while (!node.written_.compare_exchange_weak(result, READ, std::memory_order_acq_rel));

        return {curr_rd_seq, std::bit_cast<T*>(node.node_ptr_)};
    }

    auto Free(const size_t sequence_number)
    {
        auto& node = nodes_[sequence_number & (NUM_NODES - 1)];

        node.written_.store(EMPTY, std::memory_order_release);
    }

   private:
    static constexpr uint64_t EMPTY{1 << 1};
    static constexpr uint64_t RESERVED{1 << 2};
    static constexpr uint64_t COMMIT{1 << 3};
    static constexpr uint64_t READ{1 << 4};

    static constexpr size_t NUM_NODES{1 << 14};
    static constexpr size_t CACHELINE_SIZE{64};

    // (WRITE_PTR, READ_PTR)
    alignas(CACHELINE_SIZE) std::atomic<std::byte*> write_ptr_;
    alignas(CACHELINE_SIZE) std::atomic_size_t write_sequence_number_{0};
    alignas(CACHELINE_SIZE) std::atomic_size_t read_sequence_number_{0};

    std::vector<std::byte> data_;
    struct Node
    {
        std::atomic<uint64_t> written_{EMPTY};
        uintptr_t node_ptr_{};
    };

    std::array<Node, NUM_NODES> nodes_{};
};
}  // namespace yb::task