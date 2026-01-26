#include "Application.h"
#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QQmlContext>
#include <QtQml>
#include <iostream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace ObservatoryMonitor {

Application::Application(int& argc, char** argv)
    : m_app(argc, argv)
    , m_controllerListModel(nullptr)
{
    QCoreApplication::setApplicationName("observatory-monitor");
    QCoreApplication::setApplicationVersion("0.1.0");
}

Application::~Application()
{
    Logger::instance().shutdown();
}

int Application::exec()
{
    if (!initialize()) {
        return 1;
    }

    return m_app.exec();
}

ControllerProxy* Application::getController(const QString& name)
{
    if (m_proxies.contains(name)) {
        return m_proxies[name];
    }

    if (m_controllerManager.getControllerNames().contains(name)) {
        ControllerProxy* proxy = new ControllerProxy(name, &m_controllerManager, this);
        m_proxies[name] = proxy;
        return proxy;
    }

    return nullptr;
}

void Application::saveConfig()
{
    QString error;
    if (!m_config.saveToFile(m_configPath, error)) {
        Logger::instance().error("Failed to save configuration: " + error);
    } else {
        Logger::instance().info("Configuration saved successfully to " + m_configPath);
    }
}

void Application::saveLayout()
{
    QString error;
    if (!m_layout.saveToFile(m_layoutPath, error)) {
        Logger::instance().error("Failed to save layout: " + error);
    } else {
        Logger::instance().info("Layout saved successfully to " + m_layoutPath);
    }
}

void Application::setTheme(const QString& theme)
{
    if (m_config.gui().theme != theme) {
        GuiConfig gui = m_config.gui();
        gui.theme = theme;
        m_config.setGui(gui);
        emit themeChanged();
    }
}

void Application::setShowGauges(bool show)
{
    if (m_config.gui().showGauges != show) {
        GuiConfig gui = m_config.gui();
        gui.showGauges = show;
        m_config.setGui(gui);
        emit showGaugesChanged();
    }
}

void Application::setShow3DView(bool show)
{
    if (m_config.gui().show3DView != show) {
        GuiConfig gui = m_config.gui();
        gui.show3DView = show;
        m_config.setGui(gui);
        emit show3DViewChanged();
    }
}

void Application::setShowDashboard(bool show)
{
    if (m_showDashboard != show) {
        m_showDashboard = show;
        emit showDashboardChanged();
    }
}

void Application::setEditorMode(bool mode)
{
    if (m_editorMode != mode) {
        m_editorMode = mode;
        emit editorModeChanged();
    }
}

void Application::setSidebarWidth(int width)
{
    if (m_config.gui().sidebarWidth != width) {
        GuiConfig gui = m_config.gui();
        gui.sidebarWidth = width;
        m_config.setGui(gui);
        emit sidebarWidthChanged();
    }
}

void Application::setSidebarPosition(const QString& position)
{
    if (m_config.gui().sidebarPosition != position) {
        GuiConfig gui = m_config.gui();
        gui.sidebarPosition = position;
        m_config.setGui(gui);
        emit sidebarPositionChanged();
    }
}

void Application::setMqttHost(const QString& host)
{
    if (m_config.broker().host != host) {
        BrokerConfig broker = m_config.broker();
        broker.host = host;
        m_config.setBroker(broker);
        emit mqttHostChanged();
        updateBrokerConfig();
    }
}

void Application::setMqttPort(int port)
{
    if (m_config.broker().port != port) {
        BrokerConfig broker = m_config.broker();
        broker.port = port;
        m_config.setBroker(broker);
        emit mqttPortChanged();
        updateBrokerConfig();
    }
}

void Application::setMqttUsername(const QString& username)
{
    if (m_config.broker().username != username) {
        BrokerConfig broker = m_config.broker();
        broker.username = username;
        m_config.setBroker(broker);
        emit mqttUsernameChanged();
        updateBrokerConfig();
    }
}

void Application::setMqttPassword(const QString& password)
{
    if (m_config.broker().password != password) {
        BrokerConfig broker = m_config.broker();
        broker.password = password;
        m_config.setBroker(broker);
        emit mqttPasswordChanged();
        updateBrokerConfig();
    }
}

void Application::setMqttTimeout(double timeout)
{
    if (m_config.mqttTimeout() != timeout) {
        m_config.setMqttTimeout(timeout);
        emit mqttTimeoutChanged();
        updateBrokerConfig();
    }
}

