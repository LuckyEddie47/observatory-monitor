#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    qDebug() << "Observatory Monitor starting...";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    
    // QML engine will be added in Phase 7
    // For now, just verify the app runs
    
    qDebug() << "Phase 0: Basic application skeleton - OK";
    
    return 0; // Exit immediately for now
}
