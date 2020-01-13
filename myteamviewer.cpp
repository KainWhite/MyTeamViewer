#include "myteamviewer.h"
#include "ui_myteamviewer.h"

#include <QTcpSocket>
#include <QNetworkDatagram>
#include <QDateTime>
#include <QSignalMapper>
#include <QPushButton>
#include <QMessageBox>
#include <QWindow>
#include <QScreen>
#include <QBuffer>
#include <QPainter>
#include <QThread>
#include <QScrollBar>

MyTeamViewer::MyTeamViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MyTeamViewer),
    tmrBroadcast(new QTimer),
    tmrUpdateDevices(new QTimer),
    tmrUpdateScreen(new QTimer),
    tmrScreensSent(new QTimer),
    tmrScreensReceived(new QTimer)
{
    ui->setupUi(this);
    ui->twDevices->setColumnCount(1);
    ui->txtLog->setVisible(false);
    /*ui->twDevices->verticalScrollBar()->setStyleSheet(
                "QScrollBar:vertical {"
                    "background: #e6e6e6;"
                    "width: 20px;"
                    "margin-bottom: 7px;"
                "}"
                "QScrollBar::handle:vertical {"
                    "background: #ffffff;"
                    "min-height: 10px;"
                "}"
                "QScrollBar::add-line:vertical {"
                    "border: 2px solid grey;"
                    "background: #e6e6e6;"
                    "height: 10px;"
                    "subcontrol-position: bottom;"
                    "subcontrol-origin: margin;"
                "}"

                "QScrollBar::sub-line:vertical {"
                    "display: none;"

                "}"
                "QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {"
                    "border: 2px solid grey;"
                    "width: 3px;"
                    "height: 3px;"
                    "background: white;"
                "}"

                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                    "background: none;"
                "}"
    );*/

    QHostAddress currentlyConnectedIP = GetCurrentlyConnectedIP();
    if(currentlyConnectedIP == QHostAddress::Null)
        return;

    connect(tmrBroadcast, &QTimer::timeout, this, [this] {
        SendMessages(
            this->waitingSocket,
            //new QUdpSocket(this),
            GetBroadcastIntroduceMessages(),
            QHostAddress::Broadcast,
            START_PORT
        );
    });
    connect(tmrUpdateDevices, &QTimer::timeout, this, &MyTeamViewer::UpdateDevices);
    connect(tmrUpdateScreen, &QTimer::timeout, this, &MyTeamViewer::UpdateScreen);
    connect(tmrScreensSent, &QTimer::timeout, this, &MyTeamViewer::UpdateSendInfo);
    connect(tmrScreensReceived, &QTimer::timeout, this, &MyTeamViewer::UpdateReceiveInfo);
    tmrsScreenshot = QVector<QTimer*>(CONNECTED_SOCKETS_COUNT, new QTimer);

    ConfigureSocket(&waitingSocket, -1, currentlyConnectedIP, START_PORT);
    //waitingSocket->writeDatagram("a", 1, currentlyConnectedIP, 10000);

    connectedPorts = QVector<quint16>(CONNECTED_SOCKETS_COUNT);
    connectedHosts = QVector<QHostAddress>(CONNECTED_SOCKETS_COUNT);

    screenImage = QImage(1920, 1080, QImage::Format_RGB32);

    connect(ui->twDevices, &QTableWidget::cellClicked, this, &MyTeamViewer::twDevicesCellClicked);

    for(int i = 0; i < CONNECTED_SOCKETS_COUNT; i++)
    {
        ConfigureSocket(&connectedSockets[i], i, currentlyConnectedIP, START_PORT);
        connect(tmrsScreenshot[i], &QTimer::timeout, this, [i, this] {
            SendMessages(
                connectedSockets[i],
                GetScreenshotMessages(),
                connectedHosts[i],
                connectedPorts[i]
            );
        });
    }

    Start();
}

MyTeamViewer::~MyTeamViewer()
{
    delete ui;
}

QHostAddress MyTeamViewer::GetCurrentlyConnectedIP()
{
    QTcpSocket *test = new QTcpSocket();
    test->connectToHost("google.com", 80);
    if (test->waitForConnected(5000))
    {
        return test->localAddress();
    }
    else
    {
        return QHostAddress::Null;
    }
}

