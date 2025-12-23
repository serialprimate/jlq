#pragma once

#include "test_harness.hpp"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <unistd.h>

namespace jlq::test
{

    class TempFile
    {
    public:
        TempFile(std::string_view prefix, std::string_view suffix)
        {
            const auto dir = std::filesystem::temp_directory_path();
            std::string tmpl = (dir / (std::string(prefix) + "XXXXXX" + std::string(suffix))).string();

            // mkstemp requires the Xs to be at the end of the template.
            // We implement suffix support by creating without suffix and then renaming.
            if (!suffix.empty())
            {
                const auto dir_prefix = (dir / (std::string(prefix) + "XXXXXX")).string();
                tmpl = dir_prefix;
            }

            std::string buf = tmpl;
            buf.push_back('\0');

            const int fd = ::mkstemp(buf.data());
            if (fd == -1)
            {
                throw std::runtime_error(std::string("mkstemp: ") + std::strerror(errno));
            }
            ::close(fd);

            std::filesystem::path created = std::string(buf.c_str());
            if (!suffix.empty())
            {
                std::filesystem::path renamed = created;
                renamed += std::string(suffix);
                std::error_code ec;
                std::filesystem::rename(created, renamed, ec);
                if (ec)
                {
                    std::filesystem::remove(created);
                    throw std::runtime_error(std::string("rename temp file: ") + ec.message());
                }
                path_ = std::move(renamed);
            }
            else
            {
                path_ = std::move(created);
            }
        }

        TempFile(const TempFile &) = delete;
        TempFile &operator=(const TempFile &) = delete;

        TempFile(TempFile &&) = delete;
        TempFile &operator=(TempFile &&) = delete;

        ~TempFile()
        {
            if (!path_.empty())
            {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
            }
        }

        [[nodiscard]] const std::filesystem::path &path() const noexcept { return path_; }

        void write_all(std::string_view contents) const
        {
            std::ofstream out(path_, std::ios::binary | std::ios::trunc);
            JLQ_CHECK(out.good());
            out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            JLQ_CHECK(out.good());
        }

    private:
        std::filesystem::path path_;
    };

} // namespace jlq::test
