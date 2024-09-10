#include "l0/semantics/semantic_error.h"

namespace l0
{

SemanticError::SemanticError(const std::string& message)
    : message_{message}
{
}

std::string SemanticError::GetMessage() const
{
    return message_;
}

}  // namespace l0
