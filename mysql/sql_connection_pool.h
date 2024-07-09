#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <mysql.h>
#include <queue>
#include <semaphore.h>
#include "../log/log.h"

class SqlConnPool
{
public:
	void Init(const char *host, const char *user, const char *password, const char *dbname, int port, int maxConn);

	static SqlConnPool *GetInstance();

	MYSQL *GetConnection();

	void ReleaseConnection(MYSQL *conn);

	int GetFreeConnCount();

	void DestroyPool();

private:
	SqlConnPool();

	~SqlConnPool();

	int maxConn_;
	std::queue<MYSQL *> connQue_;
	sem_t semId_;
	std::mutex mtx_;
};

class SqlConnRAII{

public:
	SqlConnRAII(MYSQL **con, SqlConnPool *connPool);
	~SqlConnRAII();
	
private:
	MYSQL *conRAII_;
	SqlConnPool *poolRAII_;
};

#endif
