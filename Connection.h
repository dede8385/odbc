#ifndef __ODBC_CONNECTION_H__
#define __ODBC_CONNECTION_H__

#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include "PreparedStatement.h"

namespace ODBC
{

class Connection
{
public:
    Connection();
    ~Connection();
    
    void init(const std::string & dbDSName, 
              const std::string & userName, 
              const std::string & passWord);
    void close();
    
    bool connect();
    bool isClosed();
    
    PreparedStatement* prepareStatement(const char* strSql);
    
private:
    Connection(const Connection &);
    Connection & operator = (const Connection &);
    
private:
    SQLHENV hEnv_;
    SQLHDBC hDbc_;
    
    bool state_;
    
    std::string dbDSName_;
    std::string userName_;
    std::string passWord_;
};
    
} // namespace ODBC

#endif // __ODBC_CONNECTION_H__
