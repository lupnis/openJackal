/*
 * file name:       HeaderFetcher.cpp
 * created at:      2024/01/20
 * last modified:   2024/02/21
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "HeaderFetcher.h"

namespace JRequests {
HeaderFetcher::HeaderFetcher(QString proxy_host,
                             quint16 proxy_port,
                             QNetworkProxy::ProxyType proxy_type,
                             quint32 timeout,
                             quint32 max_retries) {
    this->request.setProxy(proxy_host, proxy_port, proxy_type);
    this->countdown = this->timeout = timeout;
    this->max_retries = max_retries;
    this->request_count = 0;
    this->tick_timer = new QTimer();
    connect(this->tick_timer, &QTimer::timeout, this,
            &HeaderFetcher::onRequestTimeout);
}
HeaderFetcher::~HeaderFetcher() {
    this->reset();
    this->tick_timer->deleteLater();
    this->tick_timer = nullptr;
}

Mode HeaderFetcher::getMode() const {
    return this->request.getMode();
}
Status HeaderFetcher::getRequestStatus() const {
    return this->request.getStatus();
}
Status HeaderFetcher::getFetcherStatus() const {
    return this->fetcher_status;
}
bool HeaderFetcher::getFinished() const {
    return this->fetcher_status == Status::Finished || this->fetcher_status == Status::Canceled;
}
Result HeaderFetcher::getResult() const {
    return this->request.getResult();
}
QPair<quint64, quint64> HeaderFetcher::getProgress() const {
    return this->request.getProgress();
}
bool HeaderFetcher::getFailed() const {
    return this->fetcher_status == Status::Canceled ||
           (this->fetcher_status == Status::Finished &&
            this->request.getFailed());
}
QHash<QString, QString> HeaderFetcher::getHeaderDict() const {
    return this->request.getHeaderDict();
}
quint64 HeaderFetcher::getContentSize() const {
    return this->request.getHeaderDict().value("content-length", "0").toUInt();
}
QString HeaderFetcher::getDataUnit() const {
    return this->request.getHeaderDict().value("accept-ranges", "none");
}
bool HeaderFetcher::getSliceAvailability() const {
    if (this->getContentSize() && this->getDataUnit() != "none") {
        return true;
    }
    return false;
}
quint32 HeaderFetcher::getTimeConsumed() const {
    return this->timeout - this->countdown;
}
quint32 HeaderFetcher::getRequestCount() const {
    return this->request_count;
}

void HeaderFetcher::setProxy(QString proxy_host,
                             quint16 proxy_port,
                             QNetworkProxy::ProxyType proxy_type) {
    this->request.setProxy(proxy_host, proxy_port, proxy_type);
}
void HeaderFetcher::setTimeout(quint32 timeout) {
    this->timeout = timeout;
}
void HeaderFetcher::setMaxRetries(quint32 max_retries) {
    this->max_retries = max_retries;
}
bool HeaderFetcher::setTask(QString url, QHash<QString, QString> headers) {
    if (this->lock.tryLock()) {
        this->request.head(url, headers);
        this->lock.unlock();
        return true;
    }
    return false;
}

bool HeaderFetcher::run() {
    if (this->lock.tryLock()) {
        this->fetcher_status = Status::Fetching;
        this->request.run();
        this->tick_timer->setInterval(1000);
        this->tick_timer->start();
        return true;
    }
    return false;
}

void HeaderFetcher::abort() {
    if (this->fetcher_status == Status::Fetching) {
        this->fetcher_status = Status::Canceled;
        this->request.abort();
    }
}

void HeaderFetcher::reset() {
    this->abort();
    this->tick_timer->stop();
    this->request.reset();
    this->countdown = this->timeout;
    this->request_count = 0;
    this->fetcher_status = Status::Init;
    this->lock.tryLock();
    this->lock.unlock();
}

void HeaderFetcher::onRequestTimeout() {
    switch (this->request.getStatus()) {
        case Status::Finished:
            if (this->request.getFailed()) {
                if (this->countdown &&
                    this->request_count < this->max_retries) {
                    this->request_count++;
                    QPair<QString, QHash<QString, QString>> task =
                        this->request.getTask();
                    this->request.reset();
                    this->request.head(task.first, task.second);
                    this->request.run();
                }
            } else {
                this->tick_timer->stop();
                this->fetcher_status = Status::Finished;
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
