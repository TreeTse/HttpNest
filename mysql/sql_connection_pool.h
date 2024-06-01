#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class ConnectionPool
{
public:
	MYSQL *GetConnection();
	bool ReleaseConnection(MYSQL *conn);
	int GetFreeConn();
	void DestroyPool();
	static ConnectionPool *GetInstance();
	void Init(string url, string user, string password, string dbname, int port, int max_conn, int close_log);

private:
	ConnectionPool();
	~ConnectionPool();
	int maxConn_;
	int curConn_;
	int freeConn_;
	Locker lock_;
	list<MYSQL *> connList_;
	Sem reserve_;

public:
	string url_;
	string port_;
	string user_;
	string passWord_;
	string databaseName_;
	int optCloseLog_;
};

class ConnectionRAII{

public:
	ConnectionRAII(MYSQL **con, ConnectionPool *connPool);
	~ConnectionRAII();
	
private:
	MYSQL *conRAII_;
	ConnectionPool *poolRAII_;
};

#endif
