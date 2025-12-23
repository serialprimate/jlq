#include "MappedFile.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace jlq
{

    namespace
    {

        [[noreturn]] void throwErrno(const char *what, int err)
        {
            throw std::system_error(std::error_code(err, std::generic_category()), what);
        }

    } // namespace

    MappedFile::MappedFile(int fd, void *mapping, std::size_t size) noexcept
        : fd_{fd}, mapping_{mapping}, size_{size} {}

    MappedFile::MappedFile(MappedFile &&other) noexcept
    {
        *this = std::move(other);
    }

    MappedFile &MappedFile::operator=(MappedFile &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        reset();
        fd_ = other.fd_;
        mapping_ = other.mapping_;
        size_ = other.size_;
        other.fd_ = -1;
        other.mapping_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    MappedFile::~MappedFile() { reset(); }

    void MappedFile::reset() noexcept
    {
        if (mapping_ != nullptr)
        {
            ::munmap(mapping_, size_);
            mapping_ = nullptr;
        }
        if (fd_ != -1)
        {
            ::close(fd_);
            fd_ = -1;
        }
        size_ = 0;
    }

    MappedFile MappedFile::openReadonly(const std::string &path)
    {
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd == -1)
        {
            throwErrno("open", errno);
        }

        struct stat st
        {
        };
        if (::fstat(fd, &st) != 0)
        {
            const int err = errno;
            ::close(fd);
            throwErrno("fstat", err);
        }

        if (st.st_size < 0)
        {
            ::close(fd);
            throw std::runtime_error("fstat returned negative size");
        }

        const std::size_t size = static_cast<std::size_t>(st.st_size);
        if (size == 0)
        {
            return MappedFile{fd, nullptr, 0};
        }

        void *mapping = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapping == MAP_FAILED)
        {
            const int err = errno;
            ::close(fd);
            throwErrno("mmap", err);
        }

        return MappedFile{fd, mapping, size};
    }

    std::span<const std::byte> MappedFile::bytes() const noexcept
    {
        if (mapping_ == nullptr || size_ == 0)
        {
            return {};
        }
        return {static_cast<const std::byte *>(mapping_), size_};
    }

    std::size_t MappedFile::size() const noexcept { return size_; }

    bool MappedFile::empty() const noexcept { return size_ == 0; }

} // namespace jlq
