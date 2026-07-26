#pragma once

#include <git2.h>

#include <stdexcept>
#include <string>

namespace gitscope::git {

class GitException : public std::runtime_error {
public:
    explicit GitException(std::string message) : std::runtime_error(std::move(message)) {}
};

// Converts a negative libgit2 return code into a GitException carrying the
// library's last error message. Returns the code unchanged when >= 0 so it
// can wrap calls whose success value matters.
inline int check(int code, const char* context)
{
    if (code >= 0)
        return code;
    const git_error* err = git_error_last();
    std::string message(context);
    message += ": ";
    if (err != nullptr && err->message != nullptr)
        message += err->message;
    else
        message += "libgit2 error " + std::to_string(code);
    throw GitException(message);
}

} // namespace gitscope::git
