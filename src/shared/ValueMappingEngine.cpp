#include "ValueMappingEngine.h"
#include <QRegularExpression>
#include <algorithm>

namespace ObservatoryMonitor {

ValueMappingEngine::ValueMappingEngine(QObject* parent)
    : QObject(parent)
{
}

QVariant ValueMappingEngine::mapValue(const QVariant& input, const QVariantMap& mapping)
{
    MappingDefinition def;
    def.type = mapping.value("type").toString();
    def.inMin = mapping.value("in_min", 0.0).toDouble();
    def.inMax = mapping.value("in_max", 1.0).toDouble();
    def.outMin = mapping.value("out_min", 0.0).toDouble();
    def.outMax = mapping.value("out_max", 1.0).toDouble();
    def.truePattern = mapping.value("true_pattern").toString();
    
    return mapValueInternal(input, def);
}

QVariant ValueMappingEngine::mapValueInternal(const QVariant& input, const MappingDefinition& mapping)
{
    if (mapping.type == "none" || mapping.type.isEmpty()) return input;
    
    if (mapping.type == "linear") {
        bool ok;
        double val = input.toDouble(&ok);
        if (!ok) return input;
        return mapLinear(val, mapping.inMin, mapping.inMax, mapping.outMin, mapping.outMax);
    }
    
    if (mapping.type == "binary") {
        QString str = input.toString();
        bool isTrue = false;
        
        if (!mapping.truePattern.isEmpty()) {
            QRegularExpression re(mapping.truePattern, QRegularExpression::CaseInsensitiveOption);
            isTrue = re.match(str).hasMatch();
        } else {
            // Default binary detection: 1, true, open, yes
            static const QStringList truthy = {"1", "true", "open", "yes", "on", "connected"};
            isTrue = truthy.contains(str.toLower());
        }
        
        return isTrue ? mapping.trueValue : mapping.falseValue;
    }
    
    return input;
}

double ValueMappingEngine::mapLinear(double value, double inMin, double inMax, double outMin, double outMax)
{
    if (std::abs(inMax - inMin) < 1e-9) return outMin;
    double clamped = std::max(std::min(inMin, inMax), std::min(std::max(inMin, inMax), value));
    
    // Support inverse mapping (e.g. 0-360 -> 0- -360)
    double t = (value - inMin) / (inMax - inMin);
    // We don't necessarily want to clamp 't' if we want to support values outside range, 
    // but for now let's keep it simple.
    return t * (outMax - outMin) + outMin;
}

} // namespace ObservatoryMonitor
