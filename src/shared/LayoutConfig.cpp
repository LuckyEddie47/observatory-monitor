#include "LayoutConfig.h"
#include "CapabilityRegistry.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QSet>
#include <QFileInfo>
#include <QDebug>

namespace ObservatoryMonitor {

LayoutConfig::LayoutConfig(QObject* parent)
    : QObject(parent)
{
    setDefaults();
}

void LayoutConfig::setDefaults()
{
    m_backgroundSource = "";
    m_backgroundColor = "transparent";
    setDefaultWidgets();
    setDefaultScene();
}

void LayoutConfig::setBackgroundSource(const QString& source)
{
    if (m_backgroundSource != source) {
        m_backgroundSource = source;
        emit backgroundSourceChanged();
    }
}

void LayoutConfig::setBackgroundColor(const QString& color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit backgroundColorChanged();
    }
}

void LayoutConfig::setDefaultWidgets()
{
    m_widgets.clear();
    
    WidgetConfig w1;
    w1.type = "Numeric";
    w1.id = "dome_az";
    w1.label = "Dome Azimuth";
    w1.x = 50;
    w1.y = 50;
    w1.propertyLink = "Observatory.Azimuth";
    m_widgets.append(w1);
    
    WidgetConfig w2;
    w2.type = "Numeric";
    w2.id = "mount_az";
    w2.label = "Mount Azimuth";
    w2.x = 200;
    w2.y = 50;
    w2.propertyLink = "Telescope.Azimuth";
    m_widgets.append(w2);
    
    WidgetConfig w3;
    w3.type = "Numeric";
    w3.id = "mount_alt";
    w3.label = "Mount Altitude";
    w3.x = 50;
    w3.y = 150;
    w3.propertyLink = "Telescope.Altitude";
    m_widgets.append(w3);
}

void LayoutConfig::setDefaultScene()
{
    m_sceneNodes.clear();

    // Default 3D Scene setup
    SceneNodeConfig dome;
    dome.id = "dome";
    dome.model = "Dome.qml";
    dome.motion.type = "rotation";
    dome.motion.axis = QVector3D(0, 1, 0);
    dome.motion.propertyLink = "Observatory.Azimuth";
    dome.motion.mapping.type = "linear";
    dome.motion.mapping.inMin = 0;
    dome.motion.mapping.inMax = 360;
    dome.motion.mapping.outMin = 0;
    dome.motion.mapping.outMax = -360;
    m_sceneNodes.append(dome);

    SceneNodeConfig pier;
    pier.id = "pier";
    pier.model = "Pier.qml";
    m_sceneNodes.append(pier);

    SceneNodeConfig mount;
    mount.id = "mount";
    mount.model = "MountAzimuth.qml";
    mount.motion.type = "rotation";
    mount.motion.axis = QVector3D(0, 1, 0);
    mount.motion.propertyLink = "Telescope.Azimuth";
    mount.motion.mapping.type = "linear";
    mount.motion.mapping.inMin = 0;
    mount.motion.mapping.inMax = 360;
    mount.motion.mapping.outMin = 0;
    mount.motion.mapping.outMax = -360;
    m_sceneNodes.append(mount);

    SceneNodeConfig tube;
    tube.id = "tube";
    tube.parentId = "mount";
    tube.model = "Tube.qml";
    tube.motion.type = "rotation";
    tube.motion.axis = QVector3D(1, 0, 0);
    tube.motion.propertyLink = "Telescope.Altitude";
    tube.motion.mapping.type = "linear";
    tube.motion.mapping.inMin = 0;
    tube.motion.mapping.inMax = 90;
    tube.motion.mapping.outMin = 0;
    tube.motion.mapping.outMax = -90;
    m_sceneNodes.append(tube);
}

static MappingDefinition parseMapping(const YAML::Node& node) {
    MappingDefinition m;
    if (!node) return m;
    if (node["type"]) m.type = QString::fromStdString(node["type"].as<std::string>());
    if (node["in_min"]) m.inMin = node["in_min"].as<double>();
    if (node["in_max"]) m.inMax = node["in_max"].as<double>();
    if (node["out_min"]) m.outMin = node["out_min"].as<double>();
    if (node["out_max"]) m.outMax = node["out_max"].as<double>();
    if (node["true_pattern"]) m.truePattern = QString::fromStdString(node["true_pattern"].as<std::string>());
    return m;
}

