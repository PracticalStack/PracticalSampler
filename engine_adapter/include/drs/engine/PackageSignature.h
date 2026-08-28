#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr std::size_t packageEd25519PublicKeyBytes = 32;
inline constexpr std::size_t packageEd25519PrivateKeyBytes = 64;
inline constexpr std::size_t packageEd25519SignatureBytes = 64;

bool generatePackageSigningKeyPair(std::vector<std::uint8_t>& publicKey,
                                   std::vector<std::uint8_t>& privateKey,
                                   std::string& issue);

bool packageSignEd25519(const std::vector<std::uint8_t>& privateKey,
                        const std::vector<std::uint8_t>& message,
                        std::vector<std::uint8_t>& signature,
                        std::string& issue);

bool packageVerifyEd25519(const std::vector<std::uint8_t>& publicKey,
                          const std::vector<std::uint8_t>& message,
                          const std::vector<std::uint8_t>& signature,
                          std::string& issue);

bool packageSignEd25519ph(const std::vector<std::uint8_t>& privateKey,
                          const std::vector<std::uint8_t>& message,
                          std::vector<std::uint8_t>& signature,
                          std::string& issue);

bool packageSignEd25519phFile(const std::vector<std::uint8_t>& privateKey,
                              const std::string& path,
                              std::uint64_t expectedBytes,
                              std::vector<std::uint8_t>& signature,
                              std::string& issue);

bool packageVerifyEd25519ph(const std::vector<std::uint8_t>& publicKey,
                            const std::vector<std::uint8_t>& message,
                            const std::vector<std::uint8_t>& signature,
                            std::string& issue);
} // namespace drs::engine
