#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "MQTT Simulator starting...";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    
    // Simulator logic will be added in later phases
    
    qDebug() << "Phase 0: Basic simulator skeleton - OK";
    
    return 0; // Exit immediately for now
}