static void emitMapping(YAML::Emitter& out, const MappingDefinition& m) {
    if (m.type.isEmpty() || m.type == "none") return;
    out << YAML::Key << "mapping" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << m.type.toStdString();
    if (m.type == "linear") {
        out << YAML::Key << "in_min" << YAML::Value << m.inMin;
        out << YAML::Key << "in_max" << YAML::Value << m.inMax;
        out << YAML::Key << "out_min" << YAML::Value << m.outMin;
        out << YAML::Key << "out_max" << YAML::Value << m.outMax;
    } else if (m.type == "binary") {
        if (!m.truePattern.isEmpty()) out << YAML::Key << "true_pattern" << YAML::Value << m.truePattern.toStdString();
    }
    out << YAML::EndMap;
}

QVariantList LayoutConfig::widgetsVariant() const
{
    QVariantList list;
    for (const auto& w : m_widgets) {
        QVariantMap map;
        map["type"] = w.type;
        map["id"] = w.id;
        map["label"] = w.label;
        map["x"] = w.x;
        map["y"] = w.y;
        map["property"] = w.propertyLink;
        
        QVariantMap mapping;
        mapping["type"] = w.mapping.type;
        mapping["in_min"] = w.mapping.inMin;
        mapping["in_max"] = w.mapping.inMax;
        mapping["out_min"] = w.mapping.outMin;
        mapping["out_max"] = w.mapping.outMax;
        mapping["true_pattern"] = w.mapping.truePattern;
        map["mapping"] = mapping;
        
        list << map;
    }
    return list;
}

QVariantList LayoutConfig::sceneNodesVariant() const
{
    QVariantList list;
    for (const auto& s : m_sceneNodes) {
        QVariantMap map;
        map["model"] = s.model;
        map["id"] = s.id;
        map["parent"] = s.parentId;
        map["offset"] = s.offset;
        
        QVariantMap motion;
        motion["type"] = s.motion.type;
        motion["axis"] = s.motion.axis;
        motion["property"] = s.motion.propertyLink;
        
        QVariantMap mapping;
        mapping["type"] = s.motion.mapping.type;
        mapping["in_min"] = s.motion.mapping.inMin;
        mapping["in_max"] = s.motion.mapping.inMax;
        mapping["out_min"] = s.motion.mapping.outMin;
        mapping["out_max"] = s.motion.mapping.outMax;
        mapping["true_pattern"] = s.motion.mapping.truePattern;
        motion["mapping"] = mapping;
        
        map["motion"] = motion;
        list << map;
    }
    return list;
}

void LayoutConfig::addWidget(const QVariantMap& config)
{
    WidgetConfig w;
    w.type = config["type"].toString();
    w.id = config["id"].toString();
    w.label = config["label"].toString();
    w.x = config["x"].toDouble();
    w.y = config["y"].toDouble();
    w.propertyLink = config["property"].toString();
    
    if (config.contains("mapping")) {
        QVariantMap m = config["mapping"].toMap();
        w.mapping.type = m["type"].toString();
        w.mapping.inMin = m["in_min"].toDouble();
        w.mapping.inMax = m["in_max"].toDouble();
        w.mapping.outMin = m["out_min"].toDouble();
        w.mapping.outMax = m["out_max"].toDouble();
        w.mapping.truePattern = m["true_pattern"].toString();
    }
    
    m_widgets.append(w);
    emit widgetsChanged();
}

void LayoutConfig::removeWidget(const QString& id)
{
    for (int i = 0; i < m_widgets.size(); ++i) {
        if (m_widgets[i].id == id) {
            m_widgets.removeAt(i);
            emit widgetsChanged();
            return;
        }
    }
}

