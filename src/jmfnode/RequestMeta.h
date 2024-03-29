/*
 * file name:       RequestMeta.h
 * created at:      2024/01/18
 * last modified:   2024/02/22
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef REQUEST_META_H
#define REQUEST_META_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSsl>

namespace JRequests {
const QHash<QString, QString> BASE_HEADERS{
    {"Accept",
     "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/"
     "apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"},
    {"Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6"},
    {"Connection", "keep-alive"},
    {"Cache-Control", "no-cache"},
    {"Pragma", "no-cache"},
    {"Sec-Ch-Ua",
     "\"Not_A Brand\";v=\"8\", \"QtNetwork Lib\";v=\"5145\", \"Jackal "
     "Mirror Fetcher\";v=\"120\""},
    {"Sec-Ch-Ua-Mobile", "?0"},
    {"Sec-Ch-Ua-Platform", "\"Windows\""},
    {"Sec-Fetch-User", "?1"},
    {"Sec-Fetch-Dest", "document"},
    {"Sec-Fetch-Mode", "navigate"},
    {"Sec-Fetch-Site", "none"},
    {"Upgrade-Insecure-Requests", "1"},
    {"User-Agent",
     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
     "like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0"}};
/*
 * default available headers defined here,
 * avoid defining "accept-encoding" if gzip is included in the encoding
 * candidate options, for auto-decompression will be disabled if this header is
 * defined.
 */

const QHash<QString, QString> EMPTY_DICT{};

Q_NAMESPACE
enum Method {
    GET,
    HEAD
}; /* current JRequests::RequestMeta only supports these two methods */
enum Mode { Unspecified, Http, Https };
enum Status { Init, Fetching, Finished, Canceled };
Q_ENUM_NS(Status)
/*
 * status indicates the current running status of JRequests::RequestMeta entity,
 * if request is not at running status(Status::Fetching), the status will not be
 * changed to canceled.
 * Init: the request is pending for running, of there are no requests.
 * Fetching: the requets is running and waiting to get full response data.
 * Finished: the request is finished, and all data has been retrived.
 * Canceled: the request is canceled on the process of fetching.
 */
enum Result { Standby, Waiting, Succeeded, Failed };
Q_ENUM_NS(Result)
/*
 * result indicates the retrival results of the task. canceled tasks will be
 * recognized as failed.
 * Standby: the request task has not been started.
 * Waiting: the request task is running and waiting to be finished.
 * Succeeded: the request task has successfully accomplished.
 * Failed: the request task failed because of network faults or manual
 * cancellation.
 */

template <typename... DICT>
static inline const QHash<QString, QString> mergeDicts(const DICT&... dicts) {
    QList<QHash<QString, QString>> dict_arr = {dicts...};
    QHash<QString, QString> ret;
    for (const QHash<QString, QString>& item : dict_arr) {
        ret.unite(item);
    }
    return ret;
}
/*
 * merge dicts together and return a united dict.
 * inputs are variable parameters of type QHash<QString, QString>
 */

class RequestMeta : public QObject {
    Q_OBJECT
   public:
    RequestMeta(QString proxy_host = "", quint16 proxy_port = 0,
                QNetworkProxy::ProxyType proxy_type =
                    QNetworkProxy::ProxyType::HttpProxy,
                qint64 buffer_size = 1048576);
    ~RequestMeta();

    Method getMethod() const;
    Mode getMode() const;
    Status getStatus() const;
    Result getResult() const;
    qint64 getBufferSize() const;
    void emitRemainingData(bool last = false);
    QPair<quint64, quint64> getProgress() const;
    QHash<QString, QString> getHeaderDict() const;
    bool getFinished() const;
    bool getFailed() const;
    QPair<QString, QHash<QString, QString>> getTask() const;

    void setProxy(QString proxy_host = "", quint16 proxy_port = 0,
                  QNetworkProxy::ProxyType proxy_type =
                      QNetworkProxy::ProxyType::HttpProxy);
    void setBufferSize(qint64 buffer_size = 1048576);

    bool head(QString url, QHash<QString, QString> headers = BASE_HEADERS);
    bool get(QString url, QHash<QString, QString> headers = BASE_HEADERS);
    bool request(QString url, Method method, QHash<QString, QString> headers);

    bool run();
    void abort();
    void reset();

   private:
    void init_manager();
    void auto_mode_select();

   signals:
    void readyRead(QByteArray data, bool last = false);

   private slots:
    void onReadyRead();
    void onReplyFinished(QNetworkReply*);
    void onProgressChanged(qint64 received, qint64 total);
    void onSslError(QNetworkReply*, const QList<QSslError>&);

   private:
    QNetworkAccessManager manager;
    QNetworkRequest network_request;
    QSslConfiguration ssl;
    QNetworkProxy proxy;

    Method method = Method::GET;
    Mode mode = Mode::Unspecified;
    Status status = Status::Init;
    Result result = Result::Standby;
    quint64 received = 0, total = 0;
    QString url;
    QHash<QString, QString> headers = EMPTY_DICT;
    qint64 buffer_size;

    QNetworkReply* reply = nullptr;
    QMutex lock;
};

}  // namespace JRequests

#endif
