#include <QtTest>
#include <QTemporaryFile>
#include "Config.h"

using namespace ObservatoryMonitor;

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDefaultConfig();
    void testLoadValidConfig();
    void testLoadMissingFile();
    void testLoadMalformedYaml();
    void testValidationMissingBrokerHost();
    void testValidationInvalidPort();
    void testValidationInvalidTimeout();
    void testValidationInvalidReconnectInterval();
    void testValidationEmptyControllers();
    void testValidationMissingControllerFields();
    void testValidationDuplicatePrefix();
    void testSaveAndLoad();
};

void TestConfig::initTestCase()
{
    qDebug() << "Running Config tests...";
}

void TestConfig::testDefaultConfig()
{
    Config config;
    config.setDefaults();
    
    QCOMPARE(config.broker().host, QString("localhost"));
    QCOMPARE(config.broker().port, 1883);
    QCOMPARE(config.mqttTimeout(), 2.0);
    QCOMPARE(config.reconnectInterval(), 10);
    QCOMPARE(config.controllers().size(), 2);
    QCOMPARE(config.equipmentTypes().size(), 4);
    
    QString errorMessage;
    QVERIFY(config.validate(errorMessage));
}

void TestConfig::testLoadValidConfig()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    
    tempFile.write(R"(
mqtt:
  broker:
    host: "test.example.com"
    port: 8883
    username: "testuser"
    password: "testpass"
  timeout: 5.0
  reconnect_interval: 20

controllers:
  - name: "Test Observatory"
    type: "Observatory"
    prefix: "TEST"
    enabled: true

equipment_types:
  - name: "Observatory"
    controllers: ["OCS"]
)");
    tempFile.close();
    
    Config config;
    QString errorMessage;
    QVERIFY(config.loadFromFile(tempFile.fileName(), errorMessage));
    
    QCOMPARE(config.broker().host, QString("test.example.com"));
    QCOMPARE(config.broker().port, 8883);
    QCOMPARE(config.broker().username, QString("testuser"));
    QCOMPARE(config.broker().password, QString("testpass"));
    QCOMPARE(config.mqttTimeout(), 5.0);
    QCOMPARE(config.reconnectInterval(), 20);
    QCOMPARE(config.controllers().size(), 1);
    QCOMPARE(config.controllers()[0].name, QString("Test Observatory"));
}

void TestConfig::testLoadMissingFile()
{
    Config config;
    QString errorMessage;
    
    QVERIFY(!config.loadFromFile("/nonexistent/path/config.yaml", errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(errorMessage.contains("cannot be read") || errorMessage.contains("Error opening"));
}

void TestConfig::testLoadMalformedYaml()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    
    // Invalid YAML - unclosed bracket
    tempFile.write(R"(
mqtt:
  broker:
    host: "localhost"
    port: [1883
)");
    tempFile.close();
    
    Config config;
    QString errorMessage;
    
    QVERIFY(!config.loadFromFile(tempFile.fileName(), errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(errorMessage.contains("line") || errorMessage.contains("Parser"));
}

void TestConfig::testValidationMissingBrokerHost()
{
    Config config;
    config.setDefaults();
    
    BrokerConfig broker = config.broker();
    broker.host = "";
    config.setBroker(broker);
    
    QString errorMessage;
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("host is empty"));
}

void TestConfig::testValidationInvalidPort()
{
    Config config;
    config.setDefaults();
    
    BrokerConfig broker = config.broker();
    broker.port = 99999;
    config.setBroker(broker);
    
    QString errorMessage;
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("port is invalid"));
    QVERIFY(errorMessage.contains("1-65535"));
}

void TestConfig::testValidationInvalidTimeout()
{
    Config config;
    config.setDefaults();
    config.setMqttTimeout(100.0);
    
    QString errorMessage;
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("timeout is out of range"));
    QVERIFY(errorMessage.contains("0.5-30.0"));
}

void TestConfig::testValidationInvalidReconnectInterval()
{
    Config config;
    config.setDefaults();
    config.setReconnectInterval(500);
    
    QString errorMessage;
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("reconnect interval is out of range"));
    QVERIFY(errorMessage.contains("1-300"));
}

void TestConfig::testValidationEmptyControllers()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    
    tempFile.write(R"(
mqtt:
  broker:
    host: "localhost"
    port: 1883
  timeout: 2.0
  reconnect_interval: 10

controllers: []

equipment_types:
  - name: "Observatory"
    controllers: ["OCS"]
)");
    tempFile.close();
    
    Config config;
    QString errorMessage;
    QVERIFY(config.loadFromFile(tempFile.fileName(), errorMessage));
    
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("No controllers defined"));
}

void TestConfig::testValidationMissingControllerFields()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    
    tempFile.write(R"(
mqtt:
  broker:
    host: "localhost"
    port: 1883
  timeout: 2.0
  reconnect_interval: 10

controllers:
  - name: ""
    type: "Observatory"
    prefix: "OCS"
    enabled: true

equipment_types:
  - name: "Observatory"
    controllers: ["OCS"]
)");
    tempFile.close();
    
    Config config;
    QString errorMessage;
    QVERIFY(config.loadFromFile(tempFile.fileName(), errorMessage));
    
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("name is empty"));
}

void TestConfig::testValidationDuplicatePrefix()
{
    Config config;
    config.setDefaults();
    
    ControllerConfig ctrl1;
    ctrl1.name = "Controller 1";
    ctrl1.type = "Observatory";
    ctrl1.prefix = "SAME";
    ctrl1.enabled = true;
    
    ControllerConfig ctrl2;
    ctrl2.name = "Controller 2";
    ctrl2.type = "Telescope";
    ctrl2.prefix = "SAME";
    ctrl2.enabled = true;
    
    config.addController(ctrl1);
    config.addController(ctrl2);
    
    QString errorMessage;
    QVERIFY(!config.validate(errorMessage));
    QVERIFY(errorMessage.contains("duplicate MQTT prefix"));
}

void TestConfig::testSaveAndLoad()
{
    Config config1;
    config1.setDefaults();
    
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.close();
    
    QString errorMessage;
    QVERIFY(config1.saveToFile(filePath, errorMessage));
    
    Config config2;
    QVERIFY(config2.loadFromFile(filePath, errorMessage));
    
    QCOMPARE(config2.broker().host, config1.broker().host);
    QCOMPARE(config2.broker().port, config1.broker().port);
    QCOMPARE(config2.mqttTimeout(), config1.mqttTimeout());
    QCOMPARE(config2.reconnectInterval(), config1.reconnectInterval());
}

QTEST_MAIN(TestConfig)
#include "test_config.moc"