void LayoutConfig::updateWidget(const QString& id, const QVariantMap& config)
{
    for (int i = 0; i < m_widgets.size(); ++i) {
        if (m_widgets[i].id == id) {
            if (config.contains("type")) m_widgets[i].type = config["type"].toString();
            if (config.contains("label")) m_widgets[i].label = config["label"].toString();
            if (config.contains("x")) m_widgets[i].x = config["x"].toDouble();
            if (config.contains("y")) m_widgets[i].y = config["y"].toDouble();
            if (config.contains("property")) m_widgets[i].propertyLink = config["property"].toString();
            
            if (config.contains("mapping")) {
                QVariantMap m = config["mapping"].toMap();
                if (m.contains("type")) m_widgets[i].mapping.type = m["type"].toString();
                if (m.contains("in_min")) m_widgets[i].mapping.inMin = m["in_min"].toDouble();
                if (m.contains("in_max")) m_widgets[i].mapping.inMax = m["in_max"].toDouble();
                if (m.contains("out_min")) m_widgets[i].mapping.outMin = m["out_min"].toDouble();
                if (m.contains("out_max")) m_widgets[i].mapping.outMax = m["out_max"].toDouble();
                if (m.contains("true_pattern")) m_widgets[i].mapping.truePattern = m["true_pattern"].toString();
            }
            emit widgetsChanged();
            return;
        }
    }
}

void LayoutConfig::addSceneNode(const QVariantMap& config)
{
    SceneNodeConfig s;
    s.id = config["id"].toString();
    s.model = config["model"].toString();
    s.parentId = config["parent"].toString();
    if (config.contains("offset")) s.offset = config["offset"].value<QVector3D>();
    
    if (config.contains("motion")) {
        QVariantMap m = config["motion"].toMap();
        s.motion.type = m["type"].toString();
        if (m.contains("axis")) s.motion.axis = m["axis"].value<QVector3D>();
        s.motion.propertyLink = m["property"].toString();
        
        if (m.contains("mapping")) {
            QVariantMap map = m["mapping"].toMap();
            s.motion.mapping.type = map["type"].toString();
            s.motion.mapping.inMin = map["in_min"].toDouble();
            s.motion.mapping.inMax = map["in_max"].toDouble();
            s.motion.mapping.outMin = map["out_min"].toDouble();
            s.motion.mapping.outMax = map["out_max"].toDouble();
        }
    }
    
    m_sceneNodes.append(s);
    emit sceneNodesChanged();
}

void LayoutConfig::removeSceneNode(const QString& id)
{
    for (int i = 0; i < m_sceneNodes.size(); ++i) {
        if (m_sceneNodes[i].id == id) {
            m_sceneNodes.removeAt(i);
            emit sceneNodesChanged();
            return;
        }
    }
}

void LayoutConfig::updateSceneNode(const QString& id, const QVariantMap& config)
{
    for (int i = 0; i < m_sceneNodes.size(); ++i) {
        if (m_sceneNodes[i].id == id) {
            if (config.contains("model")) m_sceneNodes[i].model = config["model"].toString();
            if (config.contains("parent")) m_sceneNodes[i].parentId = config["parent"].toString();
            if (config.contains("offset")) m_sceneNodes[i].offset = config["offset"].value<QVector3D>();
            
            if (config.contains("motion")) {
                QVariantMap m = config["motion"].toMap();
                if (m.contains("type")) m_sceneNodes[i].motion.type = m["type"].toString();
                if (m.contains("axis")) m_sceneNodes[i].motion.axis = m["axis"].value<QVector3D>();
                if (m.contains("property")) m_sceneNodes[i].motion.propertyLink = m["property"].toString();
                
                if (m.contains("mapping")) {
                    QVariantMap map = m["mapping"].toMap();
                    if (map.contains("type")) m_sceneNodes[i].motion.mapping.type = map["type"].toString();
                    if (map.contains("in_min")) m_sceneNodes[i].motion.mapping.inMin = map["in_min"].toDouble();
                    if (map.contains("in_max")) m_sceneNodes[i].motion.mapping.inMax = map["in_max"].toDouble();
                    if (map.contains("out_min")) m_sceneNodes[i].motion.mapping.outMin = map["out_min"].toDouble();
                    if (map.contains("out_max")) m_sceneNodes[i].motion.mapping.outMax = map["out_max"].toDouble();
                }
            }
            emit sceneNodesChanged();
            return;
        }
    }
}

