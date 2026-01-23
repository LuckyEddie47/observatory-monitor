#ifndef CONTROLLERPROXY_H
#define CONTROLLERPROXY_H

#include <QObject>
#include <QString>
#include <QVariant>
#include "ControllerManager.h"

namespace ObservatoryMonitor {

class ControllerProxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(double azimuth READ azimuth NOTIFY azimuthChanged)
    Q_PROPERTY(double altitude READ altitude NOTIFY altitudeChanged)
    Q_PROPERTY(double ra READ ra NOTIFY raChanged)
    Q_PROPERTY(double dec READ dec NOTIFY decChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit ControllerProxy(const QString& name, ControllerManager* manager, QObject* parent = nullptr);

    QString name() const { return m_name; }
    double azimuth() const { return m_azimuth; }
    double altitude() const { return m_altitude; }
    double ra() const { return m_ra; }
    double dec() const { return m_dec; }
    QString status() const;

signals:
    void azimuthChanged();
    void altitudeChanged();
    void raChanged();
    void decChanged();
    void statusChanged();

private slots:
    void onDataUpdated(const QString& controllerName, const QString& command, const QString& value);
    void onStatusChanged(const QString& name, ControllerStatus status);

private:
    double parseDegrees(const QString& value);

    QString m_name;
    ControllerManager* m_manager;
    
    double m_azimuth;
    double m_altitude;
    double m_ra;
    double m_dec;
};

} // namespace ObservatoryMonitor

#endif // CONTROLLERPROXY_H
