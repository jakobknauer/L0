#include "l0/ast/indent.h"

namespace l0::detail
{

Indent::Indent(std::ostream& stream, std::size_t tab_width)
    : streambuf_{stream.rdbuf()}, stream_{stream}, tab_width{tab_width}
{
    stream_.rdbuf(this);
}

Indent::~Indent() { stream_.rdbuf(streambuf_); }

Indent& Indent::operator++()
{
    ++indentation_level_;
    return *this;
}

Indent& Indent::operator--()
{
    --indentation_level_;
    return *this;
}

int Indent::overflow(int ch)
{
    if (is_at_start_of_line_ && ch != '\n')
    {
        std::string indent = GetIndent();
        streambuf_->sputn(indent.data(), indent.size());
    }
    is_at_start_of_line_ = ch == '\n';
    return streambuf_->sputc(ch);
}

std::string Indent::GetIndent() const { return std::string(tab_width * indentation_level_, ' '); }

}  // namespace l0::detail