void LayoutConfig::clear()
{
    m_widgets.clear();
    m_sceneNodes.clear();
    emit widgetsChanged();
    emit sceneNodesChanged();
}

bool LayoutConfig::loadFromFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Node config = YAML::LoadFile(filePath.toStdString());
        
        if (config["background_source"]) m_backgroundSource = QString::fromStdString(config["background_source"].as<std::string>());
        else m_backgroundSource = "";

        if (config["background_color"]) m_backgroundColor = QString::fromStdString(config["background_color"].as<std::string>());
        else m_backgroundColor = "transparent";

        emit backgroundSourceChanged();
        emit backgroundColorChanged();

        m_widgets.clear();
        if (config["widgets"] && config["widgets"].IsSequence() && config["widgets"].size() > 0) {
            for (const auto& wNode : config["widgets"]) {
                WidgetConfig w;
                if (wNode["type"]) w.type = QString::fromStdString(wNode["type"].as<std::string>());
                if (wNode["id"]) w.id = QString::fromStdString(wNode["id"].as<std::string>());
                if (wNode["label"]) w.label = QString::fromStdString(wNode["label"].as<std::string>());
                if (wNode["x"]) w.x = wNode["x"].as<double>();
                if (wNode["y"]) w.y = wNode["y"].as<double>();
                if (wNode["property"]) w.propertyLink = QString::fromStdString(wNode["property"].as<std::string>());
                w.mapping = parseMapping(wNode["mapping"]);
                m_widgets.append(w);
            }
        } else {
            setDefaultWidgets();
        }
        
        m_sceneNodes.clear();
        if (config["scene"] && config["scene"].IsSequence() && config["scene"].size() > 0) {
            for (const auto& sNode : config["scene"]) {
                SceneNodeConfig s;
                if (sNode["model"]) s.model = QString::fromStdString(sNode["model"].as<std::string>());
                if (sNode["id"]) s.id = QString::fromStdString(sNode["id"].as<std::string>());
                if (sNode["parent"]) s.parentId = QString::fromStdString(sNode["parent"].as<std::string>());
                if (sNode["offset"]) {
                    s.offset = QVector3D(sNode["offset"][0].as<float>(), 
                                        sNode["offset"][1].as<float>(), 
                                        sNode["offset"][2].as<float>());
                }
                
                if (sNode["motion"]) {
                    YAML::Node mNode = sNode["motion"];
                    if (mNode["type"]) s.motion.type = QString::fromStdString(mNode["type"].as<std::string>());
                    if (mNode["axis"]) {
                        s.motion.axis = QVector3D(mNode["axis"][0].as<float>(), 
                                                 mNode["axis"][1].as<float>(), 
                                                 mNode["axis"][2].as<float>());
                    }
                    if (mNode["property"]) s.motion.propertyLink = QString::fromStdString(mNode["property"].as<std::string>());
                    s.motion.mapping = parseMapping(mNode["mapping"]);
                }
                m_sceneNodes.append(s);
            }
        } else {
            // Re-inject defaults if scene is missing or empty
            setDefaultScene();
        }
        emit sceneNodesChanged();
        return true;
    } catch (const std::exception& e) {
        errorMessage = QString::fromStdString(e.what());
        return false;
    }
}

