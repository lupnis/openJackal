/*
 * file name:       FileFetcher.h
 * created at:      2024/01/23
 * last modified:   2024/02/14
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef FILE_FETCHER_H
#define FILE_FETCHER_H

#include <QDir>
#include <QFile>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "RequestMeta.h"

namespace JRequests {

class FileFetcher : public QObject {
    Q_OBJECT
   public:
    FileFetcher(QString proxy_host = "", quint16 proxy_port = 0,
                QNetworkProxy::ProxyType proxy_type =
                    QNetworkProxy::ProxyType::HttpProxy,
                quint32 timeout = 600, quint32 max_retries = 8,
                qint64 buffer_size = 1048576);

    ~FileFetcher();
    FileFetcher& operator=(const FileFetcher&){return *this;}

    Mode getMode() const;
    Status getRequestStatus() const;
    Status getFetcherStatus() const;
    Result getResult() const;
    QPair<quint64, quint64> getProgress() const;
    bool getFinished() const;
    bool getFailed() const;
    qint64 getBufferSize() const;
    quint32 getTimeConsumed() const;
    quint32 getRequestCount() const;
    bool getFileStoredAtMem() const;
    QByteArray getFileData() const;
    QString getFileStoragePath() const;

    void setProxy(QString proxy_host = "", quint16 proxy_port = 0,
                  QNetworkProxy::ProxyType proxy_type =
                      QNetworkProxy::ProxyType::HttpProxy);
    void setTimeout(quint32 timeout = 600);
    void setMaxRetries(quint32 max_retries = 8);
    void setBufferSize(qint64 buffer_size = 1048576);

    bool setTask(QString url, QHash<QString, QString> headers = BASE_HEADERS,
                 bool store_in_memory = true, QString file_path = "");
    bool run();
    void abort();
    void reset();

   private slots:
    void onFileWriteReady();
    void onRequestTimeout();

   private:
    RequestMeta request;
    quint32 timeout, countdown, max_retries, request_count;
    bool store_in_memory;
    QString file_path;
    QByteArray file_data;
    QTimer* tick_timer = nullptr;
    QThread* file_write_thread_ptr = nullptr;
    Status fetcher_status = Status::Init;
    QMutex lock;
};
}  // namespace JRequests

#endif