void MyTeamViewer::WaitNS(quint64 nanoseconds)
{
    for(int i = 0; i < nanoseconds; i++);
}

QVector<QByteArray> MyTeamViewer::GetBroadcastIntroduceMessages()
{
    return QVector<QByteArray>(1, "--name " + QSysInfo::machineHostName().toUtf8());
}
QVector<QByteArray> MyTeamViewer::GetConnectMessages()
{
    QByteArray msg = "--connect " + QSysInfo::machineHostName().toUtf8();
    for(int i = 0; i < CONNECTED_SOCKETS_COUNT; i++)
    {
        msg += " " + connectedSockets[i]->localAddress().toString() +
               " " + QByteArray::number(connectedSockets[i]->localPort());
    }
    return QVector<QByteArray>(1, msg);
}
QVector<QByteArray> MyTeamViewer::GetScreenshotMessages()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    screenImage = screen->grabWindow(0).toImage();
    //ui->txtLog->append(QString::number(screenImage.width()));
    QVector<QByteArray> screenVector;
    for(int i = 0; i < screenImage.height(); i += SCREEN_BLOCK_HEIGHT)
    {
        for(int j = 0; j < screenImage.width(); j += SCREEN_BLOCK_WIDTH)
        {
            QImage screenBlock = screenImage.copy(j, i, SCREEN_BLOCK_WIDTH, SCREEN_BLOCK_HEIGHT);
            QByteArray screenBlockBytes = QByteArray::fromRawData((const char*)screenBlock.bits(), screenBlock.sizeInBytes());
            screenBlockBytes.push_back((screenBlockNum >> 24) & 0xff);
            screenBlockBytes.push_back((screenBlockNum >> 16) & 0xff);
            screenBlockBytes.push_back((screenBlockNum >> 8) & 0xff);
            screenBlockBytes.push_back((screenBlockNum >> 0) & 0xff);
            screenBlockBytes.push_back((screenImage.width() >> 8) & 0xff);
            screenBlockBytes.push_back((screenImage.width() >> 0) & 0xff);
            screenBlockBytes.push_back((screenImage.height() >> 8) & 0xff);
            screenBlockBytes.push_back((screenImage.height() >> 0) & 0xff);
            screenBlockBytes.push_back((screenImage.format() >> 0) & 0xff);
            //screenBlockBytes = qCompress(screenBlockBytes, COMPRESSION_LEVEL);
            screenBlockNum++;
            screenVector.push_back(screenBlockBytes);
        }
    }
    return screenVector;
    //return QVector<QByteArray>(1, screenBytes);
    //return QVector<QByteArray>(1, "lol");
}
void MyTeamViewer::SendMessages(QUdpSocket *sock, const QVector<QByteArray> &messages, const QHostAddress host, quint16 port)
{
    for(int i = 0; i < messages.size(); i++)
    {
        QNetworkDatagram datagram(messages[i], host, port);
        if(sock->writeDatagram(datagram) == -1)
        {
            ui->txtLog->append("message unsuccessfull");
            return;
        }

//        if(sock == waitingSocket)
//        {
//            datagram.setDestination(host, port + CONNECTED_SOCKETS_COUNT + 1);
//            sock->writeDatagram(datagram);
//        }

        if(sock != waitingSocket)
        {
            //ui->txtLog->append(host.toString() + " " + QString::number(port));
            screensSent++;
            //WaitNS(1000);
        }
    }
}