void Application::setReconnectInterval(int interval)
{
    if (m_config.reconnectInterval() != interval) {
        m_config.setReconnectInterval(interval);
        emit reconnectIntervalChanged();
        updateBrokerConfig();
    }
}

void Application::updateBrokerConfig()
{
    m_controllerManager.updateBrokerConfig(m_config.broker(), m_config.mqttTimeout(), m_config.reconnectInterval());
}

bool Application::initialize()
{
    qmlRegisterType<ControllerProxy>("ObservatoryMonitor", 1, 0, "ControllerProxy");
    qmlRegisterUncreatableType<CapabilityRegistry>("ObservatoryMonitor", 1, 0, "CapabilityRegistry", "Access through app.caps");
    qmlRegisterUncreatableType<LayoutConfig>("ObservatoryMonitor", 1, 0, "LayoutConfig", "Access through app.layout");
    qmlRegisterUncreatableType<ValueMappingEngine>("ObservatoryMonitor", 1, 0, "ValueMappingEngine", "Access through app.valueMappingEngine");

    if (!setupPaths()) return false;
    if (!loadConfiguration()) return false;
    if (!setupLogger()) return false;

    Logger::instance().info("=================================================");
    Logger::instance().info("Observatory Monitor starting...");
    Logger::instance().info(QString("Qt version: %1").arg(QT_VERSION_STR));
    Logger::instance().info("=================================================");

    setupControllers();
    setupQml();

    m_controllerManager.connectAll();
    m_controllerManager.startPolling(1000, 5000);
    
    return true;
}

bool Application::setupPaths()
{
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_configPath = m_configDir + "/config.yaml";
    m_layoutPath = m_configDir + "/layout.yaml";
    m_capsPath = m_configDir + "/capabilities.yaml";

#ifdef Q_OS_LINUX
    if (getuid() == 0) {
        m_logDir = "/var/log/observatory-monitor";
    } else {
        QDir varLogDir("/var/log/observatory-monitor");
        QFileInfo varLogInfo("/var/log/observatory-monitor");
        if (varLogDir.exists() && varLogInfo.isWritable()) {
            m_logDir = "/var/log/observatory-monitor";
        } else {
            m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
        }
    }
#else
    m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
#endif

    QDir dir;
    if (!dir.exists(m_configDir)) {
        if (!dir.mkpath(m_configDir)) {
            std::cerr << "Failed to create config directory: " << m_configDir.toStdString() << std::endl;
            return false;
        }
    }

    if (!dir.exists(m_logDir)) {
        if (!dir.mkpath(m_logDir)) {
            std::cerr << "Failed to create log directory: " << m_logDir.toStdString() << std::endl;
            return false;
        }
    }

    return true;
}

bool Application::loadConfiguration()
{
    QString errorMessage;
    QFile configFile(m_configPath);

    if (!configFile.exists()) {
        m_config.setDefaults();
        if (!m_config.saveToFile(m_configPath, errorMessage)) {
            std::cerr << "Failed to create default config file: " << errorMessage.toStdString() << std::endl;
            return false;
        }
    } else {
        if (!m_config.loadFromFile(m_configPath, errorMessage)) {
            std::cerr << "Failed to load config file: " << errorMessage.toStdString() << std::endl;
            return false;
        }
    }

    // Load capabilities
    if (!QFile::exists(m_capsPath)) {
        m_capabilities.setDefaults();
        m_capabilities.saveToFile(m_capsPath, errorMessage);
    } else {
        m_capabilities.loadFromFile(m_capsPath, errorMessage);
    }

    // Load layout
    if (!QFile::exists(m_layoutPath)) {
        m_layout.setDefaults();
        m_layout.saveToFile(m_layoutPath, errorMessage);
    } else {
        if (!m_layout.loadFromFile(m_layoutPath, errorMessage)) {
            std::cerr << "Failed to load layout file: " << errorMessage.toStdString() << std::endl;
            // Don't return false, just use defaults
            m_layout.setDefaults();
        } else if (!m_layout.validate(&m_capabilities, errorMessage)) {
            Logger::instance().error("Layout validation failed: " + errorMessage);
            // We don't revert to defaults here so the user can see the error banner 
            // and fix the layout in the editor.
        }
    }

    if (!m_config.validate(errorMessage)) {
        std::cerr << "Configuration validation failed: " << errorMessage.toStdString() << std::endl;
        return false;
    }

    return true;
}

