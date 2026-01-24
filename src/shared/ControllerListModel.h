#ifndef CONTROLLERLISTMODEL_H
#define CONTROLLERLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QHash>
#include "ControllerManager.h"

namespace ObservatoryMonitor {

class ControllerListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ControllerRoles {
        NameRole = Qt::UserRole + 1,
        StatusRole,
        StatusStringRole,
        IsEnabledRole,
        TypeRole
    };

    explicit ControllerListModel(ControllerManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

private slots:
    void onControllerStatusChanged(const QString& name, ControllerStatus status);

private:
    ControllerManager* m_manager;
    QStringList m_controllerNames;
    
    QString statusToString(ControllerStatus status) const;
};

} // namespace ObservatoryMonitor

#endif // CONTROLLERLISTMODEL_H
