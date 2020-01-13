#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QDateTime>
#include <QHostAddress>
#include <QImage>

enum class MyTeamViewerState {WAITING, CONNECTED_HOST, CONNECTED_CLIENT};
const quint16 START_PORT = 10000;
const int CONNECTED_SOCKETS_COUNT = 3;
const int BROADCAST_WAIT_TIME = 2000; // ms

const int MAX_IPV4_HEADER_SIZE = 60;
const int MAX_UDP_HEADER_SIZE = 8;
const int MTU = 2270;

const int COMPRESSION_LEVEL = 0; // 0 - 9
const int SCREENSHOT_REFRESH_MS = CONNECTED_SOCKETS_COUNT * 20; // this time is for each connectedTimer, i.e. real refresh time is SCREENSHOT_REFRESH_MS/CONNECTED_SOCKETS_COUNT
const int SCREEN_BLOCK_WIDTH = 480;
const int SCREEN_BLOCK_HEIGHT = 1;
const QVector<QImage::Format> IMAGE_FORMATS = {
    QImage::Format_Invalid,
    QImage::Format_Mono,
    QImage::Format_MonoLSB,
    QImage::Format_Indexed8,
    QImage::Format_RGB32,
    QImage::Format_ARGB32,
    QImage::Format_ARGB32_Premultiplied,
    QImage::Format_RGB16,
    QImage::Format_ARGB8565_Premultiplied,
    QImage::Format_RGB666,
    QImage::Format_ARGB6666_Premultiplied,
    QImage::Format_RGB555,
    QImage::Format_ARGB8555_Premultiplied,
    QImage::Format_RGB888,
    QImage::Format_RGB444,
    QImage::Format_ARGB4444_Premultiplied,
    QImage::Format_RGBX8888,
    QImage::Format_RGBA8888,
    QImage::Format_RGBA8888_Premultiplied,
    QImage::Format_BGR30,
    QImage::Format_A2BGR30_Premultiplied,
    QImage::Format_RGB30,
    QImage::Format_A2RGB30_Premultiplied,
    QImage::Format_Alpha8,
    QImage::Format_Grayscale8,
    QImage::Format_RGBX64,
    QImage::Format_RGBA64,
    QImage::Format_RGBA64_Premultiplied
};

struct LastSeenDevice
{
    QDateTime date;
    QHostAddress host;
    quint16 port;
    LastSeenDevice(QDateTime _date = QDateTime::currentDateTime(), QHostAddress _host = QHostAddress::AnyIPv4, quint16 _port = START_PORT) :
        date(_date),
        host(_host),
        port(_port) {}
};

#endif // CONSTANTS_H
