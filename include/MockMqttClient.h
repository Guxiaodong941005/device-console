#pragma once

#include "MqttClientInterface.h"

#include <QTimer>
#include <QSet>

/**
 * Offline-capable MQTT stand-in: simulates connect latency, heartbeats,
 * and optional drop/reconnect so the UI state machine can be demoed
 * without a broker.
 */
class MockMqttClient final : public MqttClientInterface
{
    Q_OBJECT
public:
    explicit MockMqttClient(QObject *parent = nullptr);

    void connectToHost(const QString &host, quint16 port, const QString &clientId) override;
    void disconnectFromHost() override;
    void subscribe(const QString &topic) override;
    void publish(const QString &topic, const QByteArray &payload) override;
    bool isConnected() const override;

    /** When true, after online the client will drop and recover once (demo). */
    void setSimulateDrop(bool enabled);

private slots:
    void onConnectTimer();
    void onHeartbeat();
    void onDropTimer();

private:
    QTimer m_connectTimer;
    QTimer m_heartbeatTimer;
    QTimer m_dropTimer;
    QString m_host;
    quint16 m_port = 1883;
    QString m_clientId;
    QSet<QString> m_subscriptions;
    bool m_connected = false;
    bool m_simulateDrop = true;
    int m_heartbeatCount = 0;
};
