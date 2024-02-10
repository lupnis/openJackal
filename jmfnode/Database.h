/*
 * file name:       Database.h
 * created at:      2024/01/18
 * last modified:   2024/01/20
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <hiredis.h>

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QtSql>

namespace JDB {
class MySQLODBCController : public QObject {
    Q_OBJECT
   public:
    MySQLODBCController();
    MySQLODBCController(QString host, quint16 port, QString user, QString pass,
                        QString default_schema, QString default_table = "");
    ~MySQLODBCController();
    void setHost(QString host, quint16 port = 3306);
    void setAuth(QString user, QString pass);
    void setDefaultSchema(QString default_schema);
    void setTable(QString table);
    QString getLocation();
    bool getConnected();
    void connect();
    void disconnect();
    bool rollback();
    QPair<int, QList<QList<QVariant>>> runsql(QString sql_cmd);
    QPair<int, QList<QList<QVariant>>> select(
        QList<QHash<QString, QVariant>> match_query =
            QList<QHash<QString, QVariant>>(),
        qint32 limit_start = 0, qint32 limit_size = 1000);
    QPair<int, QList<QList<QVariant>>> insert(
        QList<QList<QVariant>> records,
        QList<QString> columns = QList<QString>());
    QPair<int, QList<QList<QVariant>>> remove(
        QList<QHash<QString, QVariant>> match_query =
            QList<QHash<QString, QVariant>>());
    QPair<int, QList<QList<QVariant>>> modify(
        QList<QHash<QString, QVariant>> match_query =
            QList<QHash<QString, QVariant>>(),
        QHash<QString, QVariant> new_value = QHash<QString, QVariant>(),
        qint32 limit_start = 0, qint32 limit_size = 1000);

   private:
    QString make_match_str(QList<QHash<QString, QVariant>> match_query =
                               QList<QHash<QString, QVariant>>());
    QString make_limit_str(qint32 limit_start = 0, qint32 limit_size = 1000);

    QSqlDatabase database;
    QString selected_table;

    QMutex lock;
};

enum RedisDataType {
    String = 1,  // string
    Array,       // list
    Integer,
    Nil,  // none
    Status,
    Error,
    Double,
    Bool,
    Map,
    Set,  // set
    Attr,
    Push,
    Bignum,
    Verb,
    ZSet,  // zset
    Hash   // hash
};

const QHash<QString, RedisDataType> REDIS_TYPE_MAP = {
    {"string", RedisDataType::String}, {"none", RedisDataType::Nil},
    {"list", RedisDataType::Array},    {"set", RedisDataType::Set},
    {"zset", RedisDataType::ZSet},     {"hash", RedisDataType::Hash},
};

struct RedisReply {
    RedisReply() {}
    RedisReply(redisReply* reply) : redisReply(reply) {}
    RedisReply(const RedisReply& rvalue) {
        this->redisReply = rvalue.redisReply;
    }
    RedisReply(RedisReply& rvalue) { this->redisReply = rvalue.redisReply; }
    ~RedisReply() {}
    RedisReply& operator=(const RedisReply& rvalue) {
        this->redisReply = rvalue.redisReply;
        return *this;
    }
    RedisReply& operator=(redisReply* rvalue) {
        this->redisReply = rvalue;
        return *this;
    }
    RedisDataType getType() const {
        return (RedisDataType)this->redisReply->type;
    }
    QList<QVariant> getData() const {
        QList<QVariant> ret;
        if (this->redisReply != nullptr) {
            quint64 size = this->redisReply->elements;
            switch (this->getType()) {
                case RedisDataType::Nil:
                    break;
                case RedisDataType::String:
                case RedisDataType::Status:
                case RedisDataType::Error:
                    ret.push_back(QString(this->redisReply->str));
                    break;
                case RedisDataType::Array:
                    for (quint64 i = 0; i < size; ++i) {
                        ret.push_back(
                            RedisReply(this->redisReply->element[i]).getData());
                    }
                    break;
                case RedisDataType::Integer:
                    ret.push_back(this->redisReply->integer);
                    break;
                default:
                    break;
            }
        }
        return ret;
    }
    void dispose() {
        if (this->redisReply != nullptr) {
            freeReplyObject(this->redisReply);
            this->redisReply = nullptr;
        }
    }
    redisReply* redisReply = nullptr;
};

class RedisController {
   public:
    RedisController();
    RedisController(QString host, quint16 port = 6379, QString user = "",
                    QString pass = "");
    ~RedisController();

    void setHost(QString host, quint16 port = 6379);
    void setAuth(QString user, QString pass);

    bool getConnected();

    void connect();
    void disconnect();

    RedisReply runredis(QString cmd);
    bool ping();
    QVariant get(QString key);
    bool set(QString key, QVariant value, qint64 expire = -1);
    bool select(quint16 db);
    qint64 dbsize();
    bool flushdb();
    bool flushall();
    QList<QString> keys(QString match = "*");
    bool exists(QString key);
    quint64 del(QList<QString> keys);
    bool move(QString key, quint16 db);
    RedisDataType type(QString key);
    bool expire(QString key, qint64 seconds);
    bool pexpire(QString key, qint64 seconds);
    qint64 ttl(QString key);
    qint64 pttl(QString key);
    bool persist(QString key);
    QPair<qint64, QList<QString>> scan(QString match = "", qint64 count = -1,
                                       qint64 cursor = 0);
    bool setnx(QString key, QVariant value);
    QVariant getset(QString key, QVariant value);
    qint64 append(QString key, QVariant value);
    qint64 incr(QString key);
    qint64 decr(QString key);
    qint64 incrby(QString key, qint64 value);
    qint64 decrby(QString key, qint64 value);
    float incrbyfloat(QString key, float value);
    qint64 strlen(QString key);
    QString getrange(QString key, qint64 start, qint64 end);
    qint64 setrange(QString key, qint64 offset, QString value);
    bool hset(QString key, QString hkey, QVariant hvalue);
    bool hmset(QString key,
               QHash<QString, QVariant> data);  // for lower version compabality
    QVariant hget(QString key, QString hkey);
    QList<QVariant> hmget(QString key, QList<QString> hkeys);
    QHash<QString, QVariant> hgetall(QString key);
    qint64 lpush(QString key, QList<QVariant> values);
    qint64 rpush(QString key, QList<QVariant> values);
    qint64 llen(QString key);
    QVariant lindex(QString key, qint64 index);
    bool lset(QString key, qint64 index, QVariant value);
    QList<QVariant> lrange(QString key, qint64 start, qint64 end);
    QList<QVariant> lpop(QString key, qint64 count);
    QList<QVariant> rpop(QString key, qint64 count);
    QString xadd(QString stream, QHash<QString, QVariant> data,
                 QString key = "*");
    QList<QPair<QString, QHash<QString, QVariant>>> xread(QString stream,
                                                          qint64 block = -1,
                                                          qint64 count = -1);
    QList<QPair<QString, QHash<QString, QVariant>>> xrange(QString key,
                                                           QString start = "-",
                                                           QString end = "+");
    qint64 xdel(QString key, QList<QString> ids);

   private:
    QList<QVariant> data_from_reply(RedisReply& reply);
    QList<QVariant> data_from_reply(RedisReply reply);

    redisContext* database = nullptr;
    QString host;
    quint16 port = 6379;
    QString user = "";
    QString pass = "";

    QMutex lock;
};
}  // namespace JDB

#endif