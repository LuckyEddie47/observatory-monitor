#include "ControllerListModel.h"

namespace ObservatoryMonitor {

ControllerListModel::ControllerListModel(ControllerManager* manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
{
    if (m_manager) {
        connect(m_manager, &ControllerManager::controllerStatusChanged,
                this, &ControllerListModel::onControllerStatusChanged);
        
        m_controllerNames = m_manager->getControllerNames();
    }
}

int ControllerListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_controllerNames.count();
}

QVariant ControllerListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_controllerNames.count())
        return QVariant();

    const QString& name = m_controllerNames.at(index.row());
    
    switch (role) {
    case NameRole:
        return name;
    case StatusRole:
        return static_cast<int>(m_manager->getControllerStatus(name));
    case StatusStringRole:
        return statusToString(m_manager->getControllerStatus(name));
    case IsEnabledRole:
        // We don't have a direct getControllerEnabled in manager yet, 
        // but we can assume if it's in the list it's at least configured.
        return true; 
    case TypeRole:
        return m_manager->getControllerType(name);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ControllerListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[StatusRole] = "status";
    roles[StatusStringRole] = "statusString";
    roles[IsEnabledRole] = "isEnabled";
    roles[TypeRole] = "type";
    return roles;
}

void ControllerListModel::refresh()
{
    beginResetModel();
    if (m_manager) {
        m_controllerNames = m_manager->getControllerNames();
    } else {
        m_controllerNames.clear();
    }
    endResetModel();
}

void ControllerListModel::onControllerStatusChanged(const QString& name, ControllerStatus status)
{
    int row = m_controllerNames.indexOf(name);
    if (row != -1) {
        QModelIndex index = this->index(row);
        emit dataChanged(index, index, {StatusRole, StatusStringRole});
    }
}

QString ControllerListModel::statusToString(ControllerStatus status) const
{
    switch (status) {
    case ControllerStatus::Disconnected: return "Disconnected";
    case ControllerStatus::Connecting:   return "Connecting";
    case ControllerStatus::Connected:    return "Connected";
    case ControllerStatus::Error:        return "Error";
    default:                             return "Unknown";
    }
}

} // namespace ObservatoryMonitor
