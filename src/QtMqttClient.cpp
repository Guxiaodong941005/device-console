#include "QtMqttClient.h"

#if defined(DEVICE_CONSOLE_HAS_QT_MQTT)

#include <QtMqtt/qmqttclient.h>
#include <QtMqtt/qmqttsubscription.h>

QtMqttClient::QtMqttClient(QObject *parent)
    : MqttClientInterface(parent)
    , m_client(new QMqttClient(this))
{
    connect(m_client, &QMqttClient::connected, this, [this]() {
        emit logMessage(QStringLiteral("[qt-mqtt] connected"));
        emit connected();
    });
    connect(m_client, &QMqttClient::disconnected, this, [this]() {
        emit logMessage(QStringLiteral("[qt-mqtt] disconnected"));
        emit disconnected();
    });
    connect(m_client, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError e) {
        if (e != QMqttClient::NoError) {
            emit connectionFailed(QStringLiteral("QMqttClient error %1").arg(static_cast<int>(e)));
        }
    });
}

QtMqttClient::~QtMqttClient() = default;

void QtMqttClient::connectToHost(const QString &host, quint16 port, const QString &clientId)
{
    m_client->setHostname(host);
    m_client->setPort(port);
    m_client->setClientId(clientId);
    emit logMessage(QStringLiteral("[qt-mqtt] connecting %1:%2").arg(host).arg(port));
    m_client->connectToHost();
}

void QtMqttClient::disconnectFromHost()
{
    m_client->disconnectFromHost();
}

void QtMqttClient::subscribe(const QString &topic)
{
    auto sub = m_client->subscribe(topic);
    if (!sub) {
        emit logMessage(QStringLiteral("[qt-mqtt] subscribe failed: %1").arg(topic));
        return;
    }
    connect(sub, &QMqttSubscription::messageReceived, this, [this](const QMqttMessage &msg) {
        emit messageReceived(msg.topic().name(), msg.payload());
    });
    emit logMessage(QStringLiteral("[qt-mqtt] subscribed: %1").arg(topic));
}

void QtMqttClient::publish(const QString &topic, const QByteArray &payload)
{
    m_client->publish(topic, payload);
    emit logMessage(QStringLiteral("[qt-mqtt] publish %1").arg(topic));
}

bool QtMqttClient::isConnected() const
{
    return m_client->state() == QMqttClient::Connected;
}

#endif // DEVICE_CONSOLE_HAS_QT_MQTT
