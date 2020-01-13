#ifndef MYTEAMVIEWER_H
#define MYTEAMVIEWER_H

#include "constants.h"

#include <QMainWindow>
#include <QUdpSocket>
#include <QTimer>

namespace Ui {
class MyTeamViewer;
}

class MyTeamViewer : public QMainWindow
{
    Q_OBJECT
public:
    explicit MyTeamViewer(QWidget *parent = nullptr);
    ~MyTeamViewer();

private:
    Ui::MyTeamViewer *ui;

    QTimer *tmrBroadcast;
    QTimer *tmrUpdateDevices;
    QTimer *tmrUpdateScreen;
    QTimer *tmrScreensSent;
    QTimer *tmrScreensReceived;
    QVector<QTimer*> tmrsScreenshot;

    QUdpSocket *waitingSocket;
    QUdpSocket *connectedSockets[CONNECTED_SOCKETS_COUNT];

    QSet<QString> avaliableDevices;
    QMap<QString, LastSeenDevice> avaliableDeviceInfo;

    QHostAddress waitingHost;
    QVector<QHostAddress> connectedHosts;
    quint16 waitingPort;
    QVector<quint16> connectedPorts;
    QString waitingName;

    quint64 screenBlockNum;
    QImage screenImage;
    quint64 screensSent;
    quint64 screensReceived;

    MyTeamViewerState myTeamViewerState;

private slots:
    QHostAddress GetCurrentlyConnectedIP();
    void WaitNS(quint64 microseconds);

    QVector<QByteArray> GetBroadcastIntroduceMessages();
    QVector<QByteArray> GetConnectMessages();
    QVector<QByteArray> GetScreenshotMessages();
    void SendMessages(QUdpSocket *sock, const QVector<QByteArray> &messages, const QHostAddress host, quint16 port);

    void ProcessIntroduceMessage(const QHostAddress senderHost, const quint16 senderPort, QStringList &strMsgs);
    void ProcessConnectMessage(const QHostAddress senderHost, const quint16 senderPort, QStringList &strMsgs);
    void ProcessScreenshotMessage(int connectedNumber, const QHostAddress senderHost, const quint16 senderPort, QByteArray &strMsgs);
    void ReceiveMessages(QUdpSocket *sock, int connectedNumber);

    void UpdateDevices();
    void UpdateScreen();
    void UpdateSendInfo();
    void UpdateReceiveInfo();

    int ConfigureSocket(QUdpSocket **sock, int connectedNumber, QHostAddress host, quint16 port);
    void SetState(MyTeamViewerState state);

    void Start();
    void twDevicesCellClicked(int row, int column);
};

#endif // MYTEAMVIEWER_H
