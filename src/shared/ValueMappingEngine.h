#ifndef VALUEMAPPINGENGINE_H
#define VALUEMAPPINGENGINE_H

#include <QString>
#include <QVariant>
#include <QObject>

namespace ObservatoryMonitor {

struct MappingDefinition {
    QString type; // "linear", "binary", "none"
    
    // Linear mapping
    double inMin = 0.0;
    double inMax = 1.0;
    double outMin = 0.0;
    double outMax = 1.0;
    
    // Binary mapping
    QVariant trueValue = true;
    QVariant falseValue = false;
    QString truePattern; // regex or string match
};

class ValueMappingEngine : public QObject
{
    Q_OBJECT
public:
    explicit ValueMappingEngine(QObject* parent = nullptr);

    Q_INVOKABLE QVariant mapValue(const QVariant& input, const QVariantMap& mapping);
    
    static QVariant mapValueInternal(const QVariant& input, const MappingDefinition& mapping);
    
private:
    static double mapLinear(double value, double inMin, double inMax, double outMin, double outMax);
};

} // namespace ObservatoryMonitor

#endif // VALUEMAPPINGENGINE_H
