#ifndef L0_TYPES_TYPES_H
#define L0_TYPES_TYPES_H

#include <llvm/IR/Attributes.h>

#include <memory>
#include <string>
#include <vector>

namespace l0
{

class Type
{
   public:
    virtual ~Type() = default;
    virtual std::string ToString() const = 0;

   protected:
    friend bool operator==(const Type& lhs, const Type& rhs);
    virtual bool Equals(const Type& other) const = 0;
};

class BooleanType : public Type
{
   public:
    std::string ToString() const override;

   protected:
    bool Equals(const Type& other) const override;
};

class IntegerType : public Type
{
   public:
    std::string ToString() const override;

   protected:
    bool Equals(const Type& other) const override;
};

class StringType : public Type
{
   public:
    std::string ToString() const override;

   protected:
    bool Equals(const Type& other) const override;
};

using ParameterList = std::vector<std::shared_ptr<Type>>;

class FunctionType : public Type
{
   public:
    std::string ToString() const override;

    std::unique_ptr<ParameterList> parameters = std::make_unique<ParameterList>();
    std::shared_ptr<Type> return_type;

   protected:
    bool Equals(const Type& other) const override;
};

}

#endif
