/*
 * file name:       NodeController.cpp
 * created at:      2024/02/16
 * last modified:   2024/02/22
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "NodeController.h"

namespace JackalMFN {
NodeController::NodeController(JConfigs::JsonConfigHandler configs) {
    this->logger.info("initializing node controller...", JLogs::Tag::PROGRESS,
                      true);
    this->configs = configs;
    this->configs.loadConfigs();

    this->logger.updateConfig(
        this->configs
            .get("/logging",
                 QJsonObject::fromVariantHash(JLogs::DEFAULT_LOG_CONFIG))
            .toHash());
    this->logger.info("[1/6] logger initialized.", JLogs::Tag::DONE);

    this->mysql_controller.setHost(this->configs.get("/mysql/host").toString(),
                                   this->configs.get("/mysql/port").toUInt());

    this->mysql_controller.setAuth(this->configs.get("/mysql/user").toString(),
                                   this->configs.get("/mysql/pass").toString());
    this->mysql_controller.setDefaultSchema(
        this->configs.get("/mysql/schema").toString());
    this->logger.info("[2/6] mysql connector initialized.", JLogs::Tag::DONE);

    this->redis_controller.setHost(this->configs.get("/redis/host").toString(),
                                   this->configs.get("/redis/port").toUInt());
    this->redis_controller.setAuth(this->configs.get("/redis/user").toString(),
                                   this->configs.get("/redis/pass").toString());
    this->logger.info("[3/6] redis connector initialized.", JLogs::Tag::DONE);

    quint32 task_queue_refresh_interval =
        this->configs.get("/runners/task_queue_refresh_interval").toUInt();
    quint8 task_queue_max_size =
        this->configs.get("/runners/task_queue_max_size").toUInt();
    quint8 failed_task_requeue_interval =
        this->configs.get("/runners/failed_task_requeue_interval").toUInt();
    quint8 task_max_retries =
        this->configs.get("/runners/task_max_retries").toUInt();
    quint8 task_file_slices =
        this->configs.get("/runners/task_file_slices").toUInt();
    bool store_slices_in_memory =
        this->configs.get("/runners/store_slices_in_memory").toBool();
    QString slices_storage_root_path =
        this->configs.get("/runners/slices_storage_root_path").toString();
    QString dest_storage_root_path =
        this->configs.get("/runners/dest_storage_root_path").toString();
    quint32 task_head_timeout =
        this->configs.get("/runners/task_head_timeout").toUInt();
    quint8 task_head_max_retries =
        this->configs.get("/runners/task_head_max_retries").toUInt();
    quint32 task_fetch_timeout =
        this->configs.get("/runners/task_fetch_timeout").toUInt();
    quint8 task_fetch_max_retries =
        this->configs.get("/runners/task_fetch_max_retries").toUInt();
    qint64 task_fetch_buffer_size =
        this->configs.get("/runners/task_fetch_buffer_size").toUInt();
    quint8 num_runners = this->configs.get("/node/num_runners").toUInt();
    for (int i = 0; i < num_runners; ++i) {
        this->task_runners.push_back(new TaskRunner(
            task_queue_refresh_interval, task_queue_max_size,
            failed_task_requeue_interval, task_max_retries, task_file_slices,
            store_slices_in_memory, slices_storage_root_path,
            dest_storage_root_path, task_head_timeout, task_head_max_retries,
            task_fetch_timeout, task_fetch_max_retries,
            task_fetch_buffer_size));
    }
    this->logger.info("[4/6] runners initialized.", JLogs::Tag::DONE);

    QString mac_md5;
    foreach (QNetworkInterface netInterface,
             QNetworkInterface::allInterfaces()) {
        if (!(netInterface.flags() & QNetworkInterface::IsLoopBack) &&
            (netInterface.flags() & QNetworkInterface::IsUp) &&
            (netInterface.flags() & QNetworkInterface::IsRunning)) {
            mac_md5 = QString(QCryptographicHash::hash(
                                  netInterface.hardwareAddress().toUtf8(),
                                  QCryptographicHash::Md5)
                                  .toHex());
            break;
        }
    }
    this->node_id = QString("jmfn_%1").arg(mac_md5);
    this->logger.info(JLogs::S("[5/6] ") + "node id: " +
                          JLogs::S(this->node_id, CYAN) + " generated.",
                      JLogs::Tag::DONE);

    this->node_timer = new QTimer();
    connect(this->node_timer, &QTimer::timeout, this,
            &NodeController::onNodeTimerTimeout);
    this->logger.info("[6/6] node timer initialized.", JLogs::Tag::DONE);

    this->logger.info("node controller initialized.", JLogs::Tag::DONE);
}
NodeController::~NodeController() {
    this->stopNode();
    this->node_timer->deleteLater();
    this->node_timer = nullptr;
    for (int i = 0; i < this->task_runners.size(); ++i) {
        this->task_runners[i]->deleteLater();
        this->task_runners[i] = nullptr;
    }
    this->task_runners.clear();
}

void NodeController::startNode() {
    this->logger.info("starting node controller...", JLogs::Tag::PROGRESS);
    for (int i = 0; i < this->task_runners.size(); ++i) {
        this->task_runners[i]->startRunnerLoop();
    }
    this->logger.info(JLogs::S("[1/5] ") +
                          JLogs::S(this->task_runners.size(), GREEN) +
                          " runners started.",
                      JLogs::Tag::DONE);

    this->mysql_controller.connect();
    this->logger.info("[2/5] connection request sent to mysql odbc connector.",
                      JLogs::Tag::DONE);

    this->redis_controller.connect();
    this->logger.info("[3/5] connection request sent to redis.",
                      JLogs::Tag::DONE);

    this->node_timer->setInterval(
        this->configs.get("/node/node_task_receiving_interval").toInt());
    this->node_timer->start();
    this->logger.info("[4/5] node timer started.", JLogs::Tag::DONE);

    try {
        this->redis_controller.select(this->configs.get("/redis/db").toUInt());
        this->redis_controller.rpush("jackalmfn:global:online",
                                     {this->node_id});
        this->logger.info(JLogs::S("[5/5] node id: ") +
                              JLogs::S(this->node_id, CYAN) +
                              " uploaded to redis.",
                          JLogs::Tag::DONE);
    } catch (...) {
        this->logger.info(JLogs::S("[5/5]", RED) +
                              " node id: " + JLogs::S(this->node_id, CYAN) +
                              " failed to upload to redis.",
                          JLogs::Tag::FAILED);
    }

    this->logger.info("node controller started.", JLogs::Tag::DONE);
}

void NodeController::stopNode() {
    this->logger.warn("stopping node controller...", JLogs::Tag::PROGRESS);

    this->node_timer->stop();
    this->logger.warn("[1/5] node timer stopped.", JLogs::Tag::DONE);

    this->mysql_controller.disconnect();
    this->logger.warn("[2/5] mysql odbc connector disconnected.",
                      JLogs::Tag::DONE);

    this->redis_controller.rpush("jackalmfn:global:down", {this->node_id});
    this->logger.warn(JLogs::S("[3/5] ") +
                          "node id: " + JLogs::S(this->node_id, CYAN) +
                          " added to downlist in redis.",
                      JLogs::Tag::DONE);

    this->redis_controller.disconnect();
    this->logger.warn("[4/5] redis disconnected.", JLogs::Tag::DONE);

    for (int i = 0; i < this->task_runners.size(); ++i) {
        this->task_runners[i]->stopRunnerLoop();
    }
    this->logger.warn("[5/5] runners stopped.", JLogs::Tag::DONE);
    this->logger.warn("node controller stopped.", JLogs::Tag::DONE);
}

void NodeController::action_stop_runners() {
    this->logger.info("stopping runners...", JLogs::Tag::PROGRESS);
    for (int i = 0; i < this->task_runners.size(); ++i) {
        this->task_runners[i]->stopRunnerLoop();
    }
    this->logger.info("runners stopped.", JLogs::Tag::SUCCEEDED);
}

void NodeController::action_start_runners() {
    this->logger.info("starting runners...", JLogs::Tag::PROGRESS);
    for (int i = 0; i < this->task_runners.size(); ++i) {
        bool ith_start_result = this->task_runners[i]->startRunnerLoop();
        if (!ith_start_result) {
            this->logger.warn(JLogs::S("failed to start runner ") +
                                  JLogs::S(i, GREEN) +
                                  ". perhaps it has been started?",
                              JLogs::Tag::FAILED);
        }
    }
    this->logger.info("runners started.", JLogs::Tag::SUCCEEDED);
}

void NodeController::action_report() {
    QList<bool> runners_running_status;
    QList<QList<TaskDetails>> main_queues;
    QList<QList<TaskDetails>> loop_queues;
    QList<TaskDetails> finished_tasks;
    QList<TaskDetails> failed_tasks;

    QList<TaskDetails> current_task_details;
    QList<QList<JRequests::Status>> current_fetcher_status;
    QList<QList<JRequests::Status>> current_request_status;
    QList<QList<JRequests::Result>> current_request_result;

    QList<QList<QPair<qint64, qint64>>> current_fetcher_progress;
    QList<QList<quint32>> fetchers_time_consumed;

    for (int i = 0; i < this->task_runners.size(); ++i) {
        this->logger.debug(
            JLogs::S("----------------Report of runner [") +
            JLogs::S(QString::number(i).rightJustified(2, '0'), GREEN) +
            "]----------------");

        runners_running_status.push_back(
            this->task_runners[i]->getIsRunnerRunning());
        this->logger.debug(JLogs::S("Running    : ") +
                           (runners_running_status[i]
                                ? JLogs::S("True", GREEN)
                                : JLogs::S("False", RED)));

        main_queues.push_back(this->task_runners[i]->getMainQueue());
        this->logger.debug(JLogs::S("Main Queue : ") +
                           JLogs::S(main_queues[i].size(), GREEN));

        loop_queues.push_back(this->task_runners[i]->getLoopQueue());
        this->logger.debug(JLogs::S("Loop Queue : ") +
                           JLogs::S(loop_queues[i].size(), GREEN));

        finished_tasks.append(this->task_runners[i]->getFinishedTasks());
        this->logger.debug(
            JLogs::S("Finished   : ") +
            JLogs::S(this->task_runners[i]->getFinishedTasks().size(), GREEN));
        this->task_runners[i]->dropFinishedTaskRecords();

        failed_tasks.append(this->task_runners[i]->getFailedTasks());
        this->logger.debug(
            JLogs::S("Failed     : ") +
            JLogs::S(this->task_runners[i]->getFailedTasks().size(), GREEN));
        this->task_runners[i]->dropFailedTaskRecords();

        current_task_details.push_back(
            this->task_runners[i]->getCurrentTaskDetails());
        this->logger.debug(
            JLogs::S("Current    : ") +
            (current_task_details[i].hasTask()
                 ? (JLogs::S(current_task_details[i].urlPath, CYAN) + " -> " +
                    JLogs::S(current_task_details[i].storagePath, MAGENTA))
                 : (JLogs::S("No Task", YELLOW))));
        this->logger.debug(
            JLogs::S("           : ") +
            JLogs::S(current_task_details[i].numFetchers, GREEN) +
            " fetchers, failed " +
            JLogs::S(current_task_details[i].failedCount, GREEN) + " times");
        this->logger.debug(
            JLogs::S("           : ") + "at stage " +
            JLogs::S(current_task_details[i].currentStage, GREEN));

        current_fetcher_status.push_back(
            this->task_runners[i]->getCurrentFetcherStatus());
        current_request_status.push_back(
            this->task_runners[i]->getCurrentRequestStatus());
        current_request_result.push_back(
            this->task_runners[i]->getCurrentRequestResult());

        current_fetcher_progress.push_back(
            this->task_runners[i]->getCurrentFetcherProgress());
        this->logger.debug(
            JLogs::S("Progress   : ") + JLogs::S("Heading (") +
            JLogs::S(current_fetcher_progress[i][0].first, GREEN) + "/" +
            JLogs::S(current_fetcher_progress[i][0].second, GREEN) + ")");
        for (int j = 1; j < current_fetcher_progress[i].size(); ++j) {
            this->logger.debug(
                JLogs::S("           : ") + JLogs::S("Slice [") +
                JLogs::S(j, GREEN) + "] (" +
                JLogs::S(current_fetcher_progress[i][j].first, GREEN) + "/" +
                JLogs::S(current_fetcher_progress[i][j].second, GREEN) + ")");
        }

        fetchers_time_consumed.push_back(
            this->task_runners[i]->getFetchersTimeConsumed());
        this->logger.debug(JLogs::S("Est Time(s): ") + JLogs::S("Heading (") +
                           JLogs::S(fetchers_time_consumed[i][0], GREEN) + ")");

        for (int j = 1; j < current_fetcher_progress[i].size(); ++j) {
            this->logger.debug(JLogs::S("           : ") + JLogs::S("Slice [") +
                               JLogs::S(j, GREEN) + "] (" +
                               JLogs::S(fetchers_time_consumed[i][j], GREEN) +
                               ")");
        }
    }
    this->logger.debug("-----------------------------------------------------");

    this->logger.info("reporting status of node to database...",
                      JLogs::Tag::PROGRESS);

    this->mysql_controller.setTable("table_jmfn_node_reports");
    this->mysql_controller.upsert(
        {QString("'%1'").arg(this->node_id), this->task_runners.size(),
         finished_tasks.size(), failed_tasks.size()},
        {"node_id", "runners_count", "finished_tasks_count",
         "failed_tasks_count"},
        {QString("'%1'").arg(this->node_id), this->task_runners.size(),
         QString("`finished_tasks_count`+%1").arg(finished_tasks.size()),
         QString("`failed_tasks_count`+%1").arg(failed_tasks.size())});

    this->mysql_controller.setTable("table_jmfn_runners_reports");
    for (int i = 0; i < this->task_runners.size(); ++i) {
        QVariantList task_main_queue_list, task_loop_queue_list;
        for (const JackalMFN::TaskDetails task : main_queues[i]) {
            task_main_queue_list.push_back((QJsonObject)task);
        }
        for (const JackalMFN::TaskDetails task : loop_queues[i]) {
            task_loop_queue_list.push_back((QJsonObject)task);
        }
        this->mysql_controller.upsert(
            {QString("'%1'").arg(this->node_id), i,
             (quint8)runners_running_status[i],
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(task_main_queue_list).toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(task_loop_queue_list).toJson())),
             QString("'%1'").arg(current_task_details[i].mirrorName),
             QString("'%1'").arg(current_task_details[i].urlPath),
             QString("'%1'").arg(current_task_details[i].storagePath),
             (quint8)(current_task_details[i].proxyHost != "" &&
                      current_task_details[i].proxyPort != 0 &&
                      current_task_details[i].proxyType !=
                          QNetworkProxy::ProxyType::NoProxy),
             current_task_details[i].numFetchers,
             current_task_details[i].currentStage,
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_fetcher_progress[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(fetchers_time_consumed[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_fetcher_status[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_request_status[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_request_result[i]).toList())
                     .toJson()))},
            {"node_id", "runner_id", "runner_running", "tasks_main_queue",
             "tasks_loop_queue", "current_mirror_name", "current_task_url",
             "current_task_dest", "use_proxy", "fetchers_count", "runner_stage",
             "progresses", "time_consumed", "fetcher_status", "request_status",
             "request_result"},
            {QString("'%1'").arg(this->node_id), i,
             (quint8)runners_running_status[i],
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(task_main_queue_list).toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(task_loop_queue_list).toJson())),
             QString("'%1'").arg(current_task_details[i].mirrorName),
             QString("'%1'").arg(current_task_details[i].urlPath),
             QString("'%1'").arg(current_task_details[i].storagePath),
             (quint8)(current_task_details[i].proxyHost != "" &&
                      current_task_details[i].proxyPort != 0 &&
                      current_task_details[i].proxyType !=
                          QNetworkProxy::ProxyType::NoProxy),
             current_task_details[i].numFetchers,
             current_task_details[i].currentStage,
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_fetcher_progress[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(fetchers_time_consumed[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_fetcher_status[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_request_status[i]).toList())
                     .toJson())),
             QString("'%1'").arg(QString::fromUtf8(
                 QJsonDocument::fromVariant(
                     QVariant::fromValue(current_request_result[i]).toList())
                     .toJson()))});
    }

    this->mysql_controller.setTable("table_jmfn_finished_tasks");
    for (const TaskDetails& task : finished_tasks) {
        this->mysql_controller.upsert({QString("'%1'").arg(this->node_id),
                                       QString("'%1'").arg(task.mirrorName),
                                       QString("'%1'").arg(task.storagePath)},
                                      {"node_id", "mirror_name", "task_path"},
                                      {QString("'%1'").arg(this->node_id),
                                       QString("'%1'").arg(task.mirrorName),
                                       QString("'%1'").arg(task.storagePath)});
    }

    this->mysql_controller.setTable("table_jmfn_failed_tasks");
    for (const TaskDetails& task : failed_tasks) {
        this->mysql_controller.upsert({QString("'%1'").arg(this->node_id),
                                       QString("'%1'").arg(task.mirrorName),
                                       QString("'%1'").arg(task.storagePath)},
                                      {"node_id", "mirror_name", "task_path"},
                                      {QString("'%1'").arg(this->node_id),
                                       QString("'%1'").arg(task.mirrorName),
                                       QString("'%1'").arg(task.storagePath)});
    }
    this->logger.info("reports uploaded.", JLogs::Tag::SUCCEEDED);
}

void NodeController::onNodeTimerTimeout() {
    if (!this->mysql_controller.getConnected()) {
        this->logger.warn("failed to connect to mysql server, retrying...",
                          JLogs::Tag::FAILED);
        this->mysql_controller.connect();
    }
    if (!this->redis_controller.getConnected()) {
        this->logger.warn("failed to connect to redis server, retrying...",
                          JLogs::Tag::FAILED);
        this->redis_controller.connect();
        this->redis_controller.select(this->configs.get("/redis/db").toUInt());
        this->redis_controller.rpush("jackalmfn:global:online",
                                     {this->node_id});
        this->logger.info(JLogs::S("node id: ") +
                              JLogs::S(this->node_id, CYAN) +
                              " uploading to redis...",
                          JLogs::Tag::PROGRESS);
    }

    QString node_prefix_fmt = QString("jackalmfn:%1:%2").arg(this->node_id);
    // receive commands
    QVariant action_cmd =
        this->redis_controller.get(node_prefix_fmt.arg("action"));
    this->redis_controller.del({node_prefix_fmt.arg("action")});

    bool converted;
    NodeActions action_index = (NodeActions)action_cmd.toUInt(&converted);
    if (converted) {
        switch (action_index) {
            case NodeActions::ShutdownNode:
                this->logger.critical("shutdown command received, quitting...",
                                      JLogs::Tag::PROGRESS);
                QCoreApplication::quit();
                break;
            case NodeActions::StopRunners:
                this->action_stop_runners();
                break;
            case NodeActions::StartRunners:
                this->action_start_runners();
                break;
            case NodeActions::Report:
                this->action_report();
                break;
            default:
                this->logger.debug("invalid action received, skipped.",
                                   JLogs::Tag::CANCELED);
        }
    } else {
        this->logger.debug("no action received, skipped.",
                           JLogs::Tag::CANCELED);
    }

    QList<QPair<QString, QHash<QString, QVariant>>> tasks =
        this->redis_controller.xread(node_prefix_fmt.arg("squeue"), -1, 10);
    if (tasks.isEmpty()) {
        this->logger.debug("no sync task received, skipped.",
                           JLogs::Tag::CANCELED);
    } else {
        this->logger.info(
            JLogs::S(tasks.size(), GREEN) + " tasks received, allocating...",
            JLogs::Tag::PROGRESS);

        int index_mintask_runner = 0, min_task_load = 0x3f3f3f3f;
        for (int i = 0; i < this->task_runners.size(); ++i) {
            int total_tasks = this->task_runners[i]->getMainQueue().size() +
                              this->task_runners[i]->getLoopQueue().size();
            if (total_tasks < min_task_load) {
                min_task_load = total_tasks;
                index_mintask_runner = i;
            }
        }
        this->logger.info(JLogs::S("arranging tasks to runner ") +
                              JLogs::S(index_mintask_runner, GREEN) + "...",
                          JLogs::Tag::PROGRESS);
        int arranged_task_cnt = 0;
        for (int i = 0; i < tasks.size(); ++i) {
            QString sid = tasks[i].first;
            QString mirror_name = tasks[i].second["mname"].toString();
            QString file_url = tasks[i].second["furl"].toString();
            QString dest_path = tasks[i].second["fpath"].toString();
            QString proxy_host = tasks[i].second["phost"].toString();
            quint16 proxy_port = tasks[i].second["pport"].toUInt();
            QNetworkProxy::ProxyType proxy_type =
                (QNetworkProxy::ProxyType)tasks[i].second["ptype"].toUInt();
            if (mirror_name == "" || file_url == "" || dest_path == "")
                continue;

            if (this->task_runners[index_mintask_runner]->addTaskToQueue(
                    mirror_name, file_url, dest_path, proxy_host, proxy_port,
                    proxy_type)) {
                this->redis_controller.xdel(node_prefix_fmt.arg("squeue"),
                                            {sid});
                arranged_task_cnt++;

            } else {
                this->logger.warn(
                    JLogs::S("failed to add task to queue at position ") +
                        JLogs::S(i, GREEN) + ", for the queue is full.",
                    JLogs::Tag::FAILED);
                break;
            }
        }
        this->logger.info(JLogs::S(arranged_task_cnt, GREEN) + " of " +
                              JLogs::S(tasks.size(), GREEN) +
                              " tasks arranged to runner " +
                              JLogs::S(index_mintask_runner, GREEN) + ".",
                          JLogs::Tag::SUCCEEDED);
    }
}

}  // namespace JackalMFN
