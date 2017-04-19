#include <odbcwapper/PreparedStatement.h>
#include <odbcwapper/Exception.h>

#include <stdio.h>

namespace ODBC
{

PreparedStatement::PreparedStatement(unsigned int paramSetSize) 
        : hStmt_(NULL), paramNum_(0), paramSetSize_(paramSetSize)
{
}

PreparedStatement::PreparedStatement(SQLHDBC hDbc, const std::string & strSql, unsigned int paramSetSize)
        : hStmt_(NULL), strSql_(strSql), paramNum_(0), paramSetSize_(paramSetSize)
{
    SQLRETURN retCode;
    retCode = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO)) 
    {
        // 抛异常
        handleError(__FILE__, __LINE__);
    }
    
    // 设置列绑定
    retCode = SQLSetStmtAttr(hStmt_, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)SQL_PARAM_BIND_BY_COLUMN, 0);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        // 抛异常
        handleError(__FILE__, __LINE__);
    }
    
    std::cout << "SQL: " << strSql << std::endl;
    retCode = SQLPrepare(hStmt_, (SQLCHAR* )strSql.c_str(), SQL_NTS);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO)) 
    {
        // 抛异常
        handleError(__FILE__, __LINE__);
    }
    
    // 设置绑定参数的次数（默认为1）
    SQLSetStmtAttr(hStmt_, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)paramSetSize_, 0);
}

PreparedStatement::~PreparedStatement()
{
    if (hStmt_)
    {
        SQLCancel(hStmt_);
        hStmt_ = NULL;
    }
    
    for (size_t i = 0; i < bindParamInfoVec_.size(); ++i)
    {
        if (bindParamInfoVec_[i].indArr != NULL)
        {
            delete [] (bindParamInfoVec_[i].indArr);
            bindParamInfoVec_[i].indArr = NULL;
        }
        
        if (bindParamInfoVec_[i].paramValue != NULL)
        {
            delete [] (bindParamInfoVec_[i].paramValue);
            bindParamInfoVec_[i].paramValue = NULL;
        }
    }
    bindParamInfoVec_.clear();
}

void PreparedStatement::resizeParamTable(int maxSize)
{
    // std::cout << "maxSize = " << maxSize << ", paramNum_ = " << paramNum_ << std::endl;
    if ((unsigned int)maxSize > paramNum_)
    {
        int offSet = maxSize - paramNum_;
        // std::cout << "offSet = " << offSet << std::endl;
        for (int i = 0; i < offSet; ++i)
        {
            BIND_PARAM_INFO paramInfo;
            paramInfo.indArr = new SQLLEN[paramSetSize_];
            memset(paramInfo.indArr, 0, sizeof(SQLLEN) * paramSetSize_);
            bindParamInfoVec_.push_back(paramInfo);
        }
    }
}

void PreparedStatement::setParamAttr(int index, SQLSMALLINT sqlType, int maxSize)
{
    int nIndex = index - 1;
    if (nIndex < 0)
    {
        char buffer[256] = { 0 };
        snprintf(buffer, sizeof(buffer), "(%s:%d) set param index must be > 0.", __FILE__, __LINE__);
        throw ODBC::Exception(buffer);
    }
    // std::cout << "index = " << index << std::endl;
    resizeParamTable(index);
    
    SQLULEN     columnSize = 0;
    SQLLEN      columnMax  = 0;
    SQLSMALLINT sql_c_type    = SQL_C_CHAR;
    SQLLEN      sql_indicator = 0;
    switch (sqlType)
    {
    case SQL_BIGINT:
        sqlType = SQL_NUMERIC;
    case SQL_NUMERIC:
    case SQL_CHAR:	
	case SQL_VARCHAR:
	    {
	        columnSize = maxSize - 1;
	        columnMax  = maxSize;
	        sql_c_type = SQL_C_CHAR;
	        sql_indicator = SQL_NTS;
	    }
	    break;
	case SQL_INTEGER:
	    {
	        columnSize = 0;
	        columnMax  = sizeof(SQLINTEGER);
	        sql_c_type = SQL_C_LONG;
	        sql_indicator = 0;
	    }
	    break;
	case SQL_SMALLINT:
	    {
	        columnSize = 0;
	        columnMax  = sizeof(SQLSMALLINT);
	        sql_c_type = SQL_C_SHORT;
	        sql_indicator = 0;
	    }
	    break;
	case SQL_TYPE_TIMESTAMP:
	    {
	        sqlType = SQL_VARCHAR;
	        columnSize = maxSize - 1;
	        columnMax  = maxSize;
	        sql_c_type = SQL_C_CHAR;
			sql_indicator = SQL_NTS;
	    }
	    break;
	default:
	    {
	        columnSize = maxSize - 1;
	        columnMax  = maxSize;
	        sql_c_type = SQL_C_CHAR;
			sql_indicator = SQL_NTS;
	    }
	    break;
    }
    
    // std::cout << "nIndex = " << nIndex <<std::endl;

    bindParamInfoVec_[nIndex].columnSize = columnSize;
    bindParamInfoVec_[nIndex].sql_c_type = sql_c_type;
    bindParamInfoVec_[nIndex].sql_type   = sqlType;
    
    for (unsigned int i = 0; i < paramSetSize_; ++i)
    {
        bindParamInfoVec_[nIndex].indArr[i] = sql_indicator;
    }
    
    unsigned int nMemSize = columnMax * paramSetSize_;
    if (NULL == bindParamInfoVec_[nIndex].paramValue)
    {
        bindParamInfoVec_[nIndex].memSize    = nMemSize;
        bindParamInfoVec_[nIndex].columnMax  = columnMax;
        bindParamInfoVec_[nIndex].paramValue = new SQLCHAR[nMemSize];
    }
    else
    {
        if (columnMax > bindParamInfoVec_[nIndex].columnMax)
        {
            delete [] (bindParamInfoVec_[nIndex].paramValue);
            bindParamInfoVec_[nIndex].memSize    = nMemSize;
            bindParamInfoVec_[nIndex].columnMax  = columnMax;
            bindParamInfoVec_[nIndex].paramValue = new SQLCHAR[nMemSize];
        }
    }
    
    memset(bindParamInfoVec_[nIndex].paramValue, 0, nMemSize);
}

