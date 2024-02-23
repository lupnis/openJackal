/*
 * file name:       Configurations.h
 * created at:      2024/02/14
 * last modified:   2024/02/23
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef CONFIGURATIONS_H
#define CONFIGURATIONS_H

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QObject>
#include <QtCore>

#include "Logging.h"

namespace JConfigs {

inline QJsonObject makeDefaultConfigs() {
    QHash<QString, QVariant> config_dict;

    config_dict["logging"] = JLogs::DEFAULT_LOG_CONFIG;

    config_dict["mysql"] =
        QHash<QString, QVariant>({{"host", "127.0.0.1"},
                                  {"port", 3306},
                                  {"user", "u_jackalmfn"},
                                  {"pass", "p_jackalmfn"},
                                  {"schema", "schema_open_jackal"}});

    config_dict["redis"] = QHash<QString, QVariant>({{"host", "127.0.0.1"},
                                                     {"port", 6379},
                                                     {"user", ""},
                                                     {"pass", ""},
                                                     {"db", 0}});

    config_dict["runners"] =
        QHash<QString, QVariant>({{"task_queue_refresh_interval", 50},
                                  {"task_queue_max_size", 64},
                                  {"failed_task_requeue_interval", 2},
                                  {"task_max_retries", 5},
                                  {"task_file_slices", 16},
                                  {"store_slices_in_memory", false},
                                  {"slices_storage_root_path", "./temp"},
                                  {"dest_storage_root_path", "./archive"},
                                  {"task_head_timeout", 120},
                                  {"task_head_max_retries", 8},
                                  {"task_fetch_timeout", 600},
                                  {"task_fetch_max_retries", 8},
                                  {"task_fetch_buffer_size", 104857500}});

    config_dict["node"] =
        QHash<QString, QVariant>({{"num_runners", 2},
                                  {"node_task_receiving_interval", 5000}});
    return QJsonObject::fromVariantHash(config_dict);
}
class JsonConfigHandler {
   public:
    JsonConfigHandler();
    JsonConfigHandler(const JsonConfigHandler& rvalue);
    ~JsonConfigHandler();

    void setFilePath(QString config_path);
    QString getFilePath() const;
    void loadConfigs(QJsonObject default_configs = makeDefaultConfigs());
    void saveConfigs();

    QVariant get(QString path, QJsonValue repl = QJsonValue()) const;
    bool pathExists(QString path) const;
    void set(QString path, QJsonValue value);

   private:
    QString src_path = "node_properties.json";
    QHash<QString, QVariant> configs;
};
}  // namespace JConfigs

#endif