bool LayoutConfig::saveToFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        
        out << YAML::Key << "background_source" << YAML::Value << m_backgroundSource.toStdString();
        out << YAML::Key << "background_color" << YAML::Value << m_backgroundColor.toStdString();

        out << YAML::Key << "widgets" << YAML::Value << YAML::BeginSeq;
        for (const auto& w : m_widgets) {
            out << YAML::BeginMap;
            out << YAML::Key << "type" << YAML::Value << w.type.toStdString();
            out << YAML::Key << "id" << YAML::Value << w.id.toStdString();
            out << YAML::Key << "label" << YAML::Value << w.label.toStdString();
            out << YAML::Key << "x" << YAML::Value << w.x;
            out << YAML::Key << "y" << YAML::Value << w.y;
            out << YAML::Key << "property" << YAML::Value << w.propertyLink.toStdString();
            emitMapping(out, w.mapping);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        
        out << YAML::Key << "scene" << YAML::Value << YAML::BeginSeq;
        for (const auto& s : m_sceneNodes) {
            out << YAML::BeginMap;
            out << YAML::Key << "model" << YAML::Value << s.model.toStdString();
            out << YAML::Key << "id" << YAML::Value << s.id.toStdString();
            out << YAML::Key << "parent" << YAML::Value << s.parentId.toStdString();
            out << YAML::Key << "offset" << YAML::Flow << YAML::BeginSeq << s.offset.x() << s.offset.y() << s.offset.z() << YAML::EndSeq;
            
            if (s.motion.type != "none" && !s.motion.type.isEmpty()) {
                out << YAML::Key << "motion" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "type" << YAML::Value << s.motion.type.toStdString();
                out << YAML::Key << "axis" << YAML::Flow << YAML::BeginSeq << s.motion.axis.x() << s.motion.axis.y() << s.motion.axis.z() << YAML::EndSeq;
                out << YAML::Key << "property" << YAML::Value << s.motion.propertyLink.toStdString();
                emitMapping(out, s.motion.mapping);
                out << YAML::EndMap;
            }
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

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

bool LayoutConfig::validate(const CapabilityRegistry* caps, QString& errorMessage)
{
    m_isValid = true;
    m_validationError = "";

    auto fail = [&](const QString& msg) {
        m_isValid = false;
        m_validationError = msg;
        errorMessage = msg;
        emit validationChanged();
        return false;
    };

    QStringList validProps;
    if (caps) {
        validProps = caps->allPropertyLinks();
    }

    // Check for duplicate widget IDs and valid properties
    QSet<QString> widgetIds;
    for (const auto& w : m_widgets) {
        if (widgetIds.contains(w.id)) {
            return fail(QString("Duplicate widget ID: %1").arg(w.id));
        }
        widgetIds.insert(w.id);

        if (caps && !w.propertyLink.isEmpty() && !validProps.contains(w.propertyLink)) {
            return fail(QString("Widget '%1' references non-existent property: %2").arg(w.id, w.propertyLink));
        }
    }

    // Check for duplicate scene node IDs, valid properties, and file existence
    QSet<QString> nodeIds;
    for (const auto& s : m_sceneNodes) {
        if (nodeIds.contains(s.id)) {
            return fail(QString("Duplicate scene node ID: %1").arg(s.id));
        }
        nodeIds.insert(s.id);

        if (caps && s.motion.type != "none" && !s.motion.propertyLink.isEmpty() && !validProps.contains(s.motion.propertyLink)) {
            return fail(QString("Scene node '%1' references non-existent property: %2").arg(s.id, s.motion.propertyLink));
        }

        // Validate model file existence (if not a primitive)
        if (!s.model.isEmpty() && !s.model.startsWith("#")) {
            bool exists = false;
            if (s.model.startsWith("qrc:")) {
                exists = QFile::exists(s.model.mid(3)); // mid(3) skips "qrc" but keeps ":/"
            } else {
                // Try as relative to known resource paths or absolute
                exists = QFile::exists(s.model) || 
                         QFile::exists(":/qt/resources/qml/" + s.model) || 
                         QFile::exists(":/qt/qml/ObservatoryMonitor/" + s.model);
            }
            
            if (!exists) {
                return fail(QString("Scene node '%1' references missing model file: %2").arg(s.id, s.model));
            }
        }
    }

    // Check for circular dependencies in scene nodes
    for (const auto& s : m_sceneNodes) {
        QString current = s.parentId;
        QSet<QString> visited;
        visited.insert(s.id);
        
        while (!current.isEmpty()) {
            if (visited.contains(current)) {
                return fail(QString("Circular dependency detected starting at node: %1").arg(s.id));
            }
            visited.insert(current);
            
            // Find parent node
            bool found = false;
            for (const auto& p : m_sceneNodes) {
                if (p.id == current) {
                    current = p.parentId;
                    found = true;
                    break;
                }
            }
            if (!found) break; // Parent not found is allowed (orphaned or global root)
        }
    }

    emit validationChanged();
    return true;
}

} // namespace ObservatoryMonitor
