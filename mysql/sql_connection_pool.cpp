#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

ConnectionPool::ConnectionPool()
{
	curConn_ = 0;
	freeConn_ = 0;
}

ConnectionPool *ConnectionPool::GetInstance()
{
	static ConnectionPool connPool;
	return &connPool;
}

void ConnectionPool::Init(string url, string user, string password, string dbname, 
						  int port, int max_conn, int close_log)
{
	url_ = url;
	port_ = port;
	user_ = user;
	passWord_ = password;
	databaseName_ = dbname;
	optCloseLog_ = close_log;

	for (int i = 0; i < max_conn; i++) {
		MYSQL *con = NULL;
		con = mysql_init(con);
		if (con == NULL) {
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0);
		if (con == NULL) {
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		connList_.push_back(con);
		++freeConn_;
	}

	reserve_ = Sem(freeConn_);
	maxConn_ = freeConn_;
}

MYSQL *ConnectionPool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == connList_.size())
		return NULL;

	reserve_.Wait();
	lock_.Lock();
	con = connList_.front();
	connList_.pop_front();
	--freeConn_;
	++curConn_;
	lock_.Unlock();
	return con;
}

bool ConnectionPool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	lock_.Lock();
	connList_.push_back(con);
	++freeConn_;
	--curConn_;
	lock_.Unlock();

	reserve_.Post();
	return true;
}

void ConnectionPool::DestroyPool()
{
	lock_.Lock();
	if (connList_.size() > 0) {
		list<MYSQL *>::iterator it;
		for (it = connList_.begin(); it != connList_.end(); ++it) {
			MYSQL *con = *it;
			mysql_close(con);
		}
		curConn_ = 0;
		freeConn_ = 0;
		connList_.clear();
	}
	lock_.Unlock();
}

int ConnectionPool::GetFreeConn()
{
	return this->freeConn_;
}

ConnectionPool::~ConnectionPool()
{
	DestroyPool();
}

ConnectionRAII::ConnectionRAII(MYSQL **sql, ConnectionPool *connPool)
{
	*sql = connPool->GetConnection();
	
	conRAII_ = *sql;
	poolRAII_ = connPool;
}

ConnectionRAII::~ConnectionRAII()
{
	poolRAII_->ReleaseConnection(conRAII_);
}
