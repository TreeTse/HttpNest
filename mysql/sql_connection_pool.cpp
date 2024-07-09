#include "sql_connection_pool.h"

SqlConnPool::SqlConnPool() {}

SqlConnPool *SqlConnPool::GetInstance()
{
	static SqlConnPool connPool;
	return &connPool;
}

void SqlConnPool::Init(const char *host, const char *user, const char *password, const char *dbname,
						  int port, int maxConn)
{
	assert(maxConn > 0);
	for (int i = 0; i < maxConn; i++) {
		MYSQL *con = nullptr;
		con = mysql_init(con);
		if (!con) {
			LOG_ERROR("MySQL Init Error");
			exit(1);
		}
		con = mysql_real_connect(con, host, user, password, dbname, port, nullptr, 0);
		if (!con) {
			LOG_ERROR("MySql Connect Error");
			exit(1);
		}
		connQue_.push(con);
	}
	maxConn_ = maxConn;
	LOG_DEBUG("SqlConnPool init success");
	sem_init(&semId_, 0, maxConn_);
}

MYSQL *SqlConnPool::GetConnection()
{
	MYSQL *con = nullptr;
	if (connQue_.empty()) {
		LOG_WARN("SqlConnPool busy!");
		return nullptr;
	}
	sem_wait(&semId_);
	{
		std::lock_guard<std::mutex> locker(mtx_);
		con = connQue_.front();
		connQue_.pop();
	}
	assert(con);
	return con;
}

void SqlConnPool::ReleaseConnection(MYSQL *con)
{
	assert(con);
	std::lock_guard<std::mutex> locker(mtx_);
	connQue_.push(con);
	sem_post(&semId_);
}

void SqlConnPool::DestroyPool()
{
	std::lock_guard<std::mutex> locker(mtx_);
	while (!connQue_.empty()) {
		auto sql = connQue_.front();
		connQue_.pop();
		mysql_close(sql);
	}
	mysql_library_end();
}

int SqlConnPool::GetFreeConnCount()
{
	std::lock_guard<std::mutex> locker(mtx_);
	return connQue_.size();
}

SqlConnPool::~SqlConnPool()
{
	DestroyPool();
}

SqlConnRAII::SqlConnRAII(MYSQL **sql, SqlConnPool *connPool)
{
	assert(connPool);
	*sql = connPool->GetConnection();
	conRAII_ = *sql;
	poolRAII_ = connPool;
}

SqlConnRAII::~SqlConnRAII()
{
	if (conRAII_) {
		poolRAII_->ReleaseConnection(conRAII_);
	}
}
