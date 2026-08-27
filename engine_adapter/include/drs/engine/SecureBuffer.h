#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace drs::engine
{
// Move-only ownership for transient keys/plaintext. Clearing is best effort:
// it prevents ordinary allocator reuse from retaining bytes, but is not a
// claim that a desktop process is a hardware-backed secret boundary.
class SecureBuffer
{
public:
    SecureBuffer() = default;
    explicit SecureBuffer(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;
    SecureBuffer(SecureBuffer&& other) noexcept : bytes_(std::move(other.bytes_)) { other.clear(); }
    SecureBuffer& operator=(SecureBuffer&& other) noexcept
    {
        if (this != &other) { clear(); bytes_ = std::move(other.bytes_); other.clear(); }
        return *this;
    }
    ~SecureBuffer() { clear(); }

    std::uint8_t* data() noexcept { return bytes_.data(); }
    const std::uint8_t* data() const noexcept { return bytes_.data(); }
    std::size_t size() const noexcept { return bytes_.size(); }
    bool empty() const noexcept { return bytes_.empty(); }
    void clear() noexcept
    {
        volatile std::uint8_t* p = bytes_.data();
        for (std::size_t i = 0; i < bytes_.size(); ++i) p[i] = 0;
        bytes_.clear();
        bytes_.shrink_to_fit();
    }
private:
    std::vector<std::uint8_t> bytes_;
};
} // namespace drs::engine
