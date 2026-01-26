#include "CapabilityRegistry.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDebug>
#include <QVariant>

namespace ObservatoryMonitor {

CapabilityRegistry::CapabilityRegistry(QObject* parent)
    : QObject(parent)
{
    setDefaults();
}

void CapabilityRegistry::registerProperties(const QString& controllerType, const QList<PropertyDefinition>& properties)
{
    m_capabilities[controllerType] = properties;
    emit capabilitiesChanged();
}

QList<PropertyDefinition> CapabilityRegistry::getProperties(const QString& controllerType) const
{
    return m_capabilities.value(controllerType);
}

QStringList CapabilityRegistry::getPropertyNames(const QString& controllerType) const
{
    QStringList names;
    for (const auto& prop : m_capabilities.value(controllerType)) {
        names << prop.name;
    }
    return names;
}

QStringList CapabilityRegistry::allPropertyLinks() const
{
    QStringList links;
    for (auto it = m_capabilities.begin(); it != m_capabilities.end(); ++it) {
        for (const auto& prop : it.value()) {
            links << it.key() + "." + prop.name;
        }
    }
    links.sort();
    return links;
}

QVariantMap CapabilityRegistry::getProperty(const QString& controllerType, const QString& propertyName) const
{
    for (const auto& prop : m_capabilities.value(controllerType)) {
        if (prop.name == propertyName) {
            QVariantMap map;
            map["name"] = prop.name;
            map["command"] = prop.command;
            map["description"] = prop.description;
            map["unit"] = prop.unit;
            map["type"] = prop.type;
            return map;
        }
    }
    return QVariantMap();
}

void CapabilityRegistry::setDefaults()
{
    m_capabilities.clear();

    // Observatory defaults
    QList<PropertyDefinition> obsProps;
    obsProps << PropertyDefinition{"Azimuth", ":GZ#", "Dome Azimuth", "deg", "numeric"};
    obsProps << PropertyDefinition{"Altitude", ":GA#", "Dome Altitude", "deg", "numeric"};
    obsProps << PropertyDefinition{"Shutter", ":RS#", "Shutter Status", "", "binary"};
    m_capabilities["Observatory"] = obsProps;

    // Telescope defaults
    QList<PropertyDefinition> telProps;
    telProps << PropertyDefinition{"Azimuth", ":GZ#", "Mount Azimuth", "deg", "numeric"};
    telProps << PropertyDefinition{"Altitude", ":GA#", "Mount Altitude", "deg", "numeric"};
    telProps << PropertyDefinition{"RA", ":GR#", "Right Ascension", "hrs", "numeric"};
    telProps << PropertyDefinition{"Dec", ":GD#", "Declination", "deg", "numeric"};
    telProps << PropertyDefinition{"PierSide", ":GS#", "Side of Pier", "", "binary"};
    m_capabilities["Telescope"] = telProps;
    
    emit capabilitiesChanged();
}

bool CapabilityRegistry::loadFromFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Node config = YAML::LoadFile(filePath.toStdString());
        if (!config["capabilities"]) return true;

        m_capabilities.clear();
        YAML::Node caps = config["capabilities"];
        
        for (auto it = caps.begin(); it != caps.end(); ++it) {
            QString type = QString::fromStdString(it->first.as<std::string>());
            YAML::Node props = it->second;
            QList<PropertyDefinition> propList;
            
            for (std::size_t i = 0; i < props.size(); ++i) {
                YAML::Node p = props[i];
                PropertyDefinition def;
                if (p["name"]) def.name = QString::fromStdString(p["name"].as<std::string>());
                if (p["command"]) def.command = QString::fromStdString(p["command"].as<std::string>());
                if (p["description"]) def.description = QString::fromStdString(p["description"].as<std::string>());
                if (p["unit"]) def.unit = QString::fromStdString(p["unit"].as<std::string>());
                if (p["type"]) def.type = QString::fromStdString(p["type"].as<std::string>());
                propList << def;
            }
            m_capabilities[type] = propList;
        }
        emit capabilitiesChanged();
        return true;
    } catch (const std::exception& e) {
        errorMessage = QString::fromStdString(e.what());
        return false;
    }
}

bool CapabilityRegistry::saveToFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "capabilities" << YAML::Value << YAML::BeginMap;
        
        for (auto it = m_capabilities.begin(); it != m_capabilities.end(); ++it) {
            out << YAML::Key << it.key().toStdString() << YAML::Value << YAML::BeginSeq;
            for (const auto& prop : it.value()) {
                out << YAML::BeginMap;
                out << YAML::Key << "name" << YAML::Value << prop.name.toStdString();
                out << YAML::Key << "command" << YAML::Value << prop.command.toStdString();
                if (!prop.description.isEmpty()) out << YAML::Key << "description" << YAML::Value << prop.description.toStdString();
                if (!prop.unit.isEmpty()) out << YAML::Key << "unit" << YAML::Value << prop.unit.toStdString();
                if (!prop.type.isEmpty()) out << YAML::Key << "type" << YAML::Value << prop.type.toStdString();
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        
        out << YAML::EndMap << YAML::EndMap;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            errorMessage = file.errorString();
            return false;
        }
        file.write(out.c_str());
        return true;
    } catch (const std::exception& e) {
        errorMessage = QString::fromStdString(e.what());
        return false;
    }
}

} // namespace ObservatoryMonitor
