/*
 * file name:       HeaderFetcher.h
 * created at:      2024/01/19
 * last modified:   2024/02/10
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef HEADER_FETCHER_H
#define HEADER_FETCHER_H

#include <QObject>
#include <QTimer>

#include "RequestMeta.h"

namespace JRequests {
class HeaderFetcher : public QObject {
    Q_OBJECT
   public:
    HeaderFetcher(QString proxy_host = "", quint16 proxy_port = 0,
                  QNetworkProxy::ProxyType proxy_type =
                      QNetworkProxy::ProxyType::HttpProxy,
                  quint32 timeout = 60, quint32 max_retries = 8);
    ~HeaderFetcher();

    Mode getMode() const;
    Status getRequestStatus() const;
    Status getFetcherStatus() const;
    Result getResult() const;
    QPair<quint64, quint64> getProgress() const;
    bool getFinished() const;
    bool getFailed() const;
    QHash<QString, QString> getHeaderDict() const;
    quint64 getContentSize() const;
    QString getDataUnit() const;
    bool getSliceAvailability() const;
    quint32 getTimeConsumed() const;
    quint32 getRequestCount() const;

    void setProxy(QString proxy_host = "", quint16 proxy_port = 0,
                  QNetworkProxy::ProxyType proxy_type =
                      QNetworkProxy::ProxyType::HttpProxy);
    void setTimeout(quint32 timeout = 60);
    void setMaxRetries(quint32 max_retries = 8);

    bool setTask(QString url, QHash<QString, QString> headers = BASE_HEADERS);
    bool run();
    void abort();
    void reset();

   private slots:
    void onRequestTimeout();

   private:
    RequestMeta request;
    quint32 timeout, countdown, max_retries, request_count;
    QTimer* tick_timer = nullptr;
    Status fetcher_status = Status::Init;
    QMutex lock;
};
}  // namespace JRequests

#endif
