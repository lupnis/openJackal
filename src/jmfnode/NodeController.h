/*
 * file name:       NodeController.h
 * created at:      2024/02/14
 * last modified:   2024/02/19
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef NODE_CONTROLLER_H
#define NODE_CONTROLLER_H

#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QObject>

#include "Configurations.h"
#include "Database.h"
#include "TaskRunner.h"

namespace JackalMFN {

enum NodeActions {
    ShutdownNode,
    StopRunners,
    StartRunners,
    Report
};

class NodeController : public QObject {
    Q_OBJECT
   public:
    NodeController(
        JConfigs::JsonConfigHandler configs = JConfigs::JsonConfigHandler());
    ~NodeController();

    void startNode();
    void stopNode();

   private:
    void action_stop_runners();
    void action_start_runners();
    void action_report();

   private slots:
    void onNodeTimerTimeout();

   private:
    QTimer* node_timer = nullptr;
    JConfigs::JsonConfigHandler configs;
    QString node_id;
    JLogs::Logger logger;
    QList<TaskRunner*> task_runners;
    JDB::MySQLODBCController mysql_controller;
    JDB::RedisController redis_controller;
};

}  // namespace JackalMFN

#endif