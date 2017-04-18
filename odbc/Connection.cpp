#include <odbcwapper/Connection.h>

namespace ODBC
{

Connection::Connection() 
        : hEnv_(NULL), 
          hDbc_(NULL), 
          state_(false), 
          dbDSName_(""), 
          userName_(""), 
          passWord_("")
{
}

Connection::~Connection()
{
    close();
}

void Connection::init(const std::string & dbDSName, 
                      const std::string & userName, 
                      const std::string & passWord)
{
    state_  = false;
    dbDSName_ = dbDSName;
    userName_ = userName;
    passWord_ = passWord;
    
    SQLRETURN retCode;
    retCode = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        // std::cout << "Erro AllocHandle: " << retCode << std::endl;
        return;
    }
    
    retCode = SQLSetEnvAttr(hEnv_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        // std::cout << "Erro AllocHandle: " << retCode << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
        return;
    }
    
    retCode = SQLAllocHandle(SQL_HANDLE_DBC, hEnv_, &hDbc_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        // std::cout << "Erro AllocHandle: " << retCode << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
        return;
    }
}

bool Connection::connect()
{
    if (state_ == false)
    {
        SQLRETURN retCode;
        retCode = SQLConnect(hDbc_, 
                            (SQLCHAR* )dbDSName_.c_str(), SQL_NTS, 
                            (SQLCHAR* )userName_.c_str(), SQL_NTS, 
                            (SQLCHAR* )passWord_.c_str(), SQL_NTS);
        if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
        {
            //std::cout << "connect fail: " << retCode << std::endl;
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc_);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
            return false;
        }
    }
    
    state_ = true;
    // std::cout << "connect success!" << std::endl;
    return true;
}

bool Connection::isClosed()
{
    return (state_ == false);
}

void Connection::close()
{
    if (hDbc_)
    {
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc_);
        hDbc_ = NULL;
    }
    
    if (hEnv_)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
        hDbc_ = NULL;
    }
}

PreparedStatement* Connection::prepareStatement(const char* strSql)
{
    return new PreparedStatement(hDbc_, strSql);
}

} // namespace ODBC
