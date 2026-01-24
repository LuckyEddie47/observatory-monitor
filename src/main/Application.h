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
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool showGauges READ showGauges WRITE setShowGauges NOTIFY showGaugesChanged)
    Q_PROPERTY(bool show3DView READ show3DView WRITE setShow3DView NOTIFY show3DViewChanged)
    Q_PROPERTY(int sidebarWidth READ sidebarWidth WRITE setSidebarWidth NOTIFY sidebarWidthChanged)

    // MQTT Broker Properties
    Q_PROPERTY(QString mqttHost READ mqttHost WRITE setMqttHost NOTIFY mqttHostChanged)
    Q_PROPERTY(int mqttPort READ mqttPort WRITE setMqttPort NOTIFY mqttPortChanged)
    Q_PROPERTY(QString mqttUsername READ mqttUsername WRITE setMqttUsername NOTIFY mqttUsernameChanged)
    Q_PROPERTY(QString mqttPassword READ mqttPassword WRITE setMqttPassword NOTIFY mqttPasswordChanged)
    Q_PROPERTY(double mqttTimeout READ mqttTimeout WRITE setMqttTimeout NOTIFY mqttTimeoutChanged)
    Q_PROPERTY(int reconnectInterval READ reconnectInterval WRITE setReconnectInterval NOTIFY reconnectIntervalChanged)

public:
    explicit Application(int& argc, char** argv);
    ~Application();

    int exec();

    Q_INVOKABLE ObservatoryMonitor::ControllerProxy* getController(const QString& name);
    Q_INVOKABLE void saveConfig();

    QString systemStatus() const;

    QString theme() const { return m_config.gui().theme; }
    void setTheme(const QString& theme);

    bool showGauges() const { return m_config.gui().showGauges; }
    void setShowGauges(bool show);

    bool show3DView() const { return m_config.gui().show3DView; }
    void setShow3DView(bool show);

    int sidebarWidth() const { return m_config.gui().sidebarWidth; }
    void setSidebarWidth(int width);

    // MQTT Broker Getters/Setters
    QString mqttHost() const { return m_config.broker().host; }
    void setMqttHost(const QString& host);

    int mqttPort() const { return m_config.broker().port; }
    void setMqttPort(int port);

    QString mqttUsername() const { return m_config.broker().username; }
    void setMqttUsername(const QString& username);

    QString mqttPassword() const { return m_config.broker().password; }
    void setMqttPassword(const QString& password);

    double mqttTimeout() const { return m_config.mqttTimeout(); }
    void setMqttTimeout(double timeout);

    int reconnectInterval() const { return m_config.reconnectInterval(); }
    void setReconnectInterval(int interval);

signals:
    void systemStatusChanged();
    void themeChanged();
    void showGaugesChanged();
    void show3DViewChanged();
    void sidebarWidthChanged();
    void mqttHostChanged();
    void mqttPortChanged();
    void mqttUsernameChanged();
    void mqttPasswordChanged();
    void mqttTimeoutChanged();
    void reconnectIntervalChanged();

private:
    bool initialize();
    bool setupPaths();
    bool loadConfiguration();
    bool setupLogger();
    void setupControllers();
    void setupQml();
    void updateBrokerConfig();

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
