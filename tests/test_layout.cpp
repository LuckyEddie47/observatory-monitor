#include <QtTest>
#include "LayoutConfig.h"
#include "CapabilityRegistry.h"

using namespace ObservatoryMonitor;

class TestLayout : public QObject
{
    Q_OBJECT

private slots:
    void testCircularDependency();
    void testDuplicateNodeIds();
    void testValidLayout();
};

void TestLayout::testCircularDependency()
{
    LayoutConfig layout;
    layout.clear();
    CapabilityRegistry caps;
    
    QVariantMap node1;
    node1["id"] = "node1";
    node1["model"] = "#Cube";
    node1["parent"] = "node2";
    
    QVariantMap node2;
    node2["id"] = "node2";
    node2["model"] = "#Cube";
    node2["parent"] = "node1";
    
    layout.addSceneNode(node1);
    layout.addSceneNode(node2);
    
    QString errorMessage;
    bool result = layout.validate(&caps, errorMessage);
    if (!result) qDebug() << "Validation error:" << errorMessage;
    QVERIFY(!result);
    QVERIFY(errorMessage.contains("Circular dependency"));
}

void TestLayout::testDuplicateNodeIds()
{
    LayoutConfig layout;
    layout.clear();
    CapabilityRegistry caps;
    
    QVariantMap node1;
    node1["id"] = "node1";
    node1["model"] = "#Cube";
    
    QVariantMap node2;
    node2["id"] = "node1"; // Duplicate
    node2["model"] = "#Cube";
    
    layout.addSceneNode(node1);
    layout.addSceneNode(node2);
    
    QString errorMessage;
    bool result = layout.validate(&caps, errorMessage);
    if (!result) qDebug() << "Validation error:" << errorMessage;
    QVERIFY(!result);
    QVERIFY(errorMessage.contains("Duplicate scene node ID"));
}

void TestLayout::testValidLayout()
{
    LayoutConfig layout;
    layout.clear();
    CapabilityRegistry caps;
    
    // Use a clean layout with one guaranteed valid node
    QVariantMap node;
    node["id"] = "valid_node";
    node["model"] = "#Cube";
    layout.addSceneNode(node);
    
    QString errorMessage;
    bool result = layout.validate(&caps, errorMessage);
    if (!result) qDebug() << "Validation error:" << errorMessage;
    QVERIFY(result);
}

QTEST_MAIN(TestLayout)
#include "test_layout.moc"
