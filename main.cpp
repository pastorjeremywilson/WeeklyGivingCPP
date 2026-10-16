#include <iostream>

#include "gui.h"
#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QStyleFactory>

#include "autoSelectWidgets.h"

int main(int argc, char* argv[]) {
    //qputenv("QT_DEBUG_PLUGINS", "1");
    QApplication app(argc, argv);

    // Set the default Fusion style and load the stylesheet
    app.setStyle(QStyleFactory::create("Fusion"));
    QFile file(":/resources/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString style(file.readAll());
        app.setStyleSheet(style);
    } else {
        std::cerr << "Unable to open qss file" << std::endl;
    }
    file.close();

    // Create and instance of MainWindow and show it
    MainWindow mainWindow = MainWindow();
    mainWindow.showMaximized();
    return QApplication::exec();
}
