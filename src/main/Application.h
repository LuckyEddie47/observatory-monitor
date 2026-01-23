#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "Config.h"
#include "ControllerManager.h"
#include "ControllerListModel.h"
#include "ControllerProxy.h"

namespace ObservatoryMonitor {

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString systemStatus READ systemStatus NOTIFY systemStatusChanged)

public:
    explicit Application(int& argc, char** argv);
    ~Application();

    int exec();

    Q_INVOKABLE ObservatoryMonitor::ControllerProxy* getController(const QString& name);

    QString systemStatus() const;

signals:
    void systemStatusChanged();

private:
    bool initialize();
    bool setupPaths();
    bool loadConfiguration();
    bool setupLogger();
    void setupControllers();
    void setupQml();

    QGuiApplication m_app;
    QQmlApplicationEngine m_engine;
    
    QString m_configDir;
    QString m_configPath;
    QString m_logDir;
    
    Config m_config;
    ControllerManager m_controllerManager;
    ControllerListModel* m_controllerListModel;
    QHash<QString, ControllerProxy*> m_proxies;
};

} // namespace ObservatoryMonitor

#endif // APPLICATION_H
