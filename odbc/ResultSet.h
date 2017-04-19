#ifndef __ODBC_RESULTSET_H__
#define __ODBC_RESULTSET_H__

#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <string.h>
#include <stdint.h>

namespace ODBC
{

const int kDateTimeLength = 32;
const int kDefualtLength = 128;

class ResultSet;

class ResultSetMetaData
{
public:
    friend class ResultSet;
    ResultSetMetaData() : columnType_(0), columnLength_(0), columnValue_(NULL)
    {
        memset(columnName_, 0, sizeof(columnName_));
    }
    
    ~ResultSetMetaData()
    {
    }
    
    void setColumnName(const char* columnName)
    {
        int copyLength = (strlen(columnName) < sizeof(columnName_)) ? strlen(columnName) : sizeof(columnName_);
        memcpy(columnName_, columnName, copyLength);
        columnName_[copyLength] = 0;
    }
    
    const char* getColumnName() const
    {
        return columnName_;
    }
    
    void setColumnType(int columnType)
    {
        columnType_ = columnType;
    }
    
    int getColumnType() const
    {
        return columnType_;
    }
    
    void setColumnLength(int columnLength)
    {
        columnLength_ = columnLength;
    }
    
    int getColumnLength() const
    {
        return columnLength_;
    }

private:
    int  columnType_;
    int  columnLength_;
    char columnName_[64];
    char* columnValue_;
};

class ResultSet
{
public:
    ResultSet();
    ResultSet(SQLHSTMT hStmt);
    ~ResultSet();
    
    bool next();
    
    int getInt(int index);
    // unsigned char getTinyInt(int index);
    short int getSmallInt(int index);
    std::string getString(int index);
    
private:
    ResultSet(const ResultSet &);
    ResultSet & operator = (const ResultSet &);
    
    int  getColumnLength(int index);
    int  getColumnType(int index);
    void handleError(const char* FILE = __FILE__, int LINE = __LINE__);
    
private:
    SQLHSTMT hStmt_;
    SQLINTEGER  rowCount_; 
    SQLSMALLINT colCount_;
    ResultSetMetaData* metaData_;
};

} // namespace ODBC

#endif // __ODBC_RESULTSET_H__