bool Application::setupLogger()
{
    if (!Logger::instance().initialize(m_logDir,
                                      m_config.logging().debugEnabled,
                                      true,
                                      m_config.logging().maxTotalSizeMB)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return false;
    }
    return true;
}

QString Application::systemStatus() const
{
    SystemStatus s = m_controllerManager.getSystemStatus();
    switch (s) {
        case SystemStatus::AllConnected: return "All Connected";
        case SystemStatus::PartiallyConnected: return "Partially Connected";
        case SystemStatus::Disconnected: return "Disconnected";
        default: return "Unknown";
    }
}

void Application::setupControllers()
{
    m_controllerManager.loadControllersFromConfig(m_config);
    m_controllerListModel = new ControllerListModel(&m_controllerManager, this);
    
    connect(&m_controllerManager, &ControllerManager::systemStatusChanged, this, [this](SystemStatus status) {
        QString statusStr;
        switch (status) {
            case SystemStatus::AllConnected: statusStr = "AllConnected"; break;
            case SystemStatus::PartiallyConnected: statusStr = "PartiallyConnected"; break;
            case SystemStatus::Disconnected: statusStr = "Disconnected"; break;
        }
        Logger::instance().info(QString("System Status Changed: %1").arg(statusStr));
        emit systemStatusChanged();
    });

    connect(&m_controllerManager, &ControllerManager::controllerStatusChanged, this, [](const QString& name, ControllerStatus status) {
        Logger::instance().info(QString("Controller '%1' status changed").arg(name));
    });

    connect(&m_controllerManager, &ControllerManager::controllerEnabledChanged, this, [this](const QString& name, bool enabled) {
        QList<ControllerConfig> ctrls = m_config.controllers();
        for (int i = 0; i < ctrls.size(); ++i) {
            if (ctrls[i].name == name) {
                ctrls[i].enabled = enabled;
                break;
            }
        }
        m_config.setControllers(ctrls);
        saveConfig();
    });

    connect(&m_controllerManager, &ControllerManager::controllerDataUpdated, this, [](const QString& name, const QString& command, const QString& value) {
        Logger::instance().debug(QString("[%1] %2 = %3").arg(name, command, value));
    });
}

void Application::setupQml()
{
    using namespace Qt::StringLiterals;
    m_engine.rootContext()->setContextProperty("app", this);
    m_engine.rootContext()->setContextProperty("caps", &m_capabilities);
    m_engine.rootContext()->setContextProperty("layout", &m_layout);
    m_engine.rootContext()->setContextProperty("valueMappingEngine", &m_valueMappingEngine);
    m_engine.rootContext()->setContextProperty("controllerModel", m_controllerListModel);
    
    // List of potential paths to try
    QStringList urls = {
        "qrc:/qt/qml/ObservatoryMonitor/main.qml",
        "qrc:/qt/resources/qml/main.qml",
        "qrc:/ObservatoryMonitor/main.qml",
        "qrc:/main.qml"
    };
    
    // Add local fallbacks
    QStringList fallbacks = {
        QCoreApplication::applicationDirPath() + "/../resources/qml/main.qml",
        "resources/qml/main.qml",
        "../resources/qml/main.qml"
    };
    
    for (const QString& fb : fallbacks) {
        if (QFile::exists(fb)) {
            QString url = QUrl::fromLocalFile(fb).toString();
            urls.append(url);
            
            // Add the directory to import paths
            QFileInfo info(fb);
            m_engine.addImportPath(info.absolutePath());
            m_engine.addImportPath(info.absolutePath() + "/widgets");
        }
    }

    connect(&m_engine, &QQmlApplicationEngine::objectCreated,
        &m_app, [this](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                Logger::instance().error(QString("Failed to create QML object from %1").arg(objUrl.toString()));
                // If this was our last attempt and we still have no root objects, exit
            } else {
                Logger::instance().info(QString("Successfully loaded QML from %1").arg(objUrl.toString()));
            }
        }, Qt::QueuedConnection);
    
    // Try to load them one by one
    for (const QString& urlStr : urls) {
        Logger::instance().info(QString("Attempting to load QML from: %1").arg(urlStr));
        m_engine.load(QUrl(urlStr));
        if (!m_engine.rootObjects().isEmpty()) {
            break;
        }
    }
    
    // Final check
    QTimer::singleShot(100, [this]() {
        if (m_engine.rootObjects().isEmpty()) {
            Logger::instance().error("All QML load attempts failed. Exiting.");
            QCoreApplication::exit(-1);
        }
    });
}

} // namespace ObservatoryMonitor
