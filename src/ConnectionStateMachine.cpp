#include "ConnectionStateMachine.h"

ConnectionStateMachine::ConnectionStateMachine(QObject *parent)
    : QObject(parent)
{
}

QString ConnectionStateMachine::stateLabel(State s)
{
    switch (s) {
    case State::Disconnected:
        return QStringLiteral("Disconnected");
    case State::Connecting:
        return QStringLiteral("Connecting");
    case State::Online:
        return QStringLiteral("Online");
    case State::Reconnecting:
        return QStringLiteral("Reconnecting");
    case State::Offline:
        return QStringLiteral("Offline");
    }
    return QStringLiteral("Unknown");
}

void ConnectionStateMachine::requestConnect()
{
    m_userWantsOnline = true;
    if (m_state == State::Online || m_state == State::Connecting || m_state == State::Reconnecting) {
        return;
    }
    if (m_state == State::Offline) {
        setState(State::Reconnecting, QStringLiteral("retrying after offline"));
    } else {
        setState(State::Connecting, QStringLiteral("user requested connect"));
    }
}

void ConnectionStateMachine::onBrokerConnected()
{
    if (!m_userWantsOnline) {
        return;
    }
    setState(State::Online, QStringLiteral("broker session up"));
}

void ConnectionStateMachine::onBrokerDisconnected()
{
    if (!m_userWantsOnline) {
        setState(State::Disconnected, QStringLiteral("idle"));
        return;
    }
    // Unexpected drop while user still wants online → Offline then UI can reconnect.
    setState(State::Offline, QStringLiteral("link lost"));
}

void ConnectionStateMachine::onBrokerFailed(const QString &reason)
{
    if (!m_userWantsOnline) {
        setState(State::Disconnected, reason);
        return;
    }
    setState(State::Offline, reason);
}

void ConnectionStateMachine::requestDisconnect()
{
    m_userWantsOnline = false;
    setState(State::Disconnected, QStringLiteral("user requested disconnect"));
}

void ConnectionStateMachine::setState(State next, const QString &detail)
{
    if (m_state == next && detail.isEmpty()) {
        return;
    }
    m_state = next;
    emit stateChanged(m_state);
    const QString text = detail.isEmpty()
                             ? stateLabel(m_state)
                             : QStringLiteral("%1 — %2").arg(stateLabel(m_state), detail);
    emit statusTextChanged(text);
}
