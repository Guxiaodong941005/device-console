#pragma once

#include <QObject>
#include <QString>

/**
 * Device connection state machine mirroring typical BLE/MQTT client flows:
 * Disconnected → Connecting → Online ↔ Reconnecting → Offline → …
 */
class ConnectionStateMachine : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Disconnected,
        Connecting,
        Online,
        Reconnecting,
        Offline
    };
    Q_ENUM(State)

    explicit ConnectionStateMachine(QObject *parent = nullptr);

    State state() const { return m_state; }
    static QString stateLabel(State s);

public slots:
    void requestConnect();
    void onBrokerConnected();
    void onBrokerDisconnected();
    void onBrokerFailed(const QString &reason);
    void requestDisconnect();

signals:
    void stateChanged(ConnectionStateMachine::State state);
    void statusTextChanged(const QString &text);

private:
    void setState(State next, const QString &detail = {});

    State m_state = State::Disconnected;
    bool m_userWantsOnline = false;
};
