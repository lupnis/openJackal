/*
 * file name:       Configurations.cpp
 * created at:      2024/02/14
 * last modified:   2024/02/17
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "Configurations.h"

namespace JConfigs {
JsonConfigHandler::JsonConfigHandler() {
    this->src_path = QDir().absoluteFilePath(this->src_path).replace('\\', '/');
}
JsonConfigHandler::JsonConfigHandler(const JsonConfigHandler& rvalue) {
    this->src_path = rvalue.getFilePath();
}
JsonConfigHandler::~JsonConfigHandler() {}

void JsonConfigHandler::setFilePath(QString config_path) {
    this->src_path = QDir().absoluteFilePath(config_path).replace('\\', '/');
}
QString JsonConfigHandler::getFilePath() const {
    return this->src_path;
}

void JsonConfigHandler::loadConfigs(QJsonObject default_configs) {
    QFile config_file(this->src_path);
    if (!config_file.exists()) {
        this->configs = default_configs.toVariantHash();
        this->saveConfigs();
    } else {
        config_file.open(QIODevice::ReadOnly);
        this->configs = QJsonDocument::fromJson(config_file.readAll())
                            .object()
                            .toVariantHash();
        config_file.close();
    }
}
void JsonConfigHandler::saveConfigs() {
    QFile config_file(this->src_path);
    QDir().mkpath(this->src_path.left(this->src_path.lastIndexOf('/')));
    config_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QJsonDocument json_doc;
    json_doc.setObject(QJsonObject::fromVariantHash(this->configs));
    config_file.write(json_doc.toJson(QJsonDocument::JsonFormat::Indented));
    config_file.close();
}

QVariant JsonConfigHandler::get(QString path, QJsonValue repl) const {
    QList<QString> path_list = path.split('/');
    QVariant config_sel = this->configs;
    QVariantList t_jarr;
    QVariantMap t_jobj;
    for (const QString& path : path_list) {
        if (path == "") {
            continue;
        }
        switch (config_sel.type()) {
            case QVariant::Type::List: {
                bool converted;
                int index = path.toInt(&converted);
                t_jarr = config_sel.toList();
                if (!converted || index >= t_jarr.size()) {
                    return repl;
                }
                config_sel = t_jarr.at(index);
                break;
            }
            case QVariant::Type::Map:
            case QVariant::Type::Hash: {
                t_jobj = config_sel.toMap();
                if (!t_jobj.contains(path)) {
                    return repl;
                }
                config_sel = t_jobj.value(path);
                break;
            }
            default:
                return repl;
        }
    }
    return config_sel;
}

bool JsonConfigHandler::pathExists(QString path) const {
    return this->get(path) == QJsonValue();
}

void JsonConfigHandler::set(QString key, QJsonValue value) {
    this->configs[key] = value.toVariant();
}

}  // namespace JConfigs
