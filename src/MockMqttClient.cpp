#include "MockMqttClient.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

MockMqttClient::MockMqttClient(QObject *parent)
    : MqttClientInterface(parent)
{
    m_connectTimer.setSingleShot(true);
    connect(&m_connectTimer, &QTimer::timeout, this, &MockMqttClient::onConnectTimer);

    m_heartbeatTimer.setInterval(2000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &MockMqttClient::onHeartbeat);

    m_dropTimer.setSingleShot(true);
    connect(&m_dropTimer, &QTimer::timeout, this, &MockMqttClient::onDropTimer);
}

void MockMqttClient::setSimulateDrop(bool enabled)
{
    m_simulateDrop = enabled;
}

void MockMqttClient::connectToHost(const QString &host, quint16 port, const QString &clientId)
{
    if (m_connected) {
        return;
    }
    m_host = host;
    m_port = port;
    m_clientId = clientId.isEmpty() ? QStringLiteral("device-console") : clientId;
    m_heartbeatCount = 0;

    emit logMessage(QStringLiteral("[mock] connecting to %1:%2 as %3…")
                        .arg(m_host)
                        .arg(m_port)
                        .arg(m_clientId));
    m_connectTimer.start(800);
}

void MockMqttClient::disconnectFromHost()
{
    m_connectTimer.stop();
    m_heartbeatTimer.stop();
    m_dropTimer.stop();
    if (!m_connected) {
        emit logMessage(QStringLiteral("[mock] disconnect while not connected"));
        emit disconnected();
        return;
    }
    m_connected = false;
    emit logMessage(QStringLiteral("[mock] disconnected by user"));
    emit disconnected();
}

void MockMqttClient::subscribe(const QString &topic)
{
    if (topic.isEmpty()) {
        return;
    }
    m_subscriptions.insert(topic);
    emit logMessage(QStringLiteral("[mock] subscribed: %1").arg(topic));
}

void MockMqttClient::publish(const QString &topic, const QByteArray &payload)
{
    emit logMessage(QStringLiteral("[mock] publish %1 → %2")
                        .arg(topic, QString::fromUtf8(payload)));
    if (!m_connected) {
        emit logMessage(QStringLiteral("[mock] publish ignored (offline)"));
        return;
    }
    // Echo as if the device acknowledged on a status topic.
    if (m_subscriptions.contains(topic) || m_subscriptions.contains(QStringLiteral("device/+/status"))) {
        QJsonObject ack{
            {QStringLiteral("echo"), true},
            {QStringLiteral("topic"), topic},
            {QStringLiteral("ts"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        };
        const QByteArray body = QJsonDocument(ack).toJson(QJsonDocument::Compact);
        emit messageReceived(QStringLiteral("device/mock/ack"), body);
    }
}

bool MockMqttClient::isConnected() const
{
    return m_connected;
}

void MockMqttClient::onConnectTimer()
{
    m_connected = true;
    emit logMessage(QStringLiteral("[mock] connected"));
    emit connected();
    m_heartbeatTimer.start();
    if (m_simulateDrop) {
        m_dropTimer.start(12000);
    }
}

void MockMqttClient::onHeartbeat()
{
    if (!m_connected) {
        return;
    }
    ++m_heartbeatCount;
    QJsonObject beat{
        {QStringLiteral("type"), QStringLiteral("heartbeat")},
        {QStringLiteral("seq"), m_heartbeatCount},
        {QStringLiteral("clientId"), m_clientId},
        {QStringLiteral("ts"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
    const QByteArray body = QJsonDocument(beat).toJson(QJsonDocument::Compact);
    const QString topic = QStringLiteral("device/%1/status").arg(m_clientId);
    emit messageReceived(topic, body);
}

void MockMqttClient::onDropTimer()
{
    if (!m_connected) {
        return;
    }
    m_connected = false;
    m_heartbeatTimer.stop();
    emit logMessage(QStringLiteral("[mock] simulated link drop — expect reconnect UI"));
    emit disconnected();
}
