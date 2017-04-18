#include "ConnPoolMgr.h"

#include <unistd.h> // usleep

CDBConnPool::CDBConnPool() : m_nMinSize(5), m_nMaxSize(50)
{
	m_strActiveSql = "SELECT 1 FROM USERDATA WHERE 0=1";
}

CDBConnPool::~CDBConnPool()
{
	UnInitPool();
}

//初始化连接池,并申请nMinSize个连接,当所有连接都申请成功时返回true.
bool CDBConnPool::InitPool(const char* dbDNSName, 
                           const char* userName, 
                           const char* passWord, 
                           const char* szActiveSql, 
                           int nMinSize, 
                           int nMaxSize)
{
	bool bRet = true;
	if (NULL != szActiveSql && '\0' != szActiveSql[0])
	{
		m_strActiveSql = szActiveSql;
	}
    m_dbDNSName = dbDNSName;
    m_userName  = userName;
    m_passWord  = passWord;
    
	m_nMinSize = nMinSize;
	m_nMaxSize = nMaxSize;
	
	m_nMinSize < MIN_DBCONN_SIZE ? m_nMinSize = MIN_DBCONN_SIZE : 1;
	m_nMinSize > MAX_DBCONN_SIZE ? m_nMinSize = MAX_DBCONN_SIZE : 1;
	m_nMaxSize < MIN_DBCONN_SIZE ? m_nMaxSize = MIN_DBCONN_SIZE : 1;
	m_nMaxSize > MAX_DBCONN_SIZE ? m_nMaxSize = MAX_DBCONN_SIZE : 1;
	
	for (int i = 0; i < m_nMinSize; ++i)
	{
		DBCONN conn;
		if (ApplayNewConn(conn))
		{
			m_Idle.push_back(conn);
		}
		else
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}

//申请新连接
bool CDBConnPool::ApplayNewConn(DBCONN & conn)
{
	bool bRet = false;
	conn.conn = new ODBC::Connection();
	if (NULL != conn.conn)
	{
		conn.conn->init(m_dbDNSName, m_userName, m_passWord);
		if(conn.conn->connect())
		{
			time(&conn.lastActive);
			bRet = true;
		}
		else
		{
			delete [] conn.conn;
			conn.conn = NULL;
		}
	}
	return bRet;
}

//获取连接池信息
void CDBConnPool::GetConnPoolInfo(int& nMinSize, int& nMaxSize, int& nIdle, int& nBusy)
{
    ODBC::MutexLockGuard guard(m_mutex);
	nMinSize = m_nMinSize;
	nMaxSize = m_nMaxSize;
	nIdle    = m_Idle.size();
	nBusy	 = m_InUse.size();
}

//重置连接池大小
void CDBConnPool::ResetConnPoolSize(int nMinSize, int nMaxSize)
{
	m_nMinSize = nMinSize;
	m_nMaxSize = nMaxSize;
	m_nMinSize < MIN_DBCONN_SIZE ? m_nMinSize = MIN_DBCONN_SIZE : 1;
	m_nMinSize > MAX_DBCONN_SIZE ? m_nMinSize = MAX_DBCONN_SIZE : 1;
	m_nMaxSize < MIN_DBCONN_SIZE ? m_nMaxSize = MIN_DBCONN_SIZE : 1;
	m_nMaxSize > MAX_DBCONN_SIZE ? m_nMaxSize = MAX_DBCONN_SIZE : 1;
}

//销毁连接池,并释放所有连接
void CDBConnPool::UnInitPool()
{
	ODBC::MutexLockGuard guard(m_mutex);
	std::list<DBCONN>::iterator it = m_Idle.begin();
	for ( ; it != m_Idle.end(); ++it)
	{
	    if (it->conn)
	    {
	        it->conn->close();
		    delete it->conn;
		    it->conn = NULL;
		}
	}
	
	m_Idle.clear();
	std::map<ODBC::Connection*, int>::iterator it2 = m_InUse.begin();
	for ( ; it2 != m_InUse.end(); ++it2)
	{
	    if (it2->first)
	    {
	        it2->first->close();
		    delete it2->first;
		}
	}
	m_InUse.clear();
}

//从池子中获取一个连接.可指定最大等待时间(单位:s),超时将自动返回NULL.最大等待时间最小值为5s
ODBC::Connection* CDBConnPool::GetConnFromPool(int nMaxTimeWait)
{
	nMaxTimeWait < 5 ? nMaxTimeWait = 5 : 1;
	ODBC::Connection* pCon = NULL;
	//看空闲队列中是否有可用的,若有则直接返回,若没有,查看是否超过池子最大值,若没超出则申请一个新的返回,若超出则等待
	ODBC::MutexLockGuard guard(m_mutex);
	if (!m_Idle.empty())
	{
		std::list<DBCONN>::iterator it = m_Idle.begin();
		for ( ; it != m_Idle.end(); )
		{
			//2013-06-28
			//取连接时不每次都active,减少数据库操作次数
			pCon = it->conn;
			time(&it->lastActive);
			m_InUse.insert(std::make_pair(it->conn, 0));
			m_Idle.erase(it++);
			break;
		}
	}
	//空闲队列为空,但总连接没有超过最大值,则重新申请一个连接
	else if (m_Idle.empty() && (int)m_InUse.size() < m_nMaxSize)
	{
		DBCONN conn;
		if (ApplayNewConn(conn))
		{
			pCon = conn.conn;
			m_InUse.insert(std::make_pair(pCon, 0));
		}
	}

	return pCon;
}

//回收连接
void CDBConnPool::RecycleConn(ODBC::Connection* pConn)
{
	if (NULL != pConn)
	{
		ODBC::MutexLockGuard guard(m_mutex);
		DBCONN conn;
		conn.conn = pConn;
		time(&conn.lastActive);
		//回收前检测连接是否可用,若不可用则清除,然后再判断是否小于最小值,若小于则再申请一下
		if (m_InUse.find(pConn) != m_InUse.end())
		{
			m_Idle.push_back(conn);
			m_InUse.erase(pConn);
		}
	}
}

//测试连接是否可用
bool CDBConnPool::TestConn(ODBC::Connection* pConn)
{
	//bool bRet = false;
	//if (NULL != pConn)
	//{
	//	int nRet = pConn->ActivateDBConnection(m_strActiveSql.c_str());
	//	if (0 == nRet || (97 == nRet && pConn->OpenDatabase() && 0 == pConn->ActivateDBConnection(m_strActiveSql.c_str())))
	//	{
	//		bRet = true;
	//	}
	//}
	//return bRet;
	return true;
}

//激活长时间空闲的连接(若当前连接总数大于连接池上限，则主动关闭空闲连接)
void CDBConnPool::ActiveIdleConn()
{
	ODBC::MutexLockGuard guard(m_mutex);
	std::list<DBCONN>::iterator it = m_Idle.begin();
	for ( ; it != m_Idle.end(); )
	{
		//有空闲的并且连接总数超过了上限，则主动关闭连接
		if ((int)m_InUse.size() + (int)m_Idle.size() > m_nMaxSize)
		{
			it->conn->close();
			m_Idle.erase(it++);
		}
		else
		{
			//对连接进行激活
			if (time(NULL) - it->lastActive > MAX_DBCONN_ACTIVE_TM)
				    //&& 0 != it->pConn->ActivateDBConnection(m_strActiveSql.c_str()))
			{
				it->conn->close();
				m_Idle.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}
}

//需要检测连接是否还有效
bool CDBConnPool::IsConnValid(ODBC::Connection* pConn)
{
	return TestConn(pConn);
}

///CDBConnPoolMgr
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDBConnPoolMgr& CDBConnPoolMgr::GetInstance()
{
    static CDBConnPoolMgr instance;
    return instance;
}

CDBConnPoolMgr::CDBConnPoolMgr() : m_bExit(false)
{
}

CDBConnPoolMgr::~CDBConnPoolMgr()
{
	//通知线程退出
	m_bExit = true;
	//等待线程退出
	//while (m_bExit) CAdapter::Sleep(200);

	ODBC::MutexLockGuard guard(m_mutex);
	std::map<std::string, CDBConnPool* >::iterator it = m_dbpool.begin();
	for ( ; it != m_dbpool.end(); ++it)
	{
		it->second->UnInitPool();
		delete it->second;
		it->second = NULL;
	}
	m_dbpool.clear();
}

//创建连接池并指定池子的名称,若使用""做为池子名称,则创建默认池名,池子名称区分大小写
bool CDBConnPoolMgr::CreateConnPool(const char* szPoolName, 
                                    const char* dbDNSName, 
                                    const char* userName, 
                                    const char* passWord, 
                                    const char* szActiveSql/*激活语句*/, 
                                    int nMinSize, 
                                    int nMaxSize)
{
	bool bRet = false;
	CDBConnPool* pPool = new CDBConnPool();
	if (NULL != pPool)
	{
		if (pPool->InitPool(dbDNSName, userName, passWord, szActiveSql, nMinSize, nMaxSize))
		{
			ODBC::MutexLockGuard guard(m_mutex);
			m_dbpool.insert(std::make_pair(szPoolName, pPool));
			bRet = true;
		}
		else
		{
			delete [] pPool;
			pPool = NULL;
		}
	}
	return bRet;
}

//从指定的池子中获取一个连接.可指定最大等待时间(单位:s),超时将自动返回NULL.最大等待时间最小值为5s
ODBC::Connection* CDBConnPoolMgr::GetConnFromPool(const char* szPoolName, int nMaxTimeWait)
{
	ODBC::Connection* pCon = NULL;
	CDBConnPool* pPool = NULL;
	{
		ODBC::MutexLockGuard guard(m_mutex);
		std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
		if (it != m_dbpool.end())
		{
			pPool = it->second;
		}
	}
	if (NULL != pPool)
	{
		pCon = pPool->GetConnFromPool(nMaxTimeWait);
	}
	//每次的延时值
	nMaxTimeWait < MIN_WAITCONN_TM ? nMaxTimeWait = MIN_WAITCONN_TM : 1;
	const int nDelay = 100;
	int nMaxLoopCnt = nMaxTimeWait * 1000/ nDelay;
	while (NULL == pCon && --nMaxLoopCnt >= 0)
	{
		{
			ODBC::MutexLockGuard guard(m_mutex);
			std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
			if (it != m_dbpool.end())
			{
				pPool = it->second;
			}
		}

		if (NULL != pPool)
		{
			pCon = pPool->GetConnFromPool(nMaxTimeWait);
			if (NULL != pCon) break;
		}

		usleep(nDelay);
	}

	return pCon;
}

//将连接回收至指定池子中
void CDBConnPoolMgr::RecycleConn(const char* szPoolName, ODBC::Connection* pConn)
{
	CDBConnPool * pPool = NULL;
	{
		ODBC::MutexLockGuard guard(m_mutex);
		std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
		if (it != m_dbpool.end())
		{
			pPool = it->second;
		}
	}

	if (NULL != pPool)
	{
		pPool->RecycleConn(pConn);
	}
}

//需要检测连接是否还有效
bool CDBConnPoolMgr::IsConnValid(const char* szPoolName, ODBC::Connection* pConn)
{
	bool bRet = false;
	CDBConnPool * pPool = NULL;
	{
		ODBC::MutexLockGuard guard(m_mutex);
		std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
		if (it != m_dbpool.end())
		{
			pPool = it->second;
		}
	}
	if (pPool)
	{
		bRet = pPool->TestConn(pConn);
		//若数据库连接断开,则延时一定时间
		if (!bRet) 
		    usleep(5000);
	}
	
	return bRet;
}

//获取连接池信息
void CDBConnPoolMgr::GetConnPoolInfo(const char* szPoolName, int& nMinSize, int& nMaxSize, int& nIdle, int& nBusy)
{
	CDBConnPool * pPool = NULL;
	{
		ODBC::MutexLockGuard guard(m_mutex);
		std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
		if (it != m_dbpool.end())
		{
			pPool = it->second;
		}
	}
	
	if (NULL != pPool)
	{
		pPool->GetConnPoolInfo(nMinSize, nMaxSize, nIdle, nBusy);
	}
}

//重置连接池大小
void CDBConnPoolMgr::ResetConnPoolSize(const char* szPoolName, int nMinSize, int nMaxSize)
{
	CDBConnPool * pPool = NULL;
	{
		ODBC::MutexLockGuard guard(m_mutex);
		std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.find(szPoolName);
		if (it != m_dbpool.end())
		{
			pPool = it->second;
		}
	}

	if (NULL != pPool)
	{
		pPool->ResetConnPoolSize(nMinSize, nMaxSize);
	}
}

void CDBConnPoolMgr::ActiveIdleConn()
{
	std::map<std::string, CDBConnPool*>::iterator it = m_dbpool.begin();
	for ( ; it != m_dbpool.end(); ++it)
	{
		if (NULL != it->second) it->second->ActiveIdleConn();
	}
}
