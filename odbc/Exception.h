#ifndef __ODBC_EXCEPTION_H__
#define __ODBC_EXCEPTION_H__

#include <string>
#include <exception>

namespace ODBC
{

class Exception : public std::exception
{
public:
    explicit Exception(const char* what);
    explicit Exception(const std::string& what);
    virtual ~Exception() throw();
    virtual const char* what() const throw();
    const char* stackTrace() const throw();

private:
    void fillStackTrace();

private:
    std::string message_;
    std::string stack_;
};

} // namespace ODBC

#endif // __ODBC_EXCEPTION_H__
