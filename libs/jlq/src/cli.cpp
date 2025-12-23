#include "jlq/cli.hpp"

#include "exit_codes.hpp"
#include "MappedFile.hpp"

#include <iostream>
#include <string>

namespace jlq
{

    namespace
    {

        void printUsage(std::ostream &os)
        {
            os << "Usage: jlq <file> [--help]\n";
        }

    } // namespace

    int run(std::span<const std::string_view> args)
    {
        // args includes argv[0]
        if (args.size() <= 1)
        {
            printUsage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        for (std::size_t i = 1; i < args.size(); ++i)
        {
            if (args[i] == "--help")
            {
                printUsage(std::cout);
                return static_cast<int>(ExitCode::Success);
            }
        }

        // Phase 1 MVP: only accept a single positional file argument.
        const std::string_view file = args[1];
        if (file.empty() || file.starts_with('-'))
        {
            printUsage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        if (args.size() != 2)
        {
            // Avoid silently accepting future flags until implemented.
            printUsage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        try
        {
            MappedFile::openReadonly(std::string(file));
        }
        catch (const std::exception &e)
        {
            std::cerr << "jlq: " << e.what() << "\n";
            return static_cast<int>(ExitCode::OsError);
        }

        return static_cast<int>(ExitCode::Success);
    }

} // namespace jlq
