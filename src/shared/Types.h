#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QDateTime>

namespace ObservatoryMonitor {

// Structure to hold cached values with metadata
struct CachedValue {
    QString value;
    QDateTime timestamp;
    bool valid;
    
    CachedValue() : valid(false) {}
    CachedValue(const QString& val) : value(val), timestamp(QDateTime::currentDateTime()), valid(true) {}
};

// Overall system status enumeration
enum class SystemStatus {
    AllConnected,       // All enabled controllers connected (GREEN)
    PartiallyConnected, // Some enabled controllers connected (YELLOW)
    Disconnected        // No enabled controllers connected (RED)
};

} // namespace ObservatoryMonitor

#endif // TYPES_H
