#include <drs/engine/SecureBuffer.h>

#include <sodium/utils.h>

namespace drs::engine
{
SecureBuffer::SecureBuffer(std::vector<std::uint8_t> bytes)
    : bytes_(std::move(bytes))
{
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : bytes_(std::move(other.bytes_))
{
    other.clear();
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept
{
    if (this != &other)
    {
        clear();
        bytes_ = std::move(other.bytes_);
        other.clear();
    }
    return *this;
}

SecureBuffer::~SecureBuffer()
{
    clear();
}

void SecureBuffer::clear() noexcept
{
    if (! bytes_.empty())
        sodium_memzero(bytes_.data(), bytes_.size());
    std::vector<std::uint8_t>().swap(bytes_);
}
} // namespace drs::engine
