#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace drs::engine
{
// Move-only ownership for transient keys/plaintext. Storage is overwritten
// with sodium_memzero before release. This is not a claim that a desktop
// process is a locked-memory or hardware-backed secret boundary.
class SecureBuffer
{
public:
    SecureBuffer() = default;
    explicit SecureBuffer(std::vector<std::uint8_t> bytes);
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;
    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;
    ~SecureBuffer();

    std::uint8_t* data() noexcept { return bytes_.data(); }
    const std::uint8_t* data() const noexcept { return bytes_.data(); }
    std::size_t size() const noexcept { return bytes_.size(); }
    bool empty() const noexcept { return bytes_.empty(); }
    const std::vector<std::uint8_t>& bytes() const noexcept { return bytes_; }
    void clear() noexcept;
private:
    std::vector<std::uint8_t> bytes_;
};
} // namespace drs::engine
