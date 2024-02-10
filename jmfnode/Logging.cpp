/*
 * file name:       Logging.cpp
 * created at:      2024/01/18
 * last modified:   2024/01/20
 * author:          lupnis<lupnisj@gmail.com>
 */

#include "Logging.h"

#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

namespace JLogs {
Logger::Logger(QHash<QString, QVariant> config) : config(config) {
#ifdef Q_OS_WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= 4;
    SetConsoleMode(hOut, dwMode);
#endif
}

Logger::~Logger() {}

QVariant Logger::getConfig(QString key) {
    if (!this->config.contains(key)) throw "invalid key: " + key.toStdString();
    return this->config[key];
}

QVariant Logger::getConfig(QString key, QVariant repl) {
    if (!this->config.contains(key)) return repl;
    return this->config[key];
}

bool Logger::setConfig(QString key, QVariant value) {
    if (this->config.contains(key)) {
        this->config[key] = value;
        return true;
    }
    return false;
}

void Logger::log(const S& content, Level level, Tag tag, bool ignore_buffer) {
    S log_content = this->make_prefix_styled(level, tag) + content + "\n";
    if (level >= this->getConfig("log_print_level").toUInt()) {
        std::cout << (std::string)log_content;
    }
    if (!ignore_buffer) {
        this->add_log_to_buffer(level, log_content);
    }
}

void Logger::debug(const S& content, Tag tag, bool ignore_buffer) {
    this->log(content, Level::DEBUG, tag, ignore_buffer);
}

void Logger::info(const S& content, Tag tag, bool ignore_buffer) {
    this->log(content, Level::INFO, tag, ignore_buffer);
}

void Logger::warn(const S& content, Tag tag, bool ignore_buffer) {
    this->log(content, Level::WARN, tag, ignore_buffer);
}

void Logger::error(const S& content, Tag tag, bool ignore_buffer) {
    this->log(content, Level::ERR, tag, ignore_buffer);
}

void Logger::critical(const S& content, Tag tag, bool ignore_buffer) {
    this->log(content, Level::CRIT, tag, ignore_buffer);
}

void Logger::flushNow() {
    this->lock.lock();
    if (this->getConfig("file_log").toBool()) {
        QFile log_file(
            this->getConfig("log_styledtored_path").toString() +
            this->getConfig("log_name").toString() + "_" +
            QDateTime().currentDateTime().toString(
                this->getConfig("log_add_date_to_styleduffix").toBool()
                    ? this->getConfig("log_filename_date_format_styledtr")
                          .toString()
                    : "") +
            "." + this->getConfig("log_styleduffix").toString());
        if (log_file.open(this->getConfig("log_append").toBool()
                              ? QIODevice::Append
                              : QIODevice::Truncate)) {
            for (QString& log : this->log_buffer) {
                log_file.write(log.toUtf8());
            }
        } else {
            this->log(S("failed to flush ") +
                          S(this->log_buffer.size(), BRIGHT_RED) +
                          "logs to disk.",
                      Level::ERR, Tag::FAILED, true);
        }
        log_file.close();
    }
    this->drop_logs();
    this->lock.unlock();
}

S Logger::make_prefix_styled(Level level, Tag tag) {
    return S(this->getConfig("display_time").toBool() ? this->make_time_styled()
                                                      : "") +
           (this->getConfig("display_time").toBool() ? " " : "") +
           S(this->getConfig("display_levels").toBool()
                 ? this->make_level_styled(level)
                 : "") +
           (this->getConfig("display_levels").toBool() ? " " : "") +
           S(this->getConfig("display_tags").toBool()
                 ? this->make_tag_styled(tag)
                 : "") +
           ((this->getConfig("display_tags").toBool() && tag != Tag::NO_TAG)
                ? " "
                : "");
}

S Logger::make_time_styled() {
    return (S((this->getConfig("time_quote").toBool()
                   ? this->getConfig("time_quote_begin").toString()
                   : ""),
              this->getConfig("time_quote_fore").toString(),
              this->getConfig("time_quote_back").toString()) +
            S(QDateTime().currentDateTime().toString(
                  this->getConfig("time_format").toString()),
              this->getConfig("time_fore").toString(),
              this->getConfig("time_back").toString()) +
            S((this->getConfig("time_quote").toBool()
                   ? this->getConfig("time_quote_end").toString()
                   : ""),
              this->getConfig("time_quote_fore").toString(),
              this->getConfig("time_quote_back").toString()));
}

S Logger::make_level_styled(Level level) {
    return S(this->getConfig(QString("level_%1_text").arg(LEVELS_MAP[level]))
                 .toString(),
             this->getConfig(QString("level_%1_fore").arg(LEVELS_MAP[level]))
                 .toString(),
             this->getConfig(QString("level_%1_back").arg(LEVELS_MAP[level]))
                 .toString());
}

S Logger::make_tag_styled(Tag tag) {
    if (tag == Tag::NO_TAG) return S();

    return S(
        this->getConfig(QString("tag_%1_text").arg(TAGS_MAP[tag])).toString(),
        this->getConfig(QString("tag_%1_fore").arg(TAGS_MAP[tag])).toString(),
        this->getConfig(QString("tag_%1_back").arg(TAGS_MAP[tag])).toString());
}

void Logger::add_log_to_buffer(Level level, QString log_content) {
    if (this->getConfig("file_log").toBool() &&
        level >= this->getConfig("file_log_level").toUInt()) {
        this->log_buffer.push_back(log_content);
        this->buffer_flush_check();
    }
}

void Logger::buffer_flush_check() {
    if (this->log_buffer.size() >
        this->getConfig("log_flush_after_n_logs").toInt()) {
        this->flushNow();
    }
}

void Logger::drop_logs() { this->log_buffer.clear(); }

}  // namespace JLogs
