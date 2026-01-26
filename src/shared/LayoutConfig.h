#ifndef LAYOUTCONFIG_H
#define LAYOUTCONFIG_H

#include <QString>
#include <QList>
#include <QVector3D>
#include <QObject>
#include "ValueMappingEngine.h"

namespace ObservatoryMonitor {

struct WidgetConfig {
    QString type;
    QString id;
    QString label;
    double x = 0;
    double y = 0;
    QString propertyLink; // e.g. "Telescope.Azimuth"
    MappingDefinition mapping;
};

struct SceneNodeConfig {
    QString model;
    QString id;
    QString parentId;
    QVector3D offset;
    
    struct Motion {
        QString type; // "rotation", "linear", "none"
        QVector3D axis;
        QString propertyLink;
        MappingDefinition mapping;
    } motion;
};

class CapabilityRegistry;

class LayoutConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList widgets READ widgetsVariant NOTIFY widgetsChanged)
    Q_PROPERTY(QVariantList sceneNodes READ sceneNodesVariant NOTIFY sceneNodesChanged)
    Q_PROPERTY(QString backgroundSource READ backgroundSource WRITE setBackgroundSource NOTIFY backgroundSourceChanged)
    Q_PROPERTY(QString backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY validationChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY validationChanged)
    
public:
    explicit LayoutConfig(QObject* parent = nullptr);
    
    bool loadFromFile(const QString& filePath, QString& errorMessage);
    bool saveToFile(const QString& filePath, QString& errorMessage);
    bool validate(const CapabilityRegistry* caps, QString& errorMessage);
    
    void setDefaults();
    void setDefaultWidgets();
    void setDefaultScene();

    QString backgroundSource() const { return m_backgroundSource; }
    void setBackgroundSource(const QString& source);

    QString backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QString& color);

    bool isValid() const { return m_isValid; }
    QString validationError() const { return m_validationError; }

    QList<WidgetConfig> widgets() const { return m_widgets; }
    QVariantList widgetsVariant() const;
    
    QList<SceneNodeConfig> sceneNodes() const { return m_sceneNodes; }
    QVariantList sceneNodesVariant() const;
    
    Q_INVOKABLE void addWidget(const QVariantMap& config);
    Q_INVOKABLE void removeWidget(const QString& id);
    Q_INVOKABLE void updateWidget(const QString& id, const QVariantMap& config);

    Q_INVOKABLE void addSceneNode(const QVariantMap& config);
    Q_INVOKABLE void removeSceneNode(const QString& id);
    Q_INVOKABLE void updateSceneNode(const QString& id, const QVariantMap& config);
    Q_INVOKABLE void clear();

signals:
    void widgetsChanged();
    void sceneNodesChanged();
    void backgroundSourceChanged();
    void backgroundColorChanged();
    void validationChanged();

private:
    QList<WidgetConfig> m_widgets;
    QList<SceneNodeConfig> m_sceneNodes;
    QString m_backgroundSource;
    QString m_backgroundColor;
    bool m_isValid = true;
    QString m_validationError;
};

} // namespace ObservatoryMonitor

#endif // LAYOUTCONFIG_H
