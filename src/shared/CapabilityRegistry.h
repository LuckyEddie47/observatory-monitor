#ifndef CAPABILITYREGISTRY_H
#define CAPABILITYREGISTRY_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QObject>

namespace ObservatoryMonitor {

struct PropertyDefinition {
    QString name;
    QString command;
    QString description;
    QString unit;
    
    // Hint for UI: "numeric", "string", "binary"
    QString type;
};

class CapabilityRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList allPropertyLinks READ allPropertyLinks NOTIFY capabilitiesChanged)
public:
    explicit CapabilityRegistry(QObject* parent = nullptr);

    // Add/Update capabilities for a controller type
    void registerProperties(const QString& controllerType, const QList<PropertyDefinition>& properties);
    
    // Get properties for a controller type
    QList<PropertyDefinition> getProperties(const QString& controllerType) const;
    
    // Get property names for a controller type (for UI selection)
    QStringList getPropertyNames(const QString& controllerType) const;
    
    // Get all property links (e.g. "Telescope.Azimuth")
    QStringList allPropertyLinks() const;

    // Find property by name
    Q_INVOKABLE QVariantMap getProperty(const QString& controllerType, const QString& propertyName) const;

    // Load/Save to YAML
    bool loadFromFile(const QString& filePath, QString& errorMessage);
    bool saveToFile(const QString& filePath, QString& errorMessage);
    
    void setDefaults();

signals:
    void capabilitiesChanged();

private:
    QHash<QString, QList<PropertyDefinition>> m_capabilities;
};

} // namespace ObservatoryMonitor

#endif // CAPABILITYREGISTRY_H
