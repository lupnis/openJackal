/*
 * file name:       Database.cpp
 * created at:      2024/01/18
 * last modified:   2024/01/20
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "Database.h"

namespace JDB {

MySQLODBCController::MySQLODBCController() {
    this->database = QSqlDatabase::addDatabase("QODBC3");
}

MySQLODBCController::MySQLODBCController(QString host, quint16 port,
                                         QString user, QString pass,
                                         QString default_schema,
                                         QString default_table)
    : selected_table(default_table) {
    this->database = QSqlDatabase::addDatabase("QODBC3");
    this->database.setHostName(host);
    this->database.setPort(port);
    this->database.setUserName(user);
    this->database.setPassword(pass);
    this->database.setDatabaseName(default_schema);
}

MySQLODBCController::~MySQLODBCController() { this->disconnect(); }

void MySQLODBCController::setHost(QString host, quint16 port) {
    this->database.setHostName(host);
    this->database.setPort(port);
}

void MySQLODBCController::setAuth(QString user, QString pass) {
    this->database.setUserName(user);
    this->database.setPassword(pass);
}

void MySQLODBCController::setDefaultSchema(QString default_schema) {
    this->database.setDatabaseName(default_schema);
}

void MySQLODBCController::setTable(QString table) {
    this->selected_table = table;
}

QString MySQLODBCController::getLocation() {
    return QString("%1%2")
        .arg((this->database.databaseName().isEmpty()
                  ? ""
                  : QString("`%1`").arg(this->database.databaseName())))
        .arg((this->selected_table.isEmpty()
                  ? ""
                  : QString(".`%1`").arg(this->selected_table)));
}

bool MySQLODBCController::getConnected() { return this->database.isOpen(); }

void MySQLODBCController::connect() { this->database.open(); }

void MySQLODBCController::disconnect() { this->database.close(); }

bool MySQLODBCController::rollback() { return this->database.rollback(); }

QString MySQLODBCController::make_match_str(
    QList<QHash<QString, QVariant>> match_query) {
    QString sql_cmd;
    if (match_query.size()) {
        sql_cmd += " WHERE (";
        for (int i = 0; i < match_query.size(); i++) {
            if (i) {
                sql_cmd += " or ";
            }
            sql_cmd += "(";
            for (QHash<QString, QVariant>::iterator it = match_query[i].begin();
                 it != match_query[i].end(); ++it) {
                if (it != match_query[i].begin()) {
                    sql_cmd += " and ";
                }
                sql_cmd += QString("`%1`%2").arg(it.key()).arg(
                    it.value().toString().replace("\'", "\\'"));
            }
            sql_cmd += ")";
        }
        sql_cmd += ")";
    }
    return sql_cmd;
}

QString MySQLODBCController::make_limit_str(qint32 limit_start,
                                            qint32 limit_size) {
    if (limit_start >= 0 && limit_size >= 0) {
        return QString(" LIMIT %1,%2").arg(limit_start).arg(limit_size);
    }
    return "";
}

QPair<int, QList<QList<QVariant>>> MySQLODBCController::runsql(
    QString sql_cmd) {
    if (!this->lock.tryLock()) {
        throw "cannot get lock";
    }
    QList<QList<QVariant>> ret;
    int affected = 0;
    QSqlQuery query;
    if (this->getConnected() && query.exec(sql_cmd)) {
        int cols = query.record().count();
        affected = query.numRowsAffected();
        while (query.next()) {
            QList<QVariant> rtmplist;
            for (int j = 0; j < cols; j++) {
                rtmplist.push_back(query.value(j));
            }
            ret.push_back(rtmplist);
        }
    }
    this->lock.unlock();
    return {affected, ret};
}

QPair<int, QList<QList<QVariant>>> MySQLODBCController::select(
    QList<QHash<QString, QVariant>> match_query, qint32 limit_start,
    qint32 limit_size) {
    QString sql_cmd = QString("SELECT * FROM `%1`").arg(this->selected_table);
    sql_cmd += this->make_match_str(match_query);
    sql_cmd += this->make_limit_str(limit_start, limit_size);
    return this->runsql(sql_cmd);
}

QPair<int, QList<QList<QVariant>>> MySQLODBCController::insert(
    QList<QList<QVariant>> records, QList<QString> columns) {
    if (records.empty()) return {0, QList<QList<QVariant>>()};
    QString sql_cmd = QString("INSERT INTO `%1`").arg(this->selected_table);
    if (!columns.empty()) {
        sql_cmd += " (";
        for (int i = 0; i < columns.size(); ++i) {
            if (i) {
                sql_cmd += ", ";
            }
            sql_cmd += QString("`%1`").arg(columns[i]);
        }
        sql_cmd += ")";
    }
    sql_cmd += " VALUES ";
    for (int i = 0; i < records.size(); ++i) {
        if (i) {
            sql_cmd += ", ";
        }
        sql_cmd += "(";
        for (int j = 0; j < records[i].size(); ++j) {
            if (j) {
                sql_cmd += ", ";
            }
            sql_cmd += QString("'%1'").arg(
                records[i][j].toString().replace("\'", "\\'"));
        }
        sql_cmd += ")";
    }
    return this->runsql(sql_cmd);
}

QPair<int, QList<QList<QVariant>>> MySQLODBCController::remove(
    QList<QHash<QString, QVariant>> match_query) {
    QString sql_cmd = QString("DELETE FROM `%1`").arg(this->selected_table);
    sql_cmd += this->make_match_str(match_query);
    return this->runsql(sql_cmd);
}

QPair<int, QList<QList<QVariant>>> MySQLODBCController::modify(
    QList<QHash<QString, QVariant>> match_query,
    QHash<QString, QVariant> new_value, qint32 limit_start, qint32 limit_size) {
    if (match_query.isEmpty() || new_value.isEmpty())
        return {0, QList<QList<QVariant>>()};
    QString sql_cmd = QString("UPDATE `%1` SET ").arg(this->selected_table);
    for (QHash<QString, QVariant>::iterator it = new_value.begin();
         it != new_value.end(); ++it) {
        if (it != new_value.begin()) {
            sql_cmd += ", ";
        }
        sql_cmd += QString("`%1`='%2'")
                       .arg(it.key())
                       .arg(it.value().toString().replace("\'", "\\'"));
    }
    sql_cmd += this->make_match_str(match_query);
    sql_cmd += this->make_limit_str(limit_start, limit_size);
    return this->runsql(sql_cmd);
}

RedisController::RedisController() {}

RedisController::RedisController(QString host, quint16 port, QString user,
                                 QString pass)
    : host(host), port(port), user(user), pass(pass) {}

RedisController::~RedisController() { this->disconnect(); }

void RedisController::setHost(QString host, quint16 port) {
    this->host = host;
    this->port = port;
}

void RedisController::setAuth(QString user, QString pass) {
    this->user = user;
    this->pass = pass;
}

bool RedisController::getConnected() { return this->database != nullptr; }

void RedisController::connect() {
    if (this->database == nullptr) {
        this->database =
            redisConnect(this->host.toStdString().c_str(), this->port);
        if (this->database == nullptr || this->database->err != 0) {
            this->disconnect();
            return;
        }
        if (!this->pass.isEmpty()) {
            if (this->data_from_reply(this->runredis(
                    QString("auth %1 %2").arg(this->user).arg(this->pass)))[0]
                    .toString() != "OK") {
                this->disconnect();
                return;
            }
        }
    } else {
        this->disconnect();
        this->connect();
    }
}

void RedisController::disconnect() {
    if (this->database != nullptr) {
        redisFree(this->database);
        this->database = nullptr;
    }
}

RedisReply RedisController::runredis(QString cmd) {
    if (!this->lock.tryLock()) {
        throw "cannot get lock";
    }
    RedisReply reply;
    if (this->getConnected()) {
        reply = (redisReply*)redisCommand(this->database,
                                          cmd.toStdString().c_str());
    }
    this->lock.unlock();
    return reply;
}

bool RedisController::ping() {
    return this->data_from_reply(this->runredis("ping"))[0].toString() ==
           "PONG";
}

QVariant RedisController::get(QString key) {
    QList<QVariant> data =
        this->data_from_reply(this->runredis(QString("get %1").arg(key)));
    return data.size() == 0 ? QVariant() : data[0];
}

bool RedisController::set(QString key, QVariant value, qint64 expire) {
    return this->data_from_reply(this->runredis(
               QString("set %1 %2 %3")
                   .arg(key)
                   .arg(value.toString())
                   .arg(expire > 0 ? (" ex " + QString::number(expire))
                                   : "")))[0]
               .toString() == "OK";
}

bool RedisController::select(quint16 db) {
    return this->data_from_reply(
                   this->runredis(QString("select %1").arg(db)))[0]
               .toString() == "OK";
}

qint64 RedisController::dbsize() {
    return this->data_from_reply(this->runredis(QString("dbsize")))[0].toInt();
}

bool RedisController::flushdb() {
    return this->data_from_reply(this->runredis(QString("flushdb")))[0]
               .toString() == "OK";
}

bool RedisController::flushall() {
    return this->data_from_reply(this->runredis(QString("flushall")))[0]
               .toString() == "OK";
}

QList<QString> RedisController::keys(QString match) {
    QList<QVariant> data =
        this->data_from_reply(this->runredis(QString("keys %1").arg(match)));
    QList<QString> ret;
    if (!data.empty() && data[0].type() == QVariant::List) {
        for (QVariant& item : data[0].toList()) {
            ret.push_back(item.toString());
        }
    }
    return ret;
}

bool RedisController::exists(QString key) {
    return this
        ->data_from_reply(this->runredis(QString("exists %1").arg(key)))[0]
        .toInt();
}

quint64 RedisController::del(QList<QString> keys) {
    return this
        ->data_from_reply(
            this->runredis(QString("del %1").arg(keys.join(' '))))[0]
        .toInt();
}

bool RedisController::move(QString key, quint16 db) {
    return this->data_from_reply(
                   this->runredis(QString("move %1 %2").arg(key).arg(db)))[0]
               .toString() == "1";
}

RedisDataType RedisController::type(QString key) {
    QString data =
        this->data_from_reply(this->runredis(QString("type %1").arg(key)))[0]
            .toString();
    return REDIS_TYPE_MAP.contains(data) ? REDIS_TYPE_MAP[data]
                                         : RedisDataType::Nil;
}

bool RedisController::expire(QString key, qint64 seconds) {
    return this->data_from_reply(this->runredis(
               QString("expire %1 %2").arg(key).arg(seconds)))[0]
               .toString() == "1";
}

bool RedisController::pexpire(QString key, qint64 seconds) {
    return this->data_from_reply(this->runredis(
               QString("pexpire %1 %2").arg(key).arg(seconds)))[0]
               .toString() == "1";
}

qint64 RedisController::ttl(QString key) {
    return this->data_from_reply(this->runredis(QString("ttl %1").arg(key)))[0]
        .toInt();
}

qint64 RedisController::pttl(QString key) {
    return this->data_from_reply(this->runredis(QString("pttl %1").arg(key)))[0]
        .toInt();
}

bool RedisController::persist(QString key) {
    return this
        ->data_from_reply(this->runredis(QString("persist %1").arg(key)))[0]
        .toInt();
}

QPair<qint64, QList<QString>> RedisController::scan(QString match, qint64 count,
                                                    qint64 cursor) {
    QList<QVariant> data = this->data_from_reply(this->runredis(
        QString("scan %1 %2 %3")
            .arg(cursor)
            .arg(match.isEmpty() ? "" : ("match " + match))
            .arg(count > 0 ? QString("count %1").arg(count) : "")));
    QPair<qint64, QList<QString>> ret;
    if (data.size() > 1) {
        ret.first = data[0].toList()[0].toInt();
        for (QVariant& item : data[1].toList()) {
            ret.second.push_back(item.toList()[0].toString());
        }
    }
    return ret;
}

bool RedisController::setnx(QString key, QVariant value) {
    return this
        ->data_from_reply(this->runredis(
            QString("setnx %1 %2").arg(key).arg(value.toString())))[0]
        .toInt();
}

QVariant RedisController::getset(QString key, QVariant value) {
    QList<QVariant> data = this->data_from_reply(
        this->runredis(QString("getset %1 %2").arg(key).arg(value.toString())));
    return (data.size() == 0 ? QVariant() : data[0]);
}

qint64 RedisController::append(QString key, QVariant value) {
    return this
        ->data_from_reply(this->runredis(
            QString("append %1 %2").arg(key).arg(value.toString())))[0]
        .toInt();
}

qint64 RedisController::incr(QString key) {
    return this->data_from_reply(this->runredis(QString("incr %1").arg(key)))[0]
        .toInt();
}

qint64 RedisController::decr(QString key) {
    return this->data_from_reply(this->runredis(QString("decr %1").arg(key)))[0]
        .toInt();
}

qint64 RedisController::incrby(QString key, qint64 value) {
    return this
        ->data_from_reply(
            this->runredis(QString("incrby %1 %2").arg(key).arg(value)))[0]
        .toInt();
}

qint64 RedisController::decrby(QString key, qint64 value) {
    return this
        ->data_from_reply(
            this->runredis(QString("decrby %1 %2").arg(key).arg(value)))[0]
        .toInt();
}

float RedisController::incrbyfloat(QString key, float value) {
    return this
        ->data_from_reply(
            this->runredis(QString("incrbyfloat %1 %2").arg(key).arg(value)))[0]
        .toFloat();
}

qint64 RedisController::strlen(QString key) {
    return this
        ->data_from_reply(this->runredis(QString("strlen %1").arg(key)))[0]
        .toInt();
}

QString RedisController::getrange(QString key, qint64 start, qint64 end) {
    return this
        ->data_from_reply(this->runredis(
            QString("getrange %1 %2 %3").arg(key).arg(start).arg(end)))[0]
        .toString();
}

qint64 RedisController::setrange(QString key, qint64 offset, QString value) {
    return this
        ->data_from_reply(this->runredis(
            QString("setrange %1 %2 %3").arg(key).arg(offset).arg(value)))[0]
        .toInt();
}

bool RedisController::hset(QString key, QString hkey, QVariant hvalue) {
    return this
        ->data_from_reply(this->runredis(QString("hset %1 %2 %3")
                                             .arg(key)
                                             .arg(hkey)
                                             .arg(hvalue.toString())))[0]
        .toInt();
}

bool RedisController::hmset(QString key, QHash<QString, QVariant> data) {
    QString cmd = QString("hmset %1").arg(key);
    for (QHash<QString, QVariant>::iterator it = data.begin(); it != data.end();
         ++it) {
        cmd += QString(" %1 %2").arg(it.key()).arg(it.value().toString());
    }
    return this->data_from_reply(this->runredis(cmd))[0].toString() == "OK";
}

QVariant RedisController::hget(QString key, QString hkey) {
    return this->data_from_reply(
        this->runredis(QString("hget %1 %2").arg(key).arg(hkey)))[0];
}

QList<QVariant> RedisController::hmget(QString key, QList<QString> hkeys) {
    QList<QVariant> data = this->data_from_reply(
        this->runredis(QString("hmget %1 %2").arg(key).arg(hkeys.join(' '))));
    QList<QVariant> ret;
    for (QVariant& item : data) {
        if (item.type() == QVariant::List) {
            ret.push_back(item.toList()[0]);
        }
    }
    return ret;
}

QHash<QString, QVariant> RedisController::hgetall(QString key) {
    QList<QVariant> data =
        this->data_from_reply(this->runredis(QString("hgetall %1").arg(key)));
    QHash<QString, QVariant> ret;
    for (int i = 1; i < data.size(); i += 2) {
        ret[data[i - 1].toList()[0].toString()] = data[i].toList()[0];
    }
    return ret;
}

qint64 RedisController::lpush(QString key, QList<QVariant> values) {
    QString cmd = QString("lpush %1").arg(key);
    for (QVariant& item : values) {
        cmd += QString(" %1").arg(item.toString());
    }
    return this->data_from_reply(this->runredis(cmd))[0].toInt();
}

qint64 RedisController::rpush(QString key, QList<QVariant> values) {
    QString cmd = QString("rpush %1").arg(key);
    for (QVariant& item : values) {
        cmd += QString(" %1").arg(item.toString());
    }
    return this->data_from_reply(this->runredis(cmd))[0].toInt();
}

qint64 RedisController::llen(QString key) {
    return this->data_from_reply(this->runredis(QString("llen %1").arg(key)))[0]
        .toInt();
}

QVariant RedisController::lindex(QString key, qint64 index) {
    return this->data_from_reply(
        this->runredis(QString("lindex %1 %2").arg(key).arg(index)))[0];
}

bool RedisController::lset(QString key, qint64 index, QVariant value) {
    return this->data_from_reply(this->runredis(QString("lset %1 %2 %3")
                                                    .arg(key)
                                                    .arg(index)
                                                    .arg(value.toString())))[0]
               .toString() == "OK";
}

QList<QVariant> RedisController::lrange(QString key, qint64 start, qint64 end) {
    QList<QVariant> data = this->data_from_reply(this->runredis(
        QString("lrange %1 %2 %3").arg(key).arg(start).arg(end)));
    QList<QVariant> ret;
    for (QVariant& item : data) {
        if (item.type() == QVariant::List) {
            ret.push_back(item.toList()[0]);
        }
    }
    return ret;
}

QList<QVariant> RedisController::lpop(QString key, qint64 count) {
    QList<QVariant> data = this->data_from_reply(
        this->runredis(QString("lpop %1 %2").arg(key).arg(count)));
    QList<QVariant> ret;
    for (QVariant& item : data) {
        if (item.type() == QVariant::List) {
            ret.push_back(item.toList()[0]);
        }
    }
    return ret;
}

QList<QVariant> RedisController::rpop(QString key, qint64 count) {
    QList<QVariant> data = this->data_from_reply(
        this->runredis(QString("rpop %1 %2").arg(key).arg(count)));
    QList<QVariant> ret;
    for (QVariant& item : data) {
        if (item.type() == QVariant::List) {
            ret.push_back(item.toList()[0]);
        }
    }
    return ret;
}

QString RedisController::xadd(QString stream, QHash<QString, QVariant> data,
                              QString key) {
    QString cmd = QString("xadd %1 %2").arg(stream).arg(key);
    for (QHash<QString, QVariant>::iterator it = data.begin(); it != data.end();
         ++it) {
        cmd += QString(" %1 %2").arg(it.key()).arg(it.value().toString());
    }
    return this->data_from_reply(this->runredis(cmd))[0].toString();
}

QList<QPair<QString, QHash<QString, QVariant>>> RedisController::xread(
    QString stream, qint64 block, qint64 count) {
    QList<QVariant> data = this->data_from_reply(
        this->runredis(QString("xread %1 %2 %3 0")
                           .arg(count > 0 ? QString("count %1").arg(count) : "")
                           .arg(block > 0 ? QString("block %1").arg(block) : "")
                           .arg(QString("streams %1").arg(stream))));
    QList<QPair<QString, QHash<QString, QVariant>>> ret;
    if (data.size() && data[0].toList().size()) {
        QList<QVariant> messages = data[0].toList()[1].toList();
        for (int i = 0; i < messages.size(); ++i) {
            QPair<QString, QHash<QString, QVariant>> message;
            message.first = messages[i].toList()[0].toList()[0].toString();
            QList<QVariant> hashes = messages[i].toList()[1].toList();
            for (int i = 1; i < hashes.size(); i += 2) {
                message.second[hashes[i - 1].toList()[0].toString()] =
                    hashes[i].toList()[0];
            }
            ret.push_back(message);
        }
    }
    return ret;
}

QList<QPair<QString, QHash<QString, QVariant>>> RedisController::xrange(
    QString key, QString start, QString end) {
    QList<QVariant> data = this->data_from_reply(this->runredis(
        QString("xrange %1 %2 %3").arg(key).arg(start).arg(end)));
    QList<QPair<QString, QHash<QString, QVariant>>> ret;
    if (data.size() && data[0].toList().size()) {
        for (int i = 0; i < data.size(); ++i) {
            QPair<QString, QHash<QString, QVariant>> message;
            message.first = data[i].toList()[0].toList()[0].toString();
            QList<QVariant> hashes = data[i].toList()[1].toList();
            for (int i = 1; i < hashes.size(); i += 2) {
                message.second[hashes[i - 1].toList()[0].toString()] =
                    hashes[i].toList()[0];
            }
            ret.push_back(message);
        }
    }
    return ret;
}

qint64 RedisController::xdel(QString key, QList<QString> ids) {
    return this
        ->data_from_reply(this->runredis(
            QString("xdel %1 %2").arg(key).arg(ids.join(' '))))[0]
        .toInt();
}

QList<QVariant> RedisController::data_from_reply(RedisReply& reply) {
    QList<QVariant> data = reply.getData();
    reply.dispose();
    return data;
}

QList<QVariant> RedisController::data_from_reply(RedisReply reply) {
    QList<QVariant> data = reply.getData();
    reply.dispose();
    return data;
}

}  // namespace JDB
