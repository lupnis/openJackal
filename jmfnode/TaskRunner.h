/*
 * file name:       TaskRunner.h
 * created at:      2024/02/01
 * last modified:   2024/02/14
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <QCryptographicHash>
#include <QTextStream>
#include <QObject>

#include "FileFetcher.h"
#include "HeaderFetcher.h"

namespace JackalMFN {
enum TaskStage {
    Init,
    Heading,
    HeadingInProgress,
    DistributingFetchers,
    FetchingUrl,
    FetchingInProgress,
    MergingSlices,
    MergingInProgress,
    CleaningCache,
    CleaningInProgress,
    TaskFinished
};

struct TaskDetails {
    QString mirrorName = "";
    QString urlPath = "";
    QString storagePath = "";
    QString proxyHost = "";
    quint16 proxyPort = 0;
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::ProxyType::HttpProxy;
    quint8 numFetchers = 0;
    quint8 failedCount = 0;
    TaskStage currentStage = TaskStage::Init;

    TaskDetails(QString mirror_name = "", QString url_path = "",
                QString storage_path = "", QString proxy_host = "",
                quint16 proxy_port = 0,
                QNetworkProxy::ProxyType proxy_type =
                    QNetworkProxy::ProxyType::HttpProxy) {
        this->mirrorName = mirror_name;
        this->urlPath = url_path;
        this->storagePath = storage_path;
        this->proxyHost = proxy_host;
        this->proxyPort = proxy_port;
        this->proxyType = proxy_type;
    }
    TaskDetails(const TaskDetails& rvalue) {
        this->mirrorName = rvalue.mirrorName;
        this->urlPath = rvalue.urlPath;
        this->storagePath = rvalue.storagePath;
        this->proxyHost = rvalue.proxyHost;
        this->proxyPort = rvalue.proxyPort;
        this->proxyType = rvalue.proxyType;
        this->failedCount = rvalue.failedCount;
    }

    TaskDetails& operator=(const TaskDetails& rvalue) {
        this->mirrorName = rvalue.mirrorName;
        this->urlPath = rvalue.urlPath;
        this->storagePath = rvalue.storagePath;
        this->proxyHost = rvalue.proxyHost;
        this->proxyPort = rvalue.proxyPort;
        this->proxyType = rvalue.proxyType;
        this->failedCount = rvalue.failedCount;
        return *this;
    }
    bool hasTask() { return this->mirrorName.size() && this->urlPath.size(); }
};

class TaskRunner : public QObject {
    Q_OBJECT
   public:
    TaskRunner(quint32 task_queue_refresh_interval = 1000,
               quint8 task_queue_max_size = 64,
               quint8 failed_task_requeue_interval = 2,
               quint8 task_max_retries = 5, quint8 task_file_slices = 8,
               bool store_slices_in_memory = true,
               QString slices_storage_root_path = "",
               QString dest_storage_root_path = "",
               quint32 task_head_timeout = 60, quint8 task_head_max_retries = 8,
               quint32 task_fetch_timeout = 600,
               quint8 task_fetch_max_retries = 8,
               qint64 task_fetch_buffer_size = 104857500);
    ~TaskRunner();
    TaskRunner& operator=(const TaskRunner&){return *this;}

    bool updateConfigurations(
        quint32 task_queue_refresh_interval = 1000,
        quint8 task_queue_max_size = 64,
        quint8 failed_task_requeue_interval = 2, quint8 task_max_retries = 5,
        quint8 task_file_slices = 8, bool store_slices_in_memory = true,
        QString slices_storage_root_path = "",
        QString dest_storage_root_path = "", quint32 task_head_timeout = 60,
        quint8 task_head_max_retries = 8, quint32 task_fetch_timeout = 600,
        quint8 task_fetch_max_retries = 8,
        qint64 task_fetch_buffer_size = 104857500);

    void addTaskToQueue(TaskDetails task, bool add_to_main_queue = true);
    void addTaskToQueue(QString mirror_name, QString url_path,
                        QString storage_path, QString proxy_host = "",
                        quint16 proxy_port = 0,
                        QNetworkProxy::ProxyType proxy_type =
                            QNetworkProxy::ProxyType::HttpProxy,
                        bool add_to_main_queue = true);

    QList<TaskDetails> getMainQueue() const;
    QList<TaskDetails> getLoopQueue() const;
    QList<TaskDetails> getFinishedTasks() const;
    TaskDetails getTaskDetails(QString mirror_name, QString url_path) const;
    bool dropTask(QString mirror_name, QString url_path);
    void dropMainQueue();
    void dropLoopQueue();
    void dropAllQueues();
    void dropFinishedTaskRecords();
    void swapQueues();
    void mergeQueues();
    void resetFaultsCounter();
    TaskDetails getNextTask();
    bool setAsNextTask(QString mirror_name, QString url_path);

    QList<JRequests::Status> getCurrentFetcherStatus() const;
    QList<JRequests::Status> getCurrentRequestStatus() const;
    QList<JRequests::Result> getCurrentRequestResult() const;
    QList<QPair<qint64, qint64>> getCurrentFetcherProgress() const;
    QList<quint32> getFetchersTimeConsumed() const;
    TaskStage getCurrentTaskStage() const;

    void dropCurrentRunningTask(bool add_to_loop_queue = false);
    bool startRunnerLoop();
    void stopRunnerLoop();
    void terminateRunnerLoop();

   signals:
    void errorsEmitted();

   private slots:
    void onErrorsEmitted();
    void onLoopTimerTimeout();

   private:
    void on_stage_init();
    void on_stage_heading();
    void on_stage_heading_in_progress();
    void on_stage_distributing();
    void on_stage_fetching();
    void on_stage_fetching_in_progress();
    void on_stage_merging();
    void on_stage_cleaning();
    void on_stage_finished();

   private:
    QList<TaskDetails> main_queue, loop_queue, finished_list;
    JRequests::HeaderFetcher header_fetcher;
    QList<JRequests::FileFetcher*> fetchers;
    TaskDetails current_running_task;
    quint32 task_queue_refresh_interval;
    quint8 task_queue_max_size;
    quint8 failed_task_requeue_interval;
    quint8 task_max_retries;
    quint8 task_file_slices;
    bool store_slices_in_memory;
    QString slices_storage_root_path;
    QString dest_storage_root_path;
    quint32 task_head_timeout;
    quint8 task_head_max_retries;
    quint32 task_fetch_timeout;
    quint8 task_fetch_max_retries;
    qint64 task_fetch_buffer_size;

    QTimer* runner_loop_timer;
    QThread* stage_thread_ptr = nullptr;
    quint8 queue_index = 1;
    QMutex lock;
};
}  // namespace JackalMFN

#endif
