#ifndef L0_GENERATION_GENERATION_ERROR_H
#define L0_GENERATION_GENERATION_ERROR_H

#include <string>

class GeneratorError
{
   public:
    GeneratorError(std::string message);

    std::string GetMessage() const;

    const char* what() const;

   private:
    const std::string message_;
};

#endif
