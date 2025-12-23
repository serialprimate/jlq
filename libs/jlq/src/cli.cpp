#include "jlq/cli.hpp"

#include "jlq/exit_codes.hpp"
#include "jlq/mapped_file.hpp"

#include <iostream>
#include <string>

namespace jlq
{

    namespace
    {

        void print_usage(std::ostream &os)
        {
            os << "Usage: jlq <file> [--help]\n";
        }

    } // namespace

    int run(std::span<const std::string_view> args)
    {
        // args includes argv[0]
        if (args.size() <= 1)
        {
            print_usage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        for (std::size_t i = 1; i < args.size(); ++i)
        {
            if (args[i] == "--help" || args[i] == "-h")
            {
                print_usage(std::cout);
                return static_cast<int>(ExitCode::Success);
            }
        }

        // Phase 1 MVP: only accept a single positional file argument.
        const std::string_view file = args[1];
        if (file.empty() || file.starts_with('-'))
        {
            print_usage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        if (args.size() != 2)
        {
            // Avoid silently accepting future flags until implemented.
            print_usage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        try
        {
            MappedFile::open_readonly(std::string(file));
        }
        catch (const std::exception &e)
        {
            std::cerr << "jlq: " << e.what() << "\n";
            return static_cast<int>(ExitCode::OsError);
        }

        return static_cast<int>(ExitCode::Success);
    }

} // namespace jlq
