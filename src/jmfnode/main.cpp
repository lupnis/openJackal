/*
 * file name:       main.cpp
 * created at:      2024/01/19
 * last modified:   2024/02/22
 * author:          lupnis<lupnisj@gmail.com>
 */

#include <QCoreApplication>

#include "NodeController.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);
    JConfigs::JsonConfigHandler config_handler;
    if (argc > 1 && QFile(argv[1]).exists()) {
        config_handler.setFilePath(argv[1]);
    } else if (QFile().exists("node_properties.json")) {
        config_handler.setFilePath("node_properties.json");
    } else {
        std::cout << "no config file found, create and load default config file: "
                     "node_properties.json..."
                  << std::endl;
    }
    JackalMFN::NodeController controller(config_handler);
    controller.startNode();
    return a.exec();
}
