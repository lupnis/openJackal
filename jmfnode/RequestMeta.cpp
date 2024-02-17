/*
 * file name:       RequestMeta.cpp
 * created at:      2024/01/18
 * last modified:   2024/02/17
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "RequestMeta.h"

namespace JRequests {
RequestMeta::RequestMeta(QString proxy_host, quint16 proxy_port,
                         QNetworkProxy::ProxyType proxy_type,
                         qint64 buffer_size) {
    this->init_manager();
    this->setProxy(proxy_host, proxy_port, proxy_type);
    this->setBufferSize(buffer_size);
}
RequestMeta::~RequestMeta() { this->reset(); }

Method RequestMeta::getMethod() const { return this->method; }
Mode RequestMeta::getMode() const { return this->mode; }
Status RequestMeta::getStatus() const { return this->status; }
Result RequestMeta::getResult() const { return this->result; }
qint64 RequestMeta::getBufferSize() const { return this->buffer_size; }
QPair<quint64, quint64> RequestMeta::getProgress() const {
    return {this->received, this->total};
}
QByteArray RequestMeta::getReplyData() const {
    QByteArray ret;
    if (this->received_queue.isEmpty() == false) {
        ret = this->received_queue.front();
    }
    this->received_queue.pop_front();
    return ret;
}

QHash<QString, QString> RequestMeta::getHeaderDict() const {
    QHash<QString, QString> ret;
    if (this->status == Status::Finished) {
        for (const QPair<QByteArray, QByteArray>& pr :
             this->reply->rawHeaderPairs()) {
            ret[QString::fromUtf8(pr.first).toLower()] =
                QString::fromUtf8(pr.second).toLower();
        }
    }
    return ret;
}
bool RequestMeta::hasNextPendingReply() const {
    return this->received_queue.size();
}
bool RequestMeta::getFinished() const {
    return this->status == Status::Finished;
}
bool RequestMeta::getFailed() const { return this->result == Result::Failed; }

QPair<QString, QHash<QString, QString>> RequestMeta::getTask() const {
    return {this->url, this->headers};
}

void RequestMeta::setProxy(QString proxy_host, quint16 proxy_port,
                           QNetworkProxy::ProxyType proxy_type) {
    if (proxy_host == "") {
        proxy_host = "127.0.0.1";
    }
    if (proxy_port == 0) {
        this->proxy.setType(QNetworkProxy::ProxyType::NoProxy);
    } else {
        this->proxy.setHostName(proxy_host);
        this->proxy.setPort(proxy_port);
        this->proxy.setType(proxy_type);
    }
    this->manager.setProxy(this->proxy);
}
void RequestMeta::setBufferSize(qint64 buffer_size) {
    this->buffer_size = buffer_size;
}

bool RequestMeta::head(QString url, QHash<QString, QString> headers) {
    return this->request(url, Method::HEAD, headers);
}
bool RequestMeta::get(QString url, QHash<QString, QString> headers) {
    return this->request(url, Method::GET, headers);
}
bool RequestMeta::request(QString url, Method method,
                          QHash<QString, QString> headers) {
    if (this->lock.tryLock()) {
        this->url = url;
        this->method = method;
        this->headers = headers;
        this->headers.detach();
        this->lock.unlock();
        return true;
    }
    return false;
}

bool RequestMeta::run() {
    if (this->lock.try_lock()) {
        this->status = Status::Fetching;
        this->result = Result::Waiting;
        this->auto_mode_select();
        if (this->mode == Mode::Https) {
            this->network_request.setSslConfiguration(this->ssl);
        }
        this->network_request.setUrl(this->url);
        QString host = url.right(url.size() - url.indexOf("//") - 2);
        host = host.left(host.indexOf("/"));
        QHash<QString, QString> new_headers =
            mergeDicts(headers, QHash<QString, QString>({{"Host", host}}));
        for (QHash<QString, QString>::iterator it = new_headers.begin();
             it != new_headers.end(); ++it) {
            this->network_request.setRawHeader(it.key().toUtf8(),
                                               it.value().toUtf8());
        }
        switch (this->method) {
            case Method::GET:
                this->reply = this->manager.get(this->network_request);
                break;
            case Method::HEAD:
                this->reply = this->manager.head(this->network_request);
                break;
        }
        this->reply->setReadBufferSize(this->buffer_size);
        connect(this->reply, &QNetworkReply::downloadProgress, this,
                &RequestMeta::onProgressChanged);
        connect(this->reply, &QNetworkReply::readyRead, this,
                &RequestMeta::onReadyRead);
        return true;
    }
    return false;
}
void RequestMeta::abort() {
    if (this->status == Status::Fetching) {
        this->result = Result::Failed;
        this->status = Status::Canceled;
        if (this->reply != nullptr) {
            this->reply->abort();
        }
    }
}
void RequestMeta::reset() {
    this->abort();
    if (this->reply != nullptr) {
        this->reply->deleteLater();
        this->reply = nullptr;
    }
    this->received_queue.clear();
    this->network_request = QNetworkRequest();
    this->method = Method::GET;
    this->url.clear();
    this->headers.clear();
    this->mode = Mode::Unspecified;
    this->result = Result::Standby;
    this->status = Status::Init;
    this->received = this->total = 0;
    this->headers.clear();
    this->lock.tryLock();
    this->lock.unlock();
}

void RequestMeta::init_manager() {
    this->manager.setProxy(this->proxy);
    this->ssl = QSslConfiguration::defaultConfiguration();
    this->ssl.setPeerVerifyDepth(QSslSocket::AutoVerifyPeer);
    this->ssl.setProtocol(QSsl::AnyProtocol);
    this->manager.setStrictTransportSecurityEnabled(false);
    connect(&this->manager, &QNetworkAccessManager::finished, this,
            &RequestMeta::onReplyFinished);
    connect(&this->manager, &QNetworkAccessManager::sslErrors, this,
            &RequestMeta::onSslError);
}
void RequestMeta::auto_mode_select() {
    if (url.startsWith("https://")) {
        this->mode = Mode::Https;
    } else if (url.startsWith("http://")) {
        this->mode = Mode::Http;
    } else {
        this->mode = Mode::Unspecified;
    }
}

void RequestMeta::onReadyRead() {
    int status_code =
        this->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();
    if (this->reply->error() == QNetworkReply::NoError && status_code >= 200 &&
        status_code < 300) {
        this->received_queue.push_back(this->reply->readAll());
        emit this->readyRead();
    }
}
void RequestMeta::onReplyFinished(QNetworkReply*) {
    if (this->status == Status::Canceled) {
        return;
    }
    int status_code =
        this->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();
    if (this->reply->error() == QNetworkReply::NoError && status_code >= 200 &&
        status_code < 300) {
        this->received_queue.push_back(this->reply->readAll());
        this->status = Status::Finished;
        this->result = Result::Succeeded;
        emit this->readyRead();
    } else if (this->reply->error() == QNetworkReply::NoError &&
               this->reply->rawHeader("Location").size()) {
        this->url = QString::fromUtf8(this->reply->rawHeader("Location"));
        this->reply->deleteLater();
        this->reply = nullptr;
        this->network_request = QNetworkRequest();
        this->auto_mode_select();
        if (this->mode == Mode::Https) {
            this->network_request.setSslConfiguration(this->ssl);
        }
        this->network_request.setUrl(this->url);
        QString host = url.right(url.size() - url.indexOf("//") - 2);
        host = host.left(host.indexOf("/"));
        QHash<QString, QString> new_headers =
            mergeDicts(headers, QHash<QString, QString>({{"Host", host}}));
        for (QHash<QString, QString>::iterator it = new_headers.begin();
             it != new_headers.end(); ++it) {
            this->network_request.setRawHeader(it.key().toUtf8(),
                                               it.value().toUtf8());
        }
        switch (this->method) {
        case Method::GET:
            this->reply = this->manager.get(this->network_request);
            break;
        case Method::HEAD:
            this->reply = this->manager.head(this->network_request);
            break;
        }
        this->reply->setReadBufferSize(this->buffer_size);
        connect(this->reply, &QNetworkReply::downloadProgress, this,
                &RequestMeta::onProgressChanged);
        connect(this->reply, &QNetworkReply::readyRead, this,
                &RequestMeta::onReadyRead);
    } else {
        this->status = Status::Finished;
        this->result = Result::Failed;
    }
}

void RequestMeta::onProgressChanged(qint64 received, qint64 total) {
    this->received = received;
    this->total = total;
}

void RequestMeta::onSslError(QNetworkReply*, const QList<QSslError>&) {}

}  // namespace JRequests
