#ifndef __ODBC_PREPARED_STATEMENT_H__
#define __ODBC_PREPARED_STATEMENT_H__

#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <odbcwapper/ResultSet.h>

namespace ODBC
{

// 绑定参数信息结构
struct BIND_PARAM_INFO
{
    SQLCHAR*     paramValue; // 参数缓存地址
    SQLLEN*      indArr;     // IND数组指针
    SQLSMALLINT  sql_c_type; // C数据类型标识
    SQLSMALLINT  sql_type;   // SQL数据类型标识
    SQLLEN       columnSize; // 对应字段的实际长度
    SQLLEN       columnMax;  // 对应字段的最大长度
    unsigned int memSize;    // 申请的内存大小

    BIND_PARAM_INFO()
    {
        memset(this, 0, sizeof(BIND_PARAM_INFO));
    }
};

class PreparedStatement
{
public:
    PreparedStatement(unsigned int paramSetSize = 1);
    PreparedStatement(SQLHSTMT hStmt, const std::string & strSql, unsigned int paramSetSize = 1);
    ~PreparedStatement();
    
    ResultSet* executeQuery();
    int executeUpdate();
    
    void setInt(int index, int value);
    // void setTinyInt(int index, unsigned char value);
    void setSmallInt(int index, short int value);
    void setString(int index, std::string & value);
    
private:
    PreparedStatement(const PreparedStatement &);
    PreparedStatement & operator = (const PreparedStatement &);
    
    void resizeParamTable(int maxSize);
    void setParamAttr(int index, SQLSMALLINT sqlType, int maxSize);
    void handleError(const char* FILE = __FILE__, int LINE = __LINE__);
    
private:
    SQLHSTMT hStmt_;
    std::string strSql_;
    unsigned int paramNum_;
    unsigned int paramSetSize_;
    std::vector<BIND_PARAM_INFO> bindParamInfoVec_;
}; 

} // namespace ODBC

#endif // __ODBC_PREPARED_STATEMENT_H__
