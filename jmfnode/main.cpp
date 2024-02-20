/*
 * file name:       main.cpp
 * created at:      2024/01/19
 * last modified:   2024/02/20
 * author:          lupnis<lupnisj@gmail.com>
 */

#include <QCoreApplication>

#include "NodeController.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);
    JConfigs::JsonConfigHandler config_handler;
    if(argc > 1 && QFile(argv[1]).exists()) {
        config_handler.setFilePath(argv[1]);
    } else {
        qDebug() << "no config file found, load default config file: node_properties.json...";
    }
    JackalMFN::NodeController controller(config_handler);
    controller.startNode();
    return a.exec();
}
