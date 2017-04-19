#include <odbc/ResultSet.h>
#include <odbc/Exception.h>

#include <stdio.h>

namespace ODBC
{

ResultSet::ResultSet() : hStmt_(NULL), rowCount_(0), colCount_(0), metaData_(NULL)
{
}

ResultSet::ResultSet(SQLHSTMT hStmt) : hStmt_(hStmt), rowCount_(0), colCount_(0), metaData_(NULL)
{
    SQLRETURN retCode;
    retCode = SQLNumResultCols(hStmt_, &colCount_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        // 抛异常
        handleError(__FILE__, __LINE__);
    }
    
    if (colCount_ < 1)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns number must be >= 1.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    metaData_ = new ResultSetMetaData[colCount_];
    if (NULL == metaData_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) system new ResultSetMetaData[] fail.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    // 设置每一列的属性
    char  columnName[128];
    int   columnType;
    int   columnLength;
    int   bufferLen;
    
    for (int i = 1; i <= colCount_; i++)
    {
        SQLColAttribute(hStmt_, i, SQL_DESC_NAME, columnName, 128, (SQLSMALLINT* )&bufferLen, 0);
        SQLColAttribute(hStmt_, i, SQL_DESC_TYPE, 0, 0, 0, (SQLLEN* )&columnType);  
        SQLColAttribute(hStmt_, i, SQL_DESC_LENGTH, NULL, 0, 0, (SQLLEN* )&columnLength);
        
        // columnName[bufferLen] = 0;
        std::cout << "columnName = " << columnName 
                  << ", bufferLen = " << bufferLen 
                  << ", columnType = " << columnType 
                  << ", columnLength = " << columnLength << std::endl;
        
        switch (columnType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
            {
                if (columnLength <= 0)
                {
                    char errMsg[1024] = { 0 };
                    snprintf(errMsg, sizeof(errMsg), "(%s:%d) column length must be > 0.", __FILE__, __LINE__);
                    throw ODBC::Exception(errMsg);
                }
                metaData_[i-1].columnValue_ = new char[columnLength];
            }
            break;
        case SQL_DATETIME:
            {
                metaData_[i-1].columnValue_ = new char[kDateTimeLength];
                columnLength = kDateTimeLength;
            }
            break;
        default:
            {
                metaData_[i-1].columnValue_ = new char[kDefualtLength];
                columnLength = kDefualtLength;
            }
        }
        
        metaData_[i-1].setColumnType(columnType);
        metaData_[i-1].setColumnLength(columnLength);
        metaData_[i-1].setColumnName(columnName);
    }
}

ResultSet::~ResultSet()
{
    if (metaData_)
    {
        for (int i = 0; i < colCount_; i++)
        {
            if (metaData_[i].columnValue_)
                delete [] metaData_[i].columnValue_;
        }
        delete [] metaData_;
    }
}

bool ResultSet::next()
{
    return (SQLFetch(hStmt_) != SQL_NO_DATA); 
}

int ResultSet::getColumnType(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    if (NULL == metaData_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) metaData_ can't is null.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    return metaData_[index - 1].getColumnType();
}

int ResultSet::getColumnLength(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    if (NULL == metaData_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) metaData_ can't is null.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    return metaData_[index - 1].getColumnLength();
}

int ResultSet::getInt(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    long value = 0;
    SQLLEN pIndicators; 
    SQLRETURN retCode;
    retCode = SQLGetData(hStmt_, index, SQL_C_LONG, (SQLPOINTER)&value, sizeof(long), &pIndicators);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
    }
    return value;
}

/*
unsigned char ResultSet::getTinyInt(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    unsigned char value = 0;
    SQLLEN pIndicators; 
    SQLRETURN retCode;
    retCode = SQLGetData(hStmt_, index, SQL_C_NUMERIC, (SQLPOINTER)&value, sizeof(unsigned char), &pIndicators);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
    }
    return value;
}
*/

short int ResultSet::getSmallInt(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    short int value = 0;
    SQLLEN pIndicators; 
    SQLRETURN retCode;
    retCode = SQLGetData(hStmt_, index, SQL_C_SHORT, (SQLPOINTER)&value, sizeof(short int), &pIndicators);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
    }
    return value;
}

std::string ResultSet::getString(int index)
{
    if (index < 1 || index > colCount_)
    {
        char errMsg[1024] = { 0 };
        snprintf(errMsg, sizeof(errMsg), "(%s:%d) columns index must be >= 1 or <= colCount_.", __FILE__, __LINE__);
        throw ODBC::Exception(errMsg);
    }
    
    SQLRETURN retCode;
    long valueLength = 0;
    memset(metaData_[index-1].columnValue_, 0, metaData_[index-1].getColumnLength());
    retCode = SQLGetData(hStmt_, index, SQL_C_CHAR, metaData_[index-1].columnValue_, metaData_[index-1].getColumnLength(), (SQLLEN* )&valueLength);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
    }
    
    return metaData_[index-1].columnValue_;
}

void ResultSet::handleError(const char* FILE, int LINE)
{
    SQLSMALLINT errmsglen;
    SQLINTEGER errnative;
    
    UCHAR errmsg[255];
    UCHAR errstate[5];
    
    char buffer[1024]= { 0 };

    SQLGetDiagRec(SQL_HANDLE_STMT, hStmt_, 1, errstate, &errnative, errmsg, sizeof(errmsg), &errmsglen);
    snprintf(buffer, sizeof(buffer), "(%s:%d) errstate(%s), errnative(%d),errmsg(%s)", 
            FILE, LINE, errstate, errnative, errmsg);
    
    throw ODBC::Exception(buffer);
}

} // namespace ODBC