ResultSet* PreparedStatement::executeQuery()
{
    SQLRETURN retCode = SQLExecute(hStmt_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return NULL;
    }
    
    return new ResultSet(hStmt_);
}

int PreparedStatement::executeUpdate()
{
    SQLRETURN retCode = SQLExecute(hStmt_);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return -1;
    }
    
    int result = -1;
    retCode = SQLRowCount(hStmt_, (SQLLEN* )&result);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return -1; 
    }
    return result;
}

/*
void PreparedStatement::setTinyInt(int index, unsigned char value)
{
    setParamAttr(index, SQL_NUMERIC, sizeof(SQLNUMERIC));
    
    SQLRETURN retCode = SQLBindParameter(hStmt_,
                                         index, 
                                         SQL_PARAM_INPUT, 
                                         bindParamInfoVec_[index-1].sql_c_type,
                                         bindParamInfoVec_[index-1].sql_type,
                                         bindParamInfoVec_[index-1].columnSize, 
                                         0, 
                                         bindParamInfoVec_[index-1].paramValue, 
                                         bindParamInfoVec_[index-1].columnMax, 
                                         bindParamInfoVec_[index-1].indArr);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return;
    }
    memcpy(bindParamInfoVec_[index-1].paramValue, &value, sizeof(unsigned char));
    std::cout << "index = " << index << ", value = " << value << std::endl;  
}
*/

void PreparedStatement::setSmallInt(int index, short int value)
{
    setParamAttr(index, SQL_SMALLINT, sizeof(SQLSMALLINT));
    
    SQLRETURN retCode = SQLBindParameter(hStmt_,
                                         index, 
                                         SQL_PARAM_INPUT, 
                                         bindParamInfoVec_[index-1].sql_c_type,
                                         bindParamInfoVec_[index-1].sql_type,
                                         bindParamInfoVec_[index-1].columnSize, 
                                         0, 
                                         bindParamInfoVec_[index-1].paramValue, 
                                         bindParamInfoVec_[index-1].columnMax, 
                                         bindParamInfoVec_[index-1].indArr);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return;
    }
    memcpy(bindParamInfoVec_[index-1].paramValue, &value, sizeof(short int));
    std::cout << "index = " << index << ", value = " << value << std::endl;  
}

void PreparedStatement::setInt(int index, int value)
{
    setParamAttr(index, SQL_INTEGER, sizeof(SQLINTEGER));
    
    SQLRETURN retCode = SQLBindParameter(hStmt_,
                                         index, 
                                         SQL_PARAM_INPUT, 
                                         bindParamInfoVec_[index-1].sql_c_type,
                                         bindParamInfoVec_[index-1].sql_type,
                                         bindParamInfoVec_[index-1].columnSize, 
                                         0, 
                                         bindParamInfoVec_[index-1].paramValue, 
                                         bindParamInfoVec_[index-1].columnMax, 
                                         bindParamInfoVec_[index-1].indArr);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return;
    }
    memcpy(bindParamInfoVec_[index-1].paramValue, &value, sizeof(int));
    std::cout << "index = " << index << ", value = " << value << std::endl;
}

void PreparedStatement::setString(int index, std::string & value)
{
    setParamAttr(index, SQL_VARCHAR, value.size() + 1);
    SQLRETURN retCode = SQLBindParameter(hStmt_,
                                         index, 
                                         SQL_PARAM_INPUT, 
                                         bindParamInfoVec_[index-1].sql_c_type,
                                         bindParamInfoVec_[index-1].sql_type,
                                         bindParamInfoVec_[index-1].columnSize, 
                                         0, 
                                         bindParamInfoVec_[index-1].paramValue, 
                                         bindParamInfoVec_[index-1].columnMax, 
                                         bindParamInfoVec_[index-1].indArr);
    if ((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))
    {
        handleError(__FILE__, __LINE__);
        return;
    }
    
    memcpy(bindParamInfoVec_[index-1].paramValue, value.c_str(), value.length());
    std::cout << "index = " << index << ", value = " << (bindParamInfoVec_[index-1].paramValue) << std::endl;
}

void PreparedStatement::handleError(const char* FILE, int LINE)
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

