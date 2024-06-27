#ifndef L0_AST_INDENT_H
#define L0_AST_INDENT_H

#include <ostream>
#include <streambuf>

namespace l0::detail
{

// Inspired by https://stackoverflow.com/questions/9599807/how-to-add-indention-to-the-stream-operator
class Indent : public std::streambuf
{
   public:
    Indent(std::ostream& stream, std::size_t tab_width = 4);
    ~Indent();

    Indent& operator++();
    Indent& operator--();

   protected:
    virtual int overflow(int ch);

   private:
    std::string GetIndent() const;

    std::streambuf* streambuf_;
    std::ostream& stream_;
    bool is_at_start_of_line_{true};
    std::size_t tab_width;
    std::size_t indentation_level_{0};
};

}  // namespace l0::detail

#endif
