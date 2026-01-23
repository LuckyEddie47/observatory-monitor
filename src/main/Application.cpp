#include "Application.h"
#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QQmlContext>
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

bool Application::initialize()
{
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

    connect(&m_controllerManager, &ControllerManager::controllerDataUpdated, this, [](const QString& name, const QString& command, const QString& value) {
        Logger::instance().debug(QString("[%1] %2 = %3").arg(name, command, value));
    });
}

void Application::setupQml()
{
    using namespace Qt::StringLiterals;
    m_engine.rootContext()->setContextProperty("app", this);
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
            urls.append(QUrl::fromLocalFile(fb).toString());
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
