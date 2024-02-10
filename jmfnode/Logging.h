/*
 * file name:       Logging.h
 * created at:      2024/01/18
 * last modified:   2024/01/20
 * author:          lupnis<lupnisj@gmail.com>
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QIODevice>
#include <QMutex>
#include <QObject>
#include <QVariant>
#include <QtCore>
#include <iostream>

namespace JLogs {

#ifndef ANSI_DEFINES
#define ANSI_DEFINES

#define ANSI_ESCAPE "\x1B["
#define ANSI_END "m"
#define ANSI_SEP ";"
#define CLEAR ANSI_ESCAPE "0" ANSI_END
#define BOLD ANSI_ESCAPE "1" ANSI_END
#define BOLD_RESET ANSI_ESCAPE "22" ANSI_END
#define FAINT ANSI_ESCAPE "2" ANSI_END
#define FAINT_RESET ANSI_ESCAPE "22" ANSI_END
#define ITALIC ANSI_ESCAPE "3" ANSI_END
#define ITALIC_RESET ANSI_ESCAPE "23" ANSI_END
#define UNDERLINE ANSI_ESCAPE "4" ANSI_END
#define UNDERLINE_RESET ANSI_ESCAPE "24" ANSI_END
#define BLINK ANSI_ESCAPE "5" ANSI_END
#define BLINK_RESET ANSI_ESCAPE "25" ANSI_END
#define REVERSE ANSI_ESCAPE "7" ANSI_END
#define REVERSE_RESET ANSI_ESCAPE "27" ANSI_END
#define INVISIBLE ANSI_ESCAPE "8" ANSI_END
#define INVISIBLE_RESET ANSI_ESCAPE "28" ANSI_END
#define STRIKE ANSI_ESCAPE "9" ANSI_END
#define STRIKE_RESET ANSI_ESCAPE "29" ANSI_END
#define DEFAULT ANSI_ESCAPE "39" ANSI_END
#define DEFAULT_BG ANSI_ESCAPE "49" ANSI_END
#define BLACK ANSI_ESCAPE "30" ANSI_END
#define BLACK_BG ANSI_ESCAPE "40" ANSI_END
#define RED ANSI_ESCAPE "31" ANSI_END
#define RED_BG ANSI_ESCAPE "41" ANSI_END
#define GREEN ANSI_ESCAPE "32" ANSI_END
#define GREEN_BG ANSI_ESCAPE "42" ANSI_END
#define YELLOW ANSI_ESCAPE "33" ANSI_END
#define YELLOW_BG ANSI_ESCAPE "43" ANSI_END
#define BLUE ANSI_ESCAPE "34" ANSI_END
#define BLUE_BG ANSI_ESCAPE "44" ANSI_END
#define MAGENTA ANSI_ESCAPE "35" ANSI_END
#define MAGENTA_BG ANSI_ESCAPE "45" ANSI_END
#define CYAN ANSI_ESCAPE "36" ANSI_END
#define CYAN_BG ANSI_ESCAPE "46" ANSI_END
#define WHITE ANSI_ESCAPE "37" ANSI_END
#define WHITE_BG ANSI_ESCAPE "47" ANSI_END
#define BRIGHT_BLACK ANSI_ESCAPE "90" ANSI_END
#define BRIGHT_BLACK_BG ANSI_ESCAPE "100" ANSI_END
#define BRIGHT_RED ANSI_ESCAPE "91" ANSI_END
#define BRIGHT_RED_BG ANSI_ESCAPE "101" ANSI_END
#define BRIGHT_GREEN ANSI_ESCAPE "92" ANSI_END
#define BRIGHT_GREEN_BG ANSI_ESCAPE "102" ANSI_END
#define BRIGHT_YELLOW ANSI_ESCAPE "93" ANSI_END
#define BRIGHT_YELLOW_BG ANSI_ESCAPE "103" ANSI_END
#define BRIGHT_BLUE ANSI_ESCAPE "94" ANSI_END
#define BRIGHT_BLUE_BG ANSI_ESCAPE "104" ANSI_END
#define BRIGHT_MAGENTA ANSI_ESCAPE "95" ANSI_END
#define BRIGHT_MAGENTA_BG ANSI_ESCAPE "105" ANSI_END
#define BRIGHT_CYAN ANSI_ESCAPE "96" ANSI_END
#define BRIGHT_CYAN_BG ANSI_ESCAPE "106" ANSI_END
#define BRIGHT_WHITE ANSI_ESCAPE "97" ANSI_END
#define BRIGHT_WHITE_BG ANSI_ESCAPE "107" ANSI_END
#define IDCOLOR(id) (ANSI_ESCAPE "38" ANSI_SEP "5" ANSI_SEP id ANSI_END)
#define IDCOLOR_BG(id) (ANSI_ESCAPE "48" ANSI_SEP "5" ANSI_SEP id ANSI_END)
#define RGBCOLOR(r, g, b) \
    (ANSI_ESCAPE "38" ANSI_SEP "2" ANSI_SEP r ANSI_SEP g ANSI_SEP b ANSI_END)
#define RGBCOLOR_BG(r, g, b) \
    (ANSI_ESCAPE "48" ANSI_SEP "2" ANSI_SEP r ANSI_SEP g ANSI_SEP b ANSI_END)
#endif

struct S {
    QString rawStr, styStr;

    S() {}
    S(const S &rvalue) {
        this->rawStr = rvalue.rawStr;
        this->styStr = rvalue.styStr;
    }
    template <typename T>
    S(const T &content) {
        this->rawStr = this->styStr = QVariant(content).toString();
    }
    template <typename T, typename... Styles>
    S(const T &content, const Styles &...styles) {
        this->rawStr = QVariant(content).toString();
        for (const QString &style : {styles...}) {
            this->styStr += style;
        }
        this->styStr += QVariant(content).toString() + CLEAR;
    }

    S operator+(const S &rvalue) {
        S new_styled;
        new_styled.rawStr = this->rawStr + rvalue.rawStr;
        new_styled.styStr = this->styStr + rvalue.styStr;
        return new_styled;
    }
    template <typename T>
    S operator+(const T &rvalue) {
        S new_styled;
        new_styled.rawStr = this->rawStr + QVariant(rvalue).toString();
        new_styled.styStr = this->styStr + QVariant(rvalue).toString();
        return new_styled;
    }
    template <typename T>
    S operator<<(const T &rvalue) {
        S new_styled;
        new_styled.rawStr = this->rawStr + QVariant(rvalue).toString();
        new_styled.styStr = this->styStr + QVariant(rvalue).toString();
        return new_styled;
    }
    S operator=(const S &rvalue) {
        this->rawStr = rvalue.rawStr;
        this->styStr = rvalue.styStr;
        return *this;
    }
    template <typename T>
    S operator=(const T &rvalue) {
        this->rawStr = this->styStr = QVariant(rvalue).toString();
        return *this;
    }

    operator std::string() const { return this->styStr.toStdString(); }
    operator QString() const { return this->rawStr; }
};

typedef S S;

enum Level { DEBUG, INFO, WARN, ERR, CRIT };

enum Tag {
    NO_TAG,
    SUCCEEDED,
    DONE,
    FAILED,
    PROGRESS,
    CANCELED,
    QUESTION,
    DANGER
};

const QHash<QString, QVariant> DEFAULT_LOG_CONFIG{
    {"colored_display", true},

    {"display_time", true},
    {"time_format", "yyyy-MM-dd hh:mm:ss"},
    {"time_fore", BRIGHT_BLACK},
    {"time_back", DEFAULT_BG},

    {"time_quote", true},
    {"time_quote_begin", "["},
    {"time_quote_end", "]"},
    {"time_quote_fore", DEFAULT},
    {"time_quote_back", DEFAULT_BG},

    {"display_levels", true},
    {"level_debug_fore", BRIGHT_BLACK},
    {"level_debug_back", DEFAULT_BG},
    {"level_debug_text", "DEBUG"},
    {"level_info_fore", WHITE},
    {"level_info_back", DEFAULT_BG},
    {"level_info_text", "INFO"},
    {"level_warn_fore", YELLOW},
    {"level_warn_back", DEFAULT_BG},
    {"level_warn_text", "WARN"},
    {"level_err_fore", RED BOLD},
    {"level_err_back", DEFAULT_BG},
    {"level_err_text", "ERROR"},
    {"level_crit_fore", MAGENTA BOLD BLINK},
    {"level_crit_back", DEFAULT_BG},
    {"level_crit_text", "CRIT"},

    {"display_tags", true},
    {"tag_succeeded_fore", GREEN},
    {"tag_succeeded_back", DEFAULT_BG},
    {"tag_succeeded_text", "SUCCEEDED"},
    {"tag_done_fore", GREEN},
    {"tag_done_back", DEFAULT_BG},
    {"tag_done_text", "DONE"},
    {"tag_failed_fore", RED},
    {"tag_failed_back", DEFAULT_BG},
    {"tag_failed_text", "FAILED"},
    {"tag_progress_fore", IDCOLOR("36")},
    {"tag_progress_back", DEFAULT_BG},
    {"tag_progress_text", "PROGRESS"},
    {"tag_canceled_fore", IDCOLOR("81")},
    {"tag_canceled_back", DEFAULT_BG},
    {"tag_canceled_text", "CANCELED"},
    {"tag_question_fore", IDCOLOR("226")},
    {"tag_question_back", DEFAULT_BG},
    {"tag_question_text", "QUESTION"},
    {"tag_danger_fore", BLACK},
    {"tag_danger_back", RED_BG},
    {"tag_danger_text", " DANGER "},

    {"file_log", true},
    {"file_log_level", Level::INFO},
    {"log_print_level", Level::INFO},
    {"log_stored_path", "./"},
    {"log_name", "logs"},
    {"log_append", true},
    {"log_add_date_to_suffix", true},
    {"log_filename_date_format_str", "yyyy_MM_dd"},
    {"log_suffix", "txt"},
    {"log_flush_after_n_logs", 0}};

const QHash<Level, QString> LEVELS_MAP{{Level::DEBUG, "debug"},
                                       {Level::INFO, "info"},
                                       {Level::WARN, "warn"},
                                       {Level::ERR, "err"},
                                       {Level::CRIT, "crit"}};

const QHash<Tag, QString> TAGS_MAP{
    {Tag::SUCCEEDED, "succeeded"}, {Tag::DONE, "done"},
    {Tag::FAILED, "failed"},       {Tag::PROGRESS, "progress"},
    {Tag::CANCELED, "canceled"},   {Tag::QUESTION, "question"},
    {Tag::DANGER, "danger"}};

class Logger {
   public:
    Logger(QHash<QString, QVariant> config = DEFAULT_LOG_CONFIG);
    ~Logger();

    QVariant getConfig(QString key);
    QVariant getConfig(QString key, QVariant repl);
    bool setConfig(QString key, QVariant value);

    void log(const S &content, Level level = Level::INFO, Tag tag = Tag::NO_TAG,
             bool ignore_buffer = false);
    void debug(const S &content, Tag tag = Tag::NO_TAG,
               bool ignore_buffer = false);
    void info(const S &content, Tag tag = Tag::NO_TAG,
              bool ignore_buffer = false);
    void warn(const S &content, Tag tag = Tag::NO_TAG,
              bool ignore_buffer = false);
    void error(const S &content, Tag tag = Tag::NO_TAG,
               bool ignore_buffer = false);
    void critical(const S &content, Tag tag = Tag::NO_TAG,
                  bool ignore_buffer = false);
    void flushNow();

   private:
    S make_level_styled(Level level);
    S make_prefix_styled(Level level, Tag tag);
    S make_time_styled();
    S make_tag_styled(Tag tag);

    void add_log_to_buffer(Level level, QString log_content);
    void buffer_flush_check();
    void drop_logs();

   private:
    QHash<QString, QVariant> config;
    QList<QString> log_buffer;
    QFile log_io;

    QMutex lock;
};

static JLogs::Logger globalLogger;

}  // namespace JLogs

#endif