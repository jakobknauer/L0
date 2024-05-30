#ifndef L0_SEMANTICS_SEMANTIC_ERROR
#define L0_SEMANTICS_SEMANTIC_ERROR

#include <string>

namespace l0
{

class SemanticError
{
   public:
    SemanticError(const std::string& message);
    std::string GetMessage() const;

   private:
    const std::string message_;
};

}  // namespace l0

#endif
