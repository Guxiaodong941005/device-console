#include "MainWindow.h"
#include "MockMqttClient.h"
#include "MqttClientInterface.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_sm = new ConnectionStateMachine(this);
    connect(m_sm, &ConnectionStateMachine::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_sm, &ConnectionStateMachine::statusTextChanged, this, [this](const QString &t) {
        m_stateDetail->setText(t);
    });

    setupUi();
    recreateClient();

    // Auto-retry when Offline (mirrors device reconnect policy). Connected once.
    connect(m_sm, &ConnectionStateMachine::stateChanged, this, [this](ConnectionStateMachine::State s) {
        if (s == ConnectionStateMachine::State::Offline && m_client) {
            QTimer::singleShot(1500, this, [this]() {
                if (m_sm->state() != ConnectionStateMachine::State::Offline) {
                    return;
                }
                m_sm->requestConnect();
                m_client->connectToHost(m_hostEdit->text().trimmed(),
                                        static_cast<quint16>(m_portSpin->value()),
                                        m_clientIdEdit->text().trimmed());
            });
        }
    });

    updateChromeForState(m_sm->state());
    setWindowTitle(QStringLiteral("Device Console · 设备调试台"));
    resize(780, 560);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);

    // --- Broker ---
    auto *brokerBox = new QGroupBox(QStringLiteral("Broker / 连接"), central);
    auto *brokerLay = new QHBoxLayout(brokerBox);
    m_hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), brokerBox);
    m_portSpin = new QSpinBox(brokerBox);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(1883);
    m_clientIdEdit = new QLineEdit(QStringLiteral("console-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8)),
                                   brokerBox);
    m_mockCheck = new QCheckBox(QStringLiteral("Mock mode（无 Broker 也可演示）"), brokerBox);
    m_mockCheck->setChecked(true);
    m_connectBtn = new QPushButton(QStringLiteral("Connect"), brokerBox);
    m_disconnectBtn = new QPushButton(QStringLiteral("Disconnect"), brokerBox);

    brokerLay->addWidget(new QLabel(QStringLiteral("Host"), brokerBox));
    brokerLay->addWidget(m_hostEdit, 1);
    brokerLay->addWidget(new QLabel(QStringLiteral("Port"), brokerBox));
    brokerLay->addWidget(m_portSpin);
    brokerLay->addWidget(new QLabel(QStringLiteral("ClientId"), brokerBox));
    brokerLay->addWidget(m_clientIdEdit, 1);
    brokerLay->addWidget(m_mockCheck);
    brokerLay->addWidget(m_connectBtn);
    brokerLay->addWidget(m_disconnectBtn);
    root->addWidget(brokerBox);

    // --- State machine badge ---
    auto *stateRow = new QHBoxLayout();
    m_stateBadge = new QLabel(QStringLiteral("Disconnected"), central);
    m_stateBadge->setAlignment(Qt::AlignCenter);
    m_stateBadge->setMinimumWidth(140);
    m_stateBadge->setStyleSheet(QStringLiteral(
        "QLabel { background:#9e9e9e; color:white; font-weight:600; padding:8px 12px; border-radius:6px; }"));
    m_stateDetail = new QLabel(QStringLiteral("idle"), central);
    stateRow->addWidget(new QLabel(QStringLiteral("State machine:"), central));
    stateRow->addWidget(m_stateBadge);
    stateRow->addWidget(m_stateDetail, 1);
    root->addLayout(stateRow);

    // --- Subscribe / publish ---
    auto *msgBox = new QGroupBox(QStringLiteral("Topics / 消息"), central);
    auto *msgLay = new QVBoxLayout(msgBox);

    auto *subRow = new QHBoxLayout();
    m_topicEdit = new QLineEdit(QStringLiteral("device/+/status"), msgBox);
    m_subscribeBtn = new QPushButton(QStringLiteral("Subscribe"), msgBox);
    subRow->addWidget(new QLabel(QStringLiteral("Subscribe"), msgBox));
    subRow->addWidget(m_topicEdit, 1);
    subRow->addWidget(m_subscribeBtn);
    msgLay->addLayout(subRow);

    auto *pubRow = new QHBoxLayout();
    m_publishTopicEdit = new QLineEdit(QStringLiteral("device/cmd/run"), msgBox);
    m_payloadEdit = new QPlainTextEdit(QStringLiteral("{\"cmd\":\"ping\",\"ts\":0}"), msgBox);
    m_payloadEdit->setMaximumHeight(72);
    m_publishBtn = new QPushButton(QStringLiteral("Publish JSON"), msgBox);
    auto *pubCol = new QVBoxLayout();
    auto *pubTopicRow = new QHBoxLayout();
    pubTopicRow->addWidget(new QLabel(QStringLiteral("Publish topic"), msgBox));
    pubTopicRow->addWidget(m_publishTopicEdit, 1);
    pubCol->addLayout(pubTopicRow);
    pubCol->addWidget(m_payloadEdit);
    pubRow->addLayout(pubCol, 1);
    pubRow->addWidget(m_publishBtn);
    msgLay->addLayout(pubRow);
    root->addWidget(msgBox);

    // --- Log ---
    auto *logBox = new QGroupBox(QStringLiteral("Message log / 日志"), central);
    auto *logLay = new QVBoxLayout(logBox);
    m_logList = new QListWidget(logBox);
    m_logList->setUniformItemSizes(true);
    logLay->addWidget(m_logList);
    root->addWidget(logBox, 1);

    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_subscribeBtn, &QPushButton::clicked, this, &MainWindow::onSubscribeClicked);
    connect(m_publishBtn, &QPushButton::clicked, this, &MainWindow::onPublishClicked);
    connect(m_mockCheck, &QCheckBox::toggled, this, [this](bool) {
        if (m_sm->state() == ConnectionStateMachine::State::Disconnected) {
            recreateClient();
            appendLog(QStringLiteral("client backend switched (mock=%1)")
                          .arg(m_mockCheck->isChecked() ? QStringLiteral("on") : QStringLiteral("off")));
        }
    });
}

