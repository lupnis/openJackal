/*
 * file name:       FileFetcher.cpp
 * created at:      2024/01/29
 * last modified:   2024/02/21
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "FileFetcher.h"

namespace JRequests {
FileFetcher::FileFetcher(QString proxy_host,
                         quint16 proxy_port,
                         QNetworkProxy::ProxyType proxy_type,
                         quint32 timeout,
                         quint32 max_retries,
                         qint64 buffer_size) {
    this->request.setProxy(proxy_host, proxy_port, proxy_type);
    this->request.setBufferSize(buffer_size);
    this->countdown = this->timeout = timeout;
    this->max_retries = max_retries;
    this->request_count = 0;
    connect(&this->request, &RequestMeta::readyRead, this,
            &FileFetcher::onFileWriteReady);
    this->tick_timer = new QTimer();
    connect(this->tick_timer, &QTimer::timeout, this,
            &FileFetcher::onRequestTimeout);
}
FileFetcher::~FileFetcher() {
    this->reset();
    this->tick_timer->deleteLater();
    this->tick_timer = nullptr;
}

Mode FileFetcher::getMode() const {
    return this->request.getMode();
}
Status FileFetcher::getRequestStatus() const {
    return this->request.getStatus();
}
Status FileFetcher::getFetcherStatus() const {
    return this->fetcher_status;
}
Result FileFetcher::getResult() const {
    return this->request.getResult();
}
QPair<quint64, quint64> FileFetcher::getProgress() const {
    return this->request.getProgress();
}
bool FileFetcher::getFinished() const {
    return this->fetcher_status == Status::Finished || this->fetcher_status == Status::Canceled;
}
bool FileFetcher::getFailed() const {
    return this->fetcher_status == Status::Canceled ||
           (this->fetcher_status == Status::Finished &&
            this->request.getFailed());
}
qint64 FileFetcher::getBufferSize() const {
    return this->request.getBufferSize();
}
quint32 FileFetcher::getTimeConsumed() const {
    return this->timeout - this->countdown;
}
quint32 FileFetcher::getRequestCount() const {
    return this->request_count;
}
bool FileFetcher::getFileStoredAtMem() const {
    return this->store_in_memory;
}
QByteArray FileFetcher::getFileData() const {
    return this->file_data;
}
QString FileFetcher::getFileStoragePath() const {
    return this->file_path;
}

void FileFetcher::setProxy(QString proxy_host,
                           quint16 proxy_port,
                           QNetworkProxy::ProxyType proxy_type) {
    this->request.setProxy(proxy_host, proxy_port, proxy_type);
}
void FileFetcher::setTimeout(quint32 timeout) {
    this->timeout = timeout;
}
void FileFetcher::setMaxRetries(quint32 max_retries) {
    this->max_retries = max_retries;
}
void FileFetcher::setBufferSize(qint64 buffer_size) {
    this->request.setBufferSize(buffer_size);
}
bool FileFetcher::setTask(QString url,
                          QHash<QString, QString> headers,
                          bool store_in_memory,
                          QString file_path) {
    if (this->lock.tryLock()) {
        this->request.get(url, headers);
        this->file_path = file_path;
        if (file_path != "") {
            this->store_in_memory = store_in_memory;
        } else {
            this->store_in_memory = true;
        }
        this->lock.unlock();
        return true;
    }
    return false;
}

bool FileFetcher::run() {
    if (this->lock.tryLock()) {
        this->fetcher_status = Status::Fetching;
        if (!this->store_in_memory) {
            if (QFile(this->file_path).exists()) {
                QFile(this->file_path).remove();
            } else {
                QString abs_path =
                    QDir().absoluteFilePath(this->file_path).replace('\\', '/');
                abs_path = abs_path.left(abs_path.lastIndexOf('/'));
                if (!QDir(abs_path).exists()) {
                    QDir().mkpath(abs_path);
                }
            }
        }
        this->tick_timer->setInterval(1000);
        this->tick_timer->start();
        this->request.run();
        return true;
    }
    return false;
}

void FileFetcher::abort() {
    if (this->fetcher_status == Status::Fetching) {
        this->fetcher_status = Status::Canceled;
        this->request.abort();
    }
}

void FileFetcher::reset() {
    this->abort();
    this->tick_timer->stop();
    this->request.reset();
    this->countdown = this->timeout;
    this->request_count = 0;
    this->file_data.clear();
    this->file_path.clear();
    this->fetcher_status = Status::Init;
    this->lock.tryLock();
    this->lock.unlock();
}

void FileFetcher::onFileWriteReady(QByteArray data, bool last) {
    if (this->store_in_memory) {
        if (data.size() > 0) {
            this->file_data += data;
        }
    } else {
        QFile file(this->file_path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            if (data.size() > 0) {
                file.write(data);
                file.flush();
            }
            file.close();
        }
    }
    if (last) {
        this->fetcher_status = Status::Finished;
    }
}

void FileFetcher::onRequestTimeout() {
    switch (this->request.getStatus()) {
        case Status::Finished:
            if (this->request.getFailed()) {
                if (this->countdown &&
                    this->request_count < this->max_retries) {
                    this->request_count++;
                    QPair<QString, QHash<QString, QString>> task =
                        this->request.getTask();
                    this->request.reset();
                    if (this->store_in_memory) {
                        this->file_data.clear();
                    } else {
                        if (QFile(this->file_path).exists()) {
                            QFile(this->file_path).remove();
                        }
                    }
                    this->request.get(task.first, task.second);
                    this->request.run();
                }
            } else {
                this->tick_timer->stop();
                this->request.emitRemainingData(true);
                return;
            }
            break;
        case Status::Canceled:
            this->tick_timer->stop();
            return;
        default:
            break;
    }
    if (!this->countdown) {
        this->abort();
        this->tick_timer->stop();
    } else {
        this->countdown--;
    }
}

}  // namespace JRequests
