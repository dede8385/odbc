#include <iostream>
#include <time.h>
#include <stdio.h>
#include <odbcwapper/Connection.h>
#include <odbcwapper/Exception.h>
#include <odbcwapper/PreparedStatement.h>
#include <odbcwapper/ResultSet.h>
#include <odbcwapper/ConnPoolMgr.h>
#include <boost/scoped_ptr.hpp>

#define DEFAULT_DBPOOL  "montnetdb"
#define DEFAULT_DBPOOL2 "liang_pt"

using namespace std;
using namespace ODBC;

const char* kUserNameBase = "dedede";

void PrintConnPoolInfo()
{
    int nMinSize, nMaxSize, nIdle, nBusy;
    CDBConnPoolMgr::GetInstance().GetConnPoolInfo(DEFAULT_DBPOOL, nMinSize, nMaxSize, nIdle, nBusy);
    cout << "--------------------" << endl;
    cout << "nMinSize = " << nMinSize << endl;
    cout << "nMaxSize = " << nMaxSize << endl;
    cout << "nIdle = " << nIdle << endl;
    cout << "nBusy = " << nBusy << endl;
    cout << "--------------------" << endl;
}

string getTimeOfDay()
{
    time_t t;
    struct tm* p;
    time(&t);
    p = localtime(&t);
    
    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, 
            p->tm_hour, p->tm_min, p->tm_sec, 0);
    return buffer;
}

void test_insert(short int tinyflag, short int smallflag, int intflag, long bigintflag)
{
    try
    {
        const char* strSql = "insert into test(tinyflag,smallflag,intflag,bigintflag)values(?,?,?,?)";
	    Connection* conn = CDBConnPoolMgr::GetInstance().GetConnFromPool(DEFAULT_DBPOOL);
	    CDBConnectionPtr connPtr(DEFAULT_DBPOOL, conn);
	    boost::scoped_ptr<PreparedStatement> pstmtPtr(conn->prepareStatement(strSql));
	    if (pstmtPtr == NULL)
	    {
	        cout << "生成PreparedStatement对象失败！！" << endl;
	        return;
	    }
    
        pstmtPtr->setSmallInt(1, tinyflag);
        pstmtPtr->setSmallInt(2, smallflag);
        pstmtPtr->setInt(3, intflag);
        pstmtPtr->setInt(4, bigintflag);
        
        int number = pstmtPtr->executeUpdate();
        cout << "number = " << number << endl;
    }
    catch (const ODBC::Exception & e)
    {
        cout << e.what() << endl;
        return;
    }
    catch (...)
    {
        cout << "system error!!" << endl;
        return;
    }
}

void test_select(const string & userId)
{
    try
    {
        const char* strSql = "select top 1 uid from userdata where userid=?";
	    Connection* conn = CDBConnPoolMgr::GetInstance().GetConnFromPool(DEFAULT_DBPOOL2);
	    CDBConnectionPtr connPtr(DEFAULT_DBPOOL2, conn);
	    boost::scoped_ptr<PreparedStatement> pstmtPtr(conn->prepareStatement(strSql));
	    if (pstmtPtr == NULL)
	    {
	        cout << "生成PreparedStatement对象失败！！" << endl;
	        return;
	    }
    
        pstmtPtr->setString(1, userId);
        boost::scoped_ptr<ResultSet> rsPtr(pstmtPtr->executeQuery());
        
        int rowCount = 0;
        while (rsPtr && rsPtr->next())
	    {
	        ++rowCount;
	        int uid = rsPtr->getInt(1);
	        cout << "uid = " << uid  << endl;
	        cout << "--------------------------" << endl;
	    }
	    
	    if (rowCount == 0)
	    {
	        cout << "userId = " << userId << " not ResultSet! " << endl;
	    }
    }
    catch (const ODBC::Exception & e)
    {
        cout << e.what() << endl;
        return;
    }
    catch (...)
    {
        cout << "system error!!" << endl;
        return;
    }
}

void test_select2()
{
    try
    {
        const char* strSql = "select tinyflag,smallflag,intflag from test";
	    Connection* conn = CDBConnPoolMgr::GetInstance().GetConnFromPool(DEFAULT_DBPOOL);
	    CDBConnectionPtr connPtr(DEFAULT_DBPOOL, conn);
	    boost::scoped_ptr<PreparedStatement> pstmtPtr(conn->prepareStatement(strSql));
	    if (pstmtPtr == NULL)
	    {
	        cout << "生成PreparedStatement对象失败！！" << endl;
	        return;
	    }
        
        boost::scoped_ptr<ResultSet> rsPtr(pstmtPtr->executeQuery());
        
        int rowCount = 0;
        while (rsPtr && rsPtr->next())
	    {
	        ++rowCount;
	        short int tinyflag  = rsPtr->getSmallInt(1);
	        short int smallflag = rsPtr->getSmallInt(2);
	        int intflag = rsPtr->getInt(3);
	        
	        cout << "tinyflag = " << tinyflag  << ", smallflag = " << smallflag  << ", intflag = " << intflag  << endl;
	        cout << "------------------------------------------------------------------------------------" << endl;
	    }
	    
	    cout << "rowCount = " << rowCount << endl;
    }
    catch (const ODBC::Exception & e)
    {
        cout << e.what() << endl;
        return;
    }
    catch (...)
    {
        cout << "system error!!" << endl;
        return;
    }
}

int main()
{
    cout << "sizeof(int): " << sizeof(int) << endl;
    cout << "sizeof(long): " << sizeof(long) << endl;
	
	// 启动数据库1
	if (CDBConnPoolMgr::GetInstance().CreateConnPool(DEFAULT_DBPOOL, 
	        "montnetdb131", "developer", "developer", "select 1", 5, 10) == false)
	{
	    cout << "初始化连接池管理类失败1！！！！" << endl;
	    return -1;
	}
	// 启动数据库2
	//if (CDBConnPoolMgr::GetInstance().CreateConnPool(DEFAULT_DBPOOL2, 
	//        "liang_pt_198", "sa", "123456", "select 1", 5, 10) == false)
	//{
	//    cout << "初始化连接池管理类失败2！！！！" << endl;
	//    return -1;
	//}
	for (int i = 1; i <= 1; i++)
	{
        short int tinyflag = 1;
        short int smallflag = 2;
        int intflag = 3;
        long bigintflag = 10000000;
	    test_insert(tinyflag, smallflag, intflag, bigintflag);
	}
	
	test_select2();
	PrintConnPoolInfo();
	
	/*
	// 存储过程
	{
	    const char* strSql = "exec testUsers";
	    Connection* conn = CDBConnPoolMgr::GetInstance().GetConnFromPool(DEFAULT_DBPOOL);
	    CDBConnectionPtr connPtr(DEFAULT_DBPOOL, conn);
	    boost::scoped_ptr<PreparedStatement> pstmtPtr(conn->prepareStatement(strSql));
	    if (pstmtPtr == NULL)
	    {
	        cout << "生成PreparedStatement对象失败！！" << endl;
	        return -1;
	    }
	    
	    boost::scoped_ptr<ResultSet> rsPtr(pstmtPtr->executeQuery());
	    while (rsPtr && rsPtr->next())
	    {
	        int userId = rsPtr->getInt(1);
	        string userName = rsPtr->getString(2);
	        string loginDate = rsPtr->getString(3);
	        cout << endl;
	        cout << "userId = " << userId << ", userName = " << userName << ",loginDate = " << loginDate << endl;
	        cout << "--------------------------------------------------------------------" << endl;
	    }
	    PrintConnPoolInfo();
	}
	*/
	return 0;
	
}
