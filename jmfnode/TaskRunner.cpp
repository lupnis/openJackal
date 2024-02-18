/*
 * file name:       TaskRunner.cpp
 * created at:      2024/02/01
 * last modified:   2024/02/18
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "TaskRunner.h"

namespace JackalMFN {
TaskRunner::TaskRunner(quint32 task_queue_refresh_interval,
                       quint8 task_queue_max_size,
                       quint8 failed_task_requeue_interval,
                       quint8 task_max_retries,
                       quint8 task_file_slices,
                       bool store_slices_in_memory,
                       QString slices_storage_root_path,
                       QString dest_storage_root_path,
                       quint32 task_head_timeout,
                       quint8 task_head_max_retries,
                       quint32 task_fetch_timeout,
                       quint8 task_fetch_max_retries,
                       qint64 task_fetch_buffer_size) {
    this->task_queue_refresh_interval = task_queue_refresh_interval;
    this->task_queue_max_size = task_queue_max_size;
    this->failed_task_requeue_interval = failed_task_requeue_interval;
    this->task_max_retries = task_max_retries;
    this->task_file_slices = task_file_slices;
    this->store_slices_in_memory = store_slices_in_memory;
    this->slices_storage_root_path = slices_storage_root_path;
    this->dest_storage_root_path = dest_storage_root_path;
    this->task_head_timeout = task_head_timeout;
    this->task_head_max_retries = task_head_max_retries;
    this->task_fetch_timeout = task_fetch_timeout;
    this->task_head_max_retries = task_fetch_max_retries;
    this->task_fetch_buffer_size = task_fetch_buffer_size;
    connect(this, &TaskRunner::errorsEmitted, this,
            &TaskRunner::onErrorsEmitted);
    this->runner_loop_timer = new QTimer();
    connect(this->runner_loop_timer, &QTimer::timeout, this,
            &TaskRunner::onLoopTimerTimeout);
}

TaskRunner::~TaskRunner() {
    this->terminateRunnerLoop();
    this->runner_loop_timer->deleteLater();
    this->runner_loop_timer = nullptr;
}

bool TaskRunner::updateConfigurations(quint32 task_queue_refresh_interval,
                                      quint8 task_queue_max_size,
                                      quint8 failed_task_requeue_interval,
                                      quint8 task_max_retries,
                                      quint8 task_file_slices,
                                      bool store_slices_in_memory,
                                      QString slices_storage_root_path,
                                      QString dest_storage_root_path,
                                      quint32 task_head_timeout,
                                      quint8 task_head_max_retries,
                                      quint32 task_fetch_timeout,
                                      quint8 task_fetch_max_retries,
                                      qint64 task_fetch_buffer_size) {
    if (this->lock.tryLock()) {
        this->task_queue_refresh_interval = task_queue_refresh_interval;
        this->task_queue_max_size = task_queue_max_size;
        this->failed_task_requeue_interval = failed_task_requeue_interval;
        this->task_max_retries = task_max_retries;
        this->task_file_slices = task_file_slices;
        this->store_slices_in_memory = store_slices_in_memory;
        this->slices_storage_root_path = slices_storage_root_path;
        this->dest_storage_root_path = dest_storage_root_path;
        this->task_head_timeout = task_head_timeout;
        this->task_head_max_retries = task_head_max_retries;
        this->task_fetch_timeout = task_fetch_timeout;
        this->task_head_max_retries = task_fetch_max_retries;
        this->task_fetch_buffer_size = task_fetch_buffer_size;
        return true;
    }
    return false;
}

bool TaskRunner::addTaskToQueue(TaskDetails task, bool add_to_main_queue) {
    if ((this->main_queue.size() + this->loop_queue.size()) >=
        this->task_queue_max_size) {
        return false;
    }
    if (add_to_main_queue) {
        this->main_queue.push_back(task);
    } else {
        this->loop_queue.push_back(task);
    }
    return true;
}
bool TaskRunner::addTaskToQueue(QString mirror_name,
                                QString url_path,
                                QString storage_path,
                                QString proxy_host,
                                quint16 proxy_port,
                                QNetworkProxy::ProxyType proxy_type,
                                bool add_to_main_queue) {
    if ((this->main_queue.size() + this->loop_queue.size()) >=
        this->task_queue_max_size) {
        return false;
    }
    if (add_to_main_queue) {
        this->main_queue.push_back({mirror_name, url_path, storage_path,
                                    proxy_host, proxy_port, proxy_type});
    } else {
        this->loop_queue.push_back({mirror_name, url_path, storage_path,
                                    proxy_host, proxy_port, proxy_type});
    }
    return true;
}

QList<TaskDetails> TaskRunner::getMainQueue() const {
    return this->main_queue;
}
QList<TaskDetails> TaskRunner::getLoopQueue() const {
    return this->loop_queue;
}
QList<TaskDetails> TaskRunner::getFinishedTasks() const {
    return this->finished_list;
}
TaskDetails TaskRunner::getTaskDetails(QString mirror_name,
                                       QString url_path) const {
    for (const TaskDetails& task : this->main_queue) {
        if (task.mirrorName == mirror_name && task.urlPath == url_path) {
            return task;
        }
    }
    for (const TaskDetails& task : this->loop_queue) {
        if (task.mirrorName == mirror_name && task.urlPath == url_path) {
            return task;
        }
    }
    return {};
}
bool TaskRunner::dropTask(QString mirror_name, QString url_path) {
    for (int i = 0; i < this->main_queue.size(); ++i) {
        if (this->main_queue[i].mirrorName == mirror_name &&
            this->main_queue[i].urlPath == url_path) {
            this->main_queue.removeAt(i);
            return true;
        }
    }
    for (int i = 0; i < this->loop_queue.size(); ++i) {
        if (this->loop_queue[i].mirrorName == mirror_name &&
            this->loop_queue[i].urlPath == url_path) {
            this->loop_queue.removeAt(i);
            return true;
        }
    }
    return false;
}
void TaskRunner::dropMainQueue() {
    this->main_queue.clear();
}
void TaskRunner::dropLoopQueue() {
    this->loop_queue.clear();
}
void TaskRunner::dropAllQueues() {
    this->dropMainQueue();
    this->dropLoopQueue();
}
void TaskRunner::dropFinishedTaskRecords() {
    this->finished_list.clear();
}
void TaskRunner::swapQueues() {
    this->main_queue.swap(this->loop_queue);
}
void TaskRunner::mergeQueues() {
    this->main_queue.append(this->loop_queue);
    this->loop_queue.clear();
}
void TaskRunner::resetFaultsCounter() {
    for (TaskDetails& task : this->main_queue) {
        task.failedCount = 0;
    }
    for (TaskDetails& task : this->loop_queue) {
        task.failedCount = 0;
    }
}
TaskDetails TaskRunner::getNextTask() {
    TaskDetails task;
    if (this->main_queue.size() || this->loop_queue.size()) {
        if (!this->queue_index) {
            if (this->loop_queue.size()) {
                task = this->loop_queue.front();
                this->loop_queue.pop_front();
            } else {
                task = this->main_queue.front();
                this->main_queue.pop_front();
            }
        } else {
            if (this->main_queue.size()) {
                task = this->main_queue.front();
                this->main_queue.pop_front();
            } else {
                task = this->loop_queue.front();
                this->loop_queue.pop_front();
            }
        }
    }
    this->queue_index =
        (this->queue_index + 1) % this->failed_task_requeue_interval;
    return task;
}

bool TaskRunner::setAsNextTask(QString mirror_name, QString url_path) {
    TaskDetails target = this->getTaskDetails(mirror_name, url_path);
    if (target.hasTask()) {
        this->dropTask(mirror_name, url_path);
        this->main_queue.push_front(target);
        return true;
    }
    return false;
}

QList<JRequests::Status> TaskRunner::getCurrentFetcherStatus() const {
    QList<JRequests::Status> status;
    status.push_back(this->header_fetcher.getFetcherStatus());
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        status.push_back(this->fetchers[i]->getFetcherStatus());
    }
    return status;
}
QList<JRequests::Status> TaskRunner::getCurrentRequestStatus() const {
    QList<JRequests::Status> status;
    status.push_back(this->header_fetcher.getRequestStatus());
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        status.push_back(this->fetchers[i]->getRequestStatus());
    }
    return status;
}
QList<JRequests::Result> TaskRunner::getCurrentRequestResult() const {
    QList<JRequests::Result> result;
    result.push_back(this->header_fetcher.getResult());
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        result.push_back(this->fetchers[i]->getResult());
    }
    return result;
}
QList<QPair<qint64, qint64>> TaskRunner::getCurrentFetcherProgress() const {
    QList<QPair<qint64, qint64>> progress;
    progress.push_back(this->header_fetcher.getProgress());
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        progress.push_back(this->fetchers[i]->getProgress());
    }
    return progress;
}
QList<quint32> TaskRunner::getFetchersTimeConsumed() const {
    QList<quint32> consumed;
    consumed.push_back(this->header_fetcher.getTimeConsumed());
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        consumed.push_back(this->fetchers[i]->getTimeConsumed());
    }
    return consumed;
}
TaskStage TaskRunner::getCurrentTaskStage() const {
    return this->current_running_task.currentStage;
}

void TaskRunner::dropCurrentRunningTask(bool add_to_loop_queue) {
    if (this->current_running_task.hasTask()) {
        TaskDetails task = this->current_running_task;
        if (add_to_loop_queue) {
            this->loop_queue.push_back(task);
        }
        this->runner_loop_timer->stop();
        this->header_fetcher.reset();
        for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
            this->fetchers[i]->reset();
        }
        this->current_running_task = {};
        this->runner_loop_timer->start();
    }
}
bool TaskRunner::startRunnerLoop() {
    if (this->lock.tryLock()) {
        this->header_fetcher.setMaxRetries(this->task_head_max_retries);
        this->header_fetcher.setTimeout(this->task_head_timeout);
        for (int i = 0; i < this->task_file_slices; ++i) {
            this->fetchers.push_back(new JRequests::FileFetcher(
                "", 0, QNetworkProxy::HttpProxy, this->task_fetch_timeout,
                this->task_fetch_max_retries, this->task_fetch_buffer_size));
        }
        this->runner_loop_timer->setInterval(this->task_queue_refresh_interval);
        this->runner_loop_timer->start();
        return true;
    }
    return false;
}
void TaskRunner::stopRunnerLoop() {
    this->runner_loop_timer->stop();
    this->header_fetcher.reset();
    this->current_running_task.currentStage = TaskStage::Init;
    for (int i = 0; i < this->fetchers.size(); ++i) {
        this->fetchers[i]->reset();
        this->fetchers[i]->deleteLater();
        this->fetchers[i] = nullptr;
    }
    this->fetchers.clear();
    if (this->stage_thread_ptr != nullptr) {
        this->stage_thread_ptr->terminate();
        this->stage_thread_ptr->deleteLater();
        this->stage_thread_ptr = nullptr;
    }
    this->lock.tryLock();
    this->lock.unlock();
}
void TaskRunner::terminateRunnerLoop() {
    this->stopRunnerLoop();
    this->dropCurrentRunningTask();
    this->dropAllQueues();
    this->dropFinishedTaskRecords();
}

void TaskRunner::onErrorsEmitted() {
    this->current_running_task.failedCount++;
    this->dropCurrentRunningTask(this->current_running_task.failedCount <=
                                 this->task_max_retries);
}

void TaskRunner::on_stage_init() {
    if (!this->current_running_task.hasTask()) {
        this->current_running_task = this->getNextTask();
    }
    if (this->current_running_task.hasTask()) {
        this->current_running_task.currentStage = TaskStage::Heading;
    }
}
void TaskRunner::on_stage_heading() {
    this->header_fetcher.setProxy(this->current_running_task.proxyHost,
                                  this->current_running_task.proxyPort,
                                  this->current_running_task.proxyType);
    this->header_fetcher.setTask(this->current_running_task.urlPath);
    this->header_fetcher.run();
    this->current_running_task.currentStage = TaskStage::HeadingInProgress;
}
void TaskRunner::on_stage_heading_in_progress() {
    if (this->header_fetcher.getFinished()) {
        this->current_running_task.currentStage =
            TaskStage::DistributingFetchers;
    }
}
void TaskRunner::on_stage_distributing() {
    this->current_running_task.numFetchers =
        ((this->header_fetcher.getSliceAvailability() &&
          this->header_fetcher.getContentSize() >= this->task_file_slices)
             ? this->task_file_slices
             : 1);
    quint64 slice_range_per_slice = this->header_fetcher.getContentSize() /
                                    this->current_running_task.numFetchers;
    QString slice_storage_path =
        QDir().absoluteFilePath(this->slices_storage_root_path) +
        "/%1/%2_%3.ojs";

    for (int i = 0; i < this->current_running_task.numFetchers - 1; ++i) {
        this->fetchers[i]->setProxy(this->current_running_task.proxyHost,
                                    this->current_running_task.proxyPort,
                                    this->current_running_task.proxyType);
        this->fetchers[i]->setTask(
            this->current_running_task.urlPath,
            JRequests::mergeDicts(
                JRequests::BASE_HEADERS,
                QHash<QString, QString>(
                    {{"Range",
                      QString("%1=%2-%3")
                          .arg(this->header_fetcher.getDataUnit())
                          .arg(i * slice_range_per_slice)
                          .arg((i + 1) * slice_range_per_slice - 1)}})),
            this->store_slices_in_memory,
            slice_storage_path.arg(this->current_running_task.mirrorName)
                .arg(QString(QCryptographicHash::hash(
                                 this->current_running_task.urlPath.toUtf8(),
                                 QCryptographicHash::Md5)
                                 .toHex()))
                .arg(i));
    }
    QHash<QString, QString> last_header = JRequests::BASE_HEADERS;
    this->fetchers[this->current_running_task.numFetchers - 1]->setProxy(
        this->current_running_task.proxyHost,
        this->current_running_task.proxyPort,
        this->current_running_task.proxyType);
    if (this->header_fetcher.getSliceAvailability()) {
        last_header = JRequests::mergeDicts(
            JRequests::BASE_HEADERS,
            QHash<QString, QString>(
                {{"Range",
                  QString("%1=%2-")
                      .arg(this->header_fetcher.getDataUnit())
                      .arg((this->current_running_task.numFetchers - 1) *
                           slice_range_per_slice)}}));
    }
    this->fetchers[this->current_running_task.numFetchers - 1]->setTask(
        this->current_running_task.urlPath, last_header,
        this->store_slices_in_memory,
        slice_storage_path.arg(this->current_running_task.mirrorName)
            .arg(QString(QCryptographicHash::hash(
                             this->current_running_task.urlPath.toUtf8(),
                             QCryptographicHash::Md5)
                             .toHex()))
            .arg(this->current_running_task.numFetchers - 1));
    this->current_running_task.currentStage = TaskStage::FetchingUrl;
}
void TaskRunner::on_stage_fetching() {
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        this->fetchers[i]->run();
    }
    this->current_running_task.currentStage = TaskStage::FetchingInProgress;
}
void TaskRunner::on_stage_fetching_in_progress() {
    for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
        if (this->fetchers[i]->getFailed()) {
            emit this->errorsEmitted();
            return;
        }
        if (!this->fetchers[i]->getFinished()) {
            return;
        }
    }
    this->current_running_task.currentStage = TaskStage::MergingSlices;
}
void TaskRunner::on_stage_merging() {
    this->stage_thread_ptr = new QThread();
    connect(this->stage_thread_ptr, &QThread::finished, this->stage_thread_ptr,
            &QThread::deleteLater);
    connect(this->stage_thread_ptr, &QThread::started, [this] {
        QString dest_path =
            QDir().absoluteFilePath(this->dest_storage_root_path) + "/%1/%2";
        dest_path = dest_path.arg(this->current_running_task.mirrorName)
                        .arg(this->current_running_task.storagePath);
        QString dest_dir = dest_path.left(dest_path.lastIndexOf('/'));
        if (!QDir().exists(dest_dir) && !QDir().mkpath(dest_dir)) {
            emit this->errorsEmitted();
            return;
        }
        QFile dest_file(dest_path);
        if (!dest_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit this->errorsEmitted();
            return;
        }
        for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
            if (this->fetchers[i]->getFileStoredAtMem()) {
                if (!dest_file.write(this->fetchers[i]->getFileData())) {
                    emit this->errorsEmitted();
                    return;
                }
                dest_file.flush();
            } else {
                QFile slice_file(this->fetchers[i]->getFileStoragePath());
                if (!slice_file.open(QIODevice::ReadOnly)) {
                    emit this->errorsEmitted();
                    return;
                }
                while (!slice_file.atEnd()) {
                    if (!dest_file.write(
                            slice_file.read(this->task_fetch_buffer_size))) {
                        emit this->errorsEmitted();
                        return;
                    }
                    dest_file.flush();
                }
            }
        }
        this->current_running_task.currentStage = TaskStage::CleaningCache;
        QThread::currentThread()->quit();
    });
    this->stage_thread_ptr->start();
    this->current_running_task.currentStage = TaskStage::MergingInProgress;
}
void TaskRunner::on_stage_cleaning() {
    this->stage_thread_ptr = new QThread();
    connect(this->stage_thread_ptr, &QThread::finished, this->stage_thread_ptr,
            &QThread::deleteLater);
    connect(this->stage_thread_ptr, &QThread::started, [this] {
        this->header_fetcher.reset();
        for (int i = 0; i < this->current_running_task.numFetchers; ++i) {
            if (!this->fetchers[i]->getFileStoredAtMem()) {
                QFile(this->fetchers[i]->getFileStoragePath()).remove();
            }
            this->fetchers[i]->reset();
        }
        this->current_running_task.currentStage = TaskStage::TaskFinished;
        QThread::currentThread()->quit();
    });
    this->stage_thread_ptr->start();
    this->current_running_task.currentStage = TaskStage::CleaningInProgress;
}
void TaskRunner::on_stage_finished() {
    this->finished_list.push_back(this->current_running_task);
    this->current_running_task = {};
    this->current_running_task.currentStage = TaskStage::Init;
}

void TaskRunner::onLoopTimerTimeout() {
    switch (this->current_running_task.currentStage) {
        case TaskStage::Init:
            this->on_stage_init();
            break;
        case TaskStage::Heading:
            this->on_stage_heading();
            break;
        case TaskStage::HeadingInProgress:
            this->on_stage_heading_in_progress();
            break;
        case TaskStage::DistributingFetchers:
            this->on_stage_distributing();
            break;
        case TaskStage::FetchingUrl:
            this->on_stage_fetching();
            break;
        case TaskStage::FetchingInProgress:
            this->on_stage_fetching_in_progress();
            break;
        case TaskStage::MergingSlices:
            this->on_stage_merging();
            break;
        case TaskStage::MergingInProgress:
            // waiting till the process of merging completes, do nothing
            break;
        case TaskStage::CleaningCache:
            this->on_stage_cleaning();
            break;
        case TaskStage::CleaningInProgress:
            // waiting for cached data to be removed, do nothing
            break;
        case TaskStage::TaskFinished:
            this->on_stage_finished();
            break;
    }
}
}  // namespace JackalMFN
