#include "FileioModern.h"

#include <fstream>
#include <iostream>

namespace ElecNeko
{
    bool InitializeFileSystem()
    {
        try
        {
            std::filesystem::path p = std::filesystem::current_path();
            (void) p;
        } catch (const std::exception &e)
        {
            std::cerr << "Failed to initialize filesystem: " << e.what() << "\n";
            return false;
        }
        return true;
    }

    std::string JoinPath(const std::filesystem::path &a, const std::filesystem::path &b)
    {
        if (b.is_absolute())
        {
            return b.string();
        }
        if (a.empty())
        {
            return b.string();
        }
        if (a.has_filename())
        {
            return (a.parent_path() / b).lexically_normal().string();
        }
        return (a / b).lexically_normal().string();
    }

    std::optional<std::vector<uint8_t>> ReadFileBinary(const std::filesystem::path &p)
    {
        std::ifstream ifs(p, std::ios::binary | std::ios::ate);
        if (!ifs)
        {
            return std::nullopt;
        }
        auto sz = ifs.tellg();
        if (sz <= 0)
        {
            return std::vector<uint8_t>{};
        }
        std::vector<uint8_t> data(static_cast<size_t>(sz));
        ifs.seekg(0, std::ios::beg);
        ifs.read(reinterpret_cast<char *>(data.data()), data.size());
        if (!ifs)
        {
            return std::nullopt;
        }
        return data;
    }

    bool WriteFileBinary(const std::filesystem::path &p, const void *data, size_t size)
    {
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path()))
        {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream ofs(p, std::ios::binary | std::ios::out);
        if (!ofs)
        {
            return false;
        }
        ofs.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));

        return !!ofs;
    }

    std::optional<std::string> ReadFileText(const std::filesystem::path &p)
    {
        std::ifstream ifs(p);
        if (!ifs)
        {
            return std::nullopt;
        }
        std::string contents;
        ifs.seekg(0, std::ios::end);
        std::streampos pos = ifs.tellg();
        if (pos > 0)
        {
            contents.resize(static_cast<size_t>(pos));
            ifs.seekg(0, std::ios::beg);
        }
        // std::string line;
        // while (std::getline(ifs, line))
        // {
        //     contents.append(line);
        //     contents.push_back('\n');
        // }
        ifs.read(contents.data(), pos);
        auto got = ifs.gcount();
        contents.resize(static_cast<size_t>(got));
        return contents;
    }

    bool WriteFileText(const std::filesystem::path &p, const std::string &text)
    {
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path()))
        {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream ofs(p, std::ios::out | std::ios::binary);
        if (!ofs)
        {
            return false;
        }
        ofs.write(text.data(), static_cast<std::streamsize>(text.size()));
        return !!ofs;
    }

    std::optional<std::vector<uint32_t>> ReadFileBinary32(const std::filesystem::path &p)
    {
        std::ifstream ifs(p, std::ios::binary | std::ios::ate);
        if (!ifs)
        {
            return std::nullopt;
        }
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size <= 0 || (size % 4) != 0)
        {
            return std::nullopt;
        }
        std::vector<uint32_t> data(static_cast<size_t>(size / 4));
        ifs.read(reinterpret_cast<char *>(data.data()), size);
        if (!ifs)
        {
            return std::nullopt;
        }
        return data;
    }

    bool FileExists(const std::filesystem::path &path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    std::string WeaklyCanonicalPath(const std::filesystem::path &path)
    {
        std::error_code ec;
        return std::filesystem::weakly_canonical(path, ec).string();
    }

    uint64_t LastWriteTime(const std::filesystem::path &path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            return 0;
        }
        auto tp = std::filesystem::last_write_time(path, ec);
        if (ec)
        {
            return 0;
        }

        return static_cast<uint64_t>(tp.time_since_epoch().count());
    }

} // namespace ElecNeko