void MyTeamViewer::ProcessIntroduceMessage(const QHostAddress senderHost, const quint16 senderPort, QStringList &strMsgs)
{
    if(!avaliableDevices.contains(strMsgs[1]))
    {
        avaliableDevices.insert(strMsgs[1]);
    }
    avaliableDeviceInfo[strMsgs[1]] = LastSeenDevice(QDateTime::currentDateTime(), senderHost, senderPort);
}
void MyTeamViewer::ProcessConnectMessage(const QHostAddress senderHost, const quint16 senderPort, QStringList &strMsgs)
{
    if(myTeamViewerState != MyTeamViewerState::WAITING)
    {
        return;
    }
    waitingHost = senderHost;
    waitingPort = senderPort;
    waitingName = strMsgs[1];
    for(int i = 2; i < strMsgs.size(); i += 2)
    {
        connectedHosts[(i - 2) / 2] = QHostAddress(strMsgs[i]);
        connectedPorts[(i - 2) / 2] = strMsgs[i + 1].toUInt();
        ui->txtLog->append( connectedHosts[(i - 2) / 2].toString() + " " + QString::number(connectedPorts[(i - 2) / 2]));
    }
    screenBlockNum = 0;
    ui->txtLog->append("connect accepted");
    SetState(MyTeamViewerState::CONNECTED_HOST);
}
void MyTeamViewer::ProcessScreenshotMessage(int connectedNumber, const QHostAddress senderHost, const quint16 senderPort, QByteArray &msg)
{
    //qDebug() << connectedNumber << connectedHosts[connectedNumber] << connectedPorts[connectedNumber];
    /*if(senderHost != connectedHosts[connectedNumber] || senderPort != connectedPorts[connectedNumber]) // need to send back theese values from host
        return;*/
    QByteArray screenBlockBytes = msg;//= qUncompress(msg);
    quint8 format = (uchar(screenBlockBytes[screenBlockBytes.size() - 1]) << 0);
    int screenHeight = (uchar(screenBlockBytes[screenBlockBytes.size() - 2]) << 0) +
                       (uchar(screenBlockBytes[screenBlockBytes.size() - 3]) << 8);
    int screenWidth = (uchar(screenBlockBytes[screenBlockBytes.size() - 4]) << 0) +
                      (uchar(screenBlockBytes[screenBlockBytes.size() - 5]) << 8);
    screenBlockNum = (uchar(screenBlockBytes[screenBlockBytes.size() - 6]) << 0) +
                     (uchar(screenBlockBytes[screenBlockBytes.size() - 7]) << 8) +
                     (uchar(screenBlockBytes[screenBlockBytes.size() - 8]) << 16) +
                     (uchar(screenBlockBytes[screenBlockBytes.size() - 9]) << 24);
    /*qDebug() << (uchar(screenBlockBytes[screenBlockBytes.size() - 4]) << 0) << " + "
             << (uchar(screenBlockBytes[screenBlockBytes.size() - 5]) << 8) << " = "
             << screenWidth << " ; "
             << (uchar(screenBlockBytes[screenBlockBytes.size() - 2]) << 0) << " + "
             << (uchar(screenBlockBytes[screenBlockBytes.size() - 3]) << 8) << " = "
             << screenHeight << endl;*/
    screenBlockBytes.remove(screenBlockBytes.size() - 9, 9);

    QImage screenBlock(
        reinterpret_cast<uchar*>(screenBlockBytes.data()),
        SCREEN_BLOCK_WIDTH,
        SCREEN_BLOCK_HEIGHT,
        IMAGE_FORMATS[format]
    );
    if(screenImage.width() != screenWidth ||
       screenImage.height() != screenHeight ||
       screenImage.format() != IMAGE_FORMATS[format])
    {
        screenImage = screenImage.scaled(screenWidth, screenHeight).convertToFormat(IMAGE_FORMATS[format]);
        //screenImage.convertToFormat(IMAGE_FORMATS[format]);
    }
    QPainter painter(&screenImage);
    painter.drawImage(
        (screenBlockNum % (screenWidth / SCREEN_BLOCK_WIDTH)) * SCREEN_BLOCK_WIDTH,
        ((screenBlockNum / (screenWidth / SCREEN_BLOCK_WIDTH)) % (screenHeight / SCREEN_BLOCK_HEIGHT)) * SCREEN_BLOCK_HEIGHT,
        screenBlock
    );
}
void MyTeamViewer::ReceiveMessages(QUdpSocket *sock, int connectedNumber)
{
    QByteArray msg;
    while (sock->hasPendingDatagrams())
    {
        QHostAddress senderHost;
        quint16 senderPort;
        msg.resize(int(sock->pendingDatagramSize()));
        if(sock->readDatagram(msg.data(), msg.size(), &senderHost, &senderPort) == -1)
            return;
        if(sock == waitingSocket)
        {
            QStringList strMsgs(((QString)msg).split(" "));
            if(senderHost != sock->localAddress() || senderPort != sock->localPort())
            {
                if(strMsgs[0] == "--name")
                {
                    ProcessIntroduceMessage(senderHost, senderPort, strMsgs);
                }
                else if(strMsgs[0] == "--connect")
                {
                    ProcessConnectMessage(senderHost, senderPort, strMsgs);
                }
            }
        }
        else
        {
            //qDebug() << "LOL";
            screensReceived++;
            ProcessScreenshotMessage(connectedNumber, senderHost, senderPort, msg);
        }
    }
}

