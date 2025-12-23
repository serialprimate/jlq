#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace jlq
{

    class MappedFile
    {
    public:
        MappedFile() = default;

        MappedFile(const MappedFile &) = delete;
        MappedFile &operator=(const MappedFile &) = delete;

        MappedFile(MappedFile &&other) noexcept;
        MappedFile &operator=(MappedFile &&other) noexcept;

        ~MappedFile();

        static MappedFile openReadonly(const std::string &path);

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        explicit MappedFile(int fd, void *mapping, std::size_t size) noexcept;

        void reset() noexcept;

        int fd_{-1};
        void *mapping_{nullptr};
        std::size_t size_{0};
    };

} // namespace jlq
