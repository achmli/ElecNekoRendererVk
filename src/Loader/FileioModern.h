#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ElecNeko
{
    bool InitializeFileSystem();
    std::string JoinPath(const std::filesystem::path &a, const std::filesystem::path &b);

    std::optional<std::vector<uint8_t>> ReadFileBinary(const std::filesystem::path &p);
    bool WriteFileBinary(const std::filesystem::path &p, const void *data, size_t size);

    std::optional<std::string> ReadFileText(const std::filesystem::path &p);
    bool WriteFileText(const std::filesystem::path &p, const std::string &text);

    std::optional<std::vector<uint32_t>> ReadFileBinary32(const std::filesystem::path &p);

    bool FileExists(const std::filesystem::path &path);
    std::string WeaklyCanonicalPath(const std::filesystem::path &path);
    uint64_t LastWriteTime(const std::filesystem::path &path);
} // namespace ElecNeko
