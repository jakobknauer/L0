#include "l0/generation/generator_error.h"

GeneratorError::GeneratorError(std::string message)
    : message_{message}
{
}

std::string GeneratorError::GetMessage() const
{
    return message_;
}

const char* GeneratorError::what() const
{
    return message_.c_str();
}
