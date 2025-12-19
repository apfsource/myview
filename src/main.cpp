#include <QApplication>
#include <QLocalSocket>
#include <QDataStream>
#include <QFile>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Global Window Icon
    a.setWindowIcon(QIcon(":/logo.png"));
    a.setApplicationName("Image Viewer");
    
    // Load Stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString qss = QLatin1String(styleFile.readAll());
        a.setStyleSheet(qss);
    }
    
    // Collect arguments (images)
    QStringList args = a.arguments();
    args.removeFirst(); // Remove program name
    
    // Attempt to connect to existing instance
    QLocalSocket socket;
    socket.connectToServer("myview_server");
    if (socket.waitForConnected(500)) {
        // Connected to existing instance: Send args
        QDataStream out(&socket);
        out << args;
        socket.waitForBytesWritten();
        return 0; // Exit this instance
    }
    
    // No existing instance: Start application (Server)
    MainWindow w;
    
    if (!args.isEmpty()) {
        for (const QString &arg : args) {
            w.openImageInNewTab(arg);
        }
    } else {
        w.showWelcomeTab();
    }
    
    w.show();
    return a.exec();
}
