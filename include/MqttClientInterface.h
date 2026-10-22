#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

/**
 * Thin MQTT client abstraction.
 *
 * Default build uses MockMqttClient (QTimer heartbeats) so the portfolio
 * demos offline. To plug a real broker client:
 *   1. Implement this interface (see QtMqttClient.h skeleton when
 *      DEVICE_CONSOLE_USE_QT_MQTT=ON and Qt6::Mqtt is installed).
 *   2. Or wrap QMqttClient / Paho / mosquitto behind the same signals/slots.
 */
class MqttClientInterface : public QObject
{
    Q_OBJECT
public:
    explicit MqttClientInterface(QObject *parent = nullptr) : QObject(parent) {}
    ~MqttClientInterface() override = default;

    virtual void connectToHost(const QString &host, quint16 port, const QString &clientId) = 0;
    virtual void disconnectFromHost() = 0;
    virtual void subscribe(const QString &topic) = 0;
    virtual void publish(const QString &topic, const QByteArray &payload) = 0;
    virtual bool isConnected() const = 0;

signals:
    void connected();
    void disconnected();
    void connectionFailed(const QString &reason);
    void messageReceived(const QString &topic, const QByteArray &payload);
    void logMessage(const QString &line);
};