void MainWindow::recreateClient()
{
    m_client.reset();
    // Default portfolio build: always Mock. Real QMqttClient can replace this
    // when DEVICE_CONSOLE_HAS_QT_MQTT and checkbox is unchecked.
    auto *mock = new MockMqttClient();
    mock->setSimulateDrop(true);
    m_client.reset(mock);

    connect(m_client.get(), &MqttClientInterface::connected, m_sm, &ConnectionStateMachine::onBrokerConnected);
    connect(m_client.get(), &MqttClientInterface::disconnected, m_sm, &ConnectionStateMachine::onBrokerDisconnected);
    connect(m_client.get(), &MqttClientInterface::connectionFailed, m_sm, &ConnectionStateMachine::onBrokerFailed);
    connect(m_client.get(), &MqttClientInterface::logMessage, this, &MainWindow::onMqttLog);
    connect(m_client.get(), &MqttClientInterface::messageReceived, this, &MainWindow::onMessageReceived);

}

void MainWindow::onConnectClicked()
{
    if (!m_mockCheck->isChecked()) {
        appendLog(QStringLiteral(
            "Real MQTT not linked in this build — keeping Mock. "
            "See README: cmake -DDEVICE_CONSOLE_USE_QT_MQTT=ON"));
        m_mockCheck->setChecked(true);
    }
    recreateClient();
    m_sm->requestConnect();
    m_client->connectToHost(m_hostEdit->text().trimmed(),
                            static_cast<quint16>(m_portSpin->value()),
                            m_clientIdEdit->text().trimmed());
}

void MainWindow::onDisconnectClicked()
{
    m_sm->requestDisconnect();
    if (m_client) {
        m_client->disconnectFromHost();
    }
}

void MainWindow::onSubscribeClicked()
{
    if (!m_client) {
        return;
    }
    m_client->subscribe(m_topicEdit->text().trimmed());
}

void MainWindow::onPublishClicked()
{
    if (!m_client) {
        return;
    }
    const QByteArray payload = m_payloadEdit->toPlainText().toUtf8();
    m_client->publish(m_publishTopicEdit->text().trimmed(), payload);
}

void MainWindow::onStateChanged(ConnectionStateMachine::State state)
{
    updateChromeForState(state);
}

void MainWindow::onMqttLog(const QString &line)
{
    appendLog(line);
}

void MainWindow::onMessageReceived(const QString &topic, const QByteArray &payload)
{
    appendLog(QStringLiteral("← %1 | %2").arg(topic, QString::fromUtf8(payload)));
}

void MainWindow::appendLog(const QString &line)
{
    const QString stamped =
        QStringLiteral("%1  %2").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), line);
    m_logList->addItem(stamped);
    m_logList->scrollToBottom();
    while (m_logList->count() > 500) {
        delete m_logList->takeItem(0);
    }
}

void MainWindow::updateChromeForState(ConnectionStateMachine::State state)
{
    const bool idle = state == ConnectionStateMachine::State::Disconnected;
    const bool busy = state == ConnectionStateMachine::State::Connecting
                      || state == ConnectionStateMachine::State::Reconnecting;
    const bool online = state == ConnectionStateMachine::State::Online;

    m_connectBtn->setEnabled(idle || state == ConnectionStateMachine::State::Offline);
    m_disconnectBtn->setEnabled(!idle);
    m_hostEdit->setEnabled(idle);
    m_portSpin->setEnabled(idle);
    m_clientIdEdit->setEnabled(idle);
    m_mockCheck->setEnabled(idle);
    m_subscribeBtn->setEnabled(online);
    m_publishBtn->setEnabled(online || busy);

    QString bg = QStringLiteral("#9e9e9e");
    switch (state) {
    case ConnectionStateMachine::State::Disconnected:
        bg = QStringLiteral("#9e9e9e");
        break;
    case ConnectionStateMachine::State::Connecting:
    case ConnectionStateMachine::State::Reconnecting:
        bg = QStringLiteral("#fb8c00");
        break;
    case ConnectionStateMachine::State::Online:
        bg = QStringLiteral("#43a047");
        break;
    case ConnectionStateMachine::State::Offline:
        bg = QStringLiteral("#e53935");
        break;
    }
    m_stateBadge->setText(ConnectionStateMachine::stateLabel(state));
    m_stateBadge->setStyleSheet(
        QStringLiteral("QLabel { background:%1; color:white; font-weight:600; padding:8px 12px; border-radius:6px; }")
            .arg(bg));
}
