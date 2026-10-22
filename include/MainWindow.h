#pragma once

#include "ConnectionStateMachine.h"

#include <QMainWindow>
#include <memory>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class QListWidget;
class QCheckBox;
class QPlainTextEdit;
class MqttClientInterface;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSubscribeClicked();
    void onPublishClicked();
    void onStateChanged(ConnectionStateMachine::State state);
    void onMqttLog(const QString &line);
    void onMessageReceived(const QString &topic, const QByteArray &payload);

private:
    void setupUi();
    void appendLog(const QString &line);
    void updateChromeForState(ConnectionStateMachine::State state);
    void recreateClient();

    QLineEdit *m_hostEdit = nullptr;
    QSpinBox *m_portSpin = nullptr;
    QLineEdit *m_clientIdEdit = nullptr;
    QCheckBox *m_mockCheck = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_disconnectBtn = nullptr;
    QLabel *m_stateBadge = nullptr;
    QLabel *m_stateDetail = nullptr;

    QLineEdit *m_topicEdit = nullptr;
    QPushButton *m_subscribeBtn = nullptr;
    QLineEdit *m_publishTopicEdit = nullptr;
    QPlainTextEdit *m_payloadEdit = nullptr;
    QPushButton *m_publishBtn = nullptr;
    QListWidget *m_logList = nullptr;

    ConnectionStateMachine *m_sm = nullptr;
    std::unique_ptr<MqttClientInterface> m_client;
};