void MyTeamViewer::UpdateDevices()
{
    ui->twDevices->setRowCount(0);
    QDateTime now = QDateTime::currentDateTime();
    for(QSet<QString>::iterator it = avaliableDevices.begin(); it != avaliableDevices.end(); it++)
    {
        if(avaliableDeviceInfo[*it].date.msecsTo(now) > BROADCAST_WAIT_TIME)
        {
            avaliableDevices.remove(*it);
        }
        else
        {
            ui->twDevices->insertRow(ui->twDevices->rowCount());
            ui->twDevices->setRowHeight(ui->twDevices->rowCount() - 1, 50);
            ui->twDevices->setColumnWidth(0, 150);
            ui->twDevices->setItem(ui->twDevices->rowCount() - 1, 0, new QTableWidgetItem(*it));
        }
    }
}
void MyTeamViewer::UpdateScreen()
{
    //QPixmap::fromImage(screenImage).sca
    ui->lblScreen->setPixmap(QPixmap::fromImage(screenImage).scaled(ui->lblScreen->size(), Qt::KeepAspectRatio));
}
void MyTeamViewer::UpdateSendInfo()
{
    ui->txtLog->append(QString::number(screensSent));
}
void MyTeamViewer::UpdateReceiveInfo()
{
    ui->txtLog->append(QString::number(screensReceived));
}

int MyTeamViewer::ConfigureSocket(QUdpSocket **sock, int connectedNumber, QHostAddress host, quint16 port)
{
    *sock = new QUdpSocket(this);
    while(port != 0 && !(*sock)->bind(host, port))
    {
        port++;
    }
    if(port == 0)
    {
        ui->txtLog->append(
            "can't bind to(" +
            host.toString() + ", " +
            QString::number(START_PORT) + " - " + QString::number(1<<16) + ")\n"
        );
        return 1;
    }
    ui->txtLog->append("start successfull(" + host.toString() + ", " + QString::number(port) + ")\n");
    connect(*sock, &QUdpSocket::readyRead, this, [sock, connectedNumber, this] {
        ReceiveMessages(*sock, connectedNumber);
    });
    return 0;
}
void MyTeamViewer::SetState(MyTeamViewerState state)
{
    myTeamViewerState = state;
    screensSent = 0;
    screensReceived = 0;
    switch(state)
    {
    case MyTeamViewerState::WAITING :
        tmrUpdateScreen->stop();
        for(int i = 0; i < tmrsScreenshot.size(); i++)
        {
            tmrsScreenshot[i]->stop();
        }
        break;
    case MyTeamViewerState::CONNECTED_HOST :
        for(int i = 0; i < tmrsScreenshot.size(); i++)
        {
            tmrsScreenshot[i]->start(SCREENSHOT_REFRESH_MS);
            QThread::usleep(SCREENSHOT_REFRESH_MS * 1000 / CONNECTED_SOCKETS_COUNT);
        }
        tmrScreensSent->start(1000);
        break;
    case MyTeamViewerState::CONNECTED_CLIENT :
        tmrUpdateScreen->start(SCREENSHOT_REFRESH_MS / CONNECTED_SOCKETS_COUNT);
        tmrScreensReceived->start(1000);
        break;
    }
}

void MyTeamViewer::Start()
{
    tmrBroadcast->start(1000);
    tmrUpdateDevices->start(1000);
    SetState(MyTeamViewerState::WAITING);
}

void MyTeamViewer::twDevicesCellClicked(int row, int column)
{
    waitingName = ui->twDevices->item(row, column)->text();
    waitingHost = avaliableDeviceInfo[waitingName].host;
    waitingPort = avaliableDeviceInfo[waitingName].port;
    SendMessages(
        waitingSocket,
        GetConnectMessages(),
        waitingHost,
        waitingPort
    );
    SetState(MyTeamViewerState::CONNECTED_CLIENT);
}
