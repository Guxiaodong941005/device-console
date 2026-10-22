#pragma once

/**
 * Skeleton for a real QMqttClient wrapper.
 *
 * Build with:
 *   cmake -B build -DDEVICE_CONSOLE_USE_QT_MQTT=ON
 * after installing Qt MQTT (e.g. qt6-mqtt-dev / Qt online installer module).
 *
 * When DEVICE_CONSOLE_HAS_QT_MQTT is defined, implement connect/subscribe/publish
 * by forwarding to QMqttClient and emitting MqttClientInterface signals.
 *
 * This header is kept in-tree so recruiters can see the intended extension point
 * even when the system package is unavailable (Debian/Ubuntu often omit Qt MQTT).
 */

#if defined(DEVICE_CONSOLE_HAS_QT_MQTT)

#include "MqttClientInterface.h"

class QMqttClient;

class QtMqttClient final : public MqttClientInterface
{
    Q_OBJECT
public:
    explicit QtMqttClient(QObject *parent = nullptr);
    ~QtMqttClient() override;

    void connectToHost(const QString &host, quint16 port, const QString &clientId) override;
    void disconnectFromHost() override;
    void subscribe(const QString &topic) override;
    void publish(const QString &topic, const QByteArray &payload) override;
    bool isConnected() const override;

private:
    QMqttClient *m_client = nullptr;
};

#endif // DEVICE_CONSOLE_HAS_QT_MQTT
