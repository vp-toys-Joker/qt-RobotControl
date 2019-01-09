/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "declaration.h"

#include "btclient.h"

#include <qbluetoothsocket.h>

BtClient::BtClient(QObject *parent)
:   QObject(parent), socket(nullptr)
, count(0)
{
}

BtClient::~BtClient()
{
    stopClient();
}

void BtClient::beginClient(const QBluetoothServiceInfo &remoteService)
{
    startClient(remoteService);
}

//! [startClient]
void BtClient::startClient(const QBluetoothServiceInfo &remoteService)
{
    // Connect to service
    if (socket == nullptr)
    {
        socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
        if (socket == nullptr)
        {
            qDebug() << "Create socket error!";
            emit disconnected(false);
            return;
        }
        connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
        //connect(socket, SIGNAL(connected()), this, SLOT(connectedSocket()));
        connect(socket, &QBluetoothSocket::connected, this, &BtClient::connectedSocket);
        connect(socket, SIGNAL(disconnected()), this, SLOT(disConnected()));
//        connect(socket, &QBluetoothSocket::error, this, &BtClient::errorSocket);
        connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(errorSocket(QBluetoothSocket::SocketError)));
        connect(socket, SIGNAL(stateChanged(QBluetoothSocket::SocketState)), this, SLOT(stateSocket(QBluetoothSocket::SocketState)));
    }
    qDebug() << "Create socket Ok!";
    socket->connectToService(remoteService);
    qDebug() << "ConnectToService done";

}
//! [startClient]

//! [stopClient]
void BtClient::stopClient()
{
    if (socket != nullptr)
    {
        if((socket->error() == QBluetoothSocket::NoSocketError
        || socket->error() == QBluetoothSocket::OperationError)
        && socket->state() == QBluetoothSocket::ConnectedState)
        {
            sendMessage(cmdDisconnect);
            //for(int i = 0; i < 100; ++i){}
        }
        delete socket;
        socket = nullptr;
    }
    emit disconnected(true);
}
//! [stopClient]

//! [readSocket]
void BtClient::readSocket()
{
    if (!socket)
    {
        stopClient();
        return;
    }

    while (socket->canReadLine()) {
        QByteArray line = socket->readLine();
        emit messageReceived(socket->peerName(),
                             QString::fromUtf8(line.constData(), line.length()));
    }
}
//! [readSocket]

//! [sendMessage]
void BtClient::sendMessage(const char &msg)
{
    QString comm;
    switch(msg)
    {
        case cmdBackward:
            comm = "назад";
        break;
        case cmdDisconnect:
            comm = "конец работы";
        break;
        case cmdRightForward:
            comm = "вправо вперед";
        break;
        case cmdLeftForward:
            comm = "влево вперед";
        break;
        case cmdRightBackward:
            comm = "вправо назад";
        break;
        case cmdForward:
            comm = "вперед";
        break;
        case cmdLeftBackward:
            comm = "влево назад";
        break;
        case cmdLeftRotate:
            comm = "разворот влево";
        break;
        case cmdRightRotate:
            comm = "разворот вправо";
        break;
        case cmdStop:
            comm = "стоп";
        break;
        case cmdParam0_On: //включить звук или что-то другое
            comm = "mode = PARAM_0_ON";
        break;
        case cmdParam0_Off: //выключить звук или что-то другое
            comm = "mode = PARAM_0_OFF";
        break;
        case cmdParam1_On: //включить фары или что-то другое
            comm = "mode = PARAM_1_ON";
        break;
        case cmdParam1_Off: //выключить фары или что-то другое
            comm = "mode = PARAM_1_OFF";
        break;
        case cmdParam2_On: //включить задний свет или что-то другое
            comm = "mode = PARAM_2_ON";
        break;
        case cmdParam2_Off: //выключить задний свет или что-то другое
            comm = "mode = PARAM_2_OFF";
        break;
        case cmdParam3_On: //включить дополнительную опцию или что-то другое
            comm = "mode = PARAM_3_ON";
        break;
        case cmdParam3_Off: //выключить дополнительную опцию или что-то другое
            comm = "mode = PARAM_3_OFF";
        break;
        case cmdSpeed0: case cmdSpeed1: case cmdSpeed2: case cmdSpeed3: case cmdSpeed4: case cmdSpeed5:
        case cmdSpeed6: case cmdSpeed7: case cmdSpeed8: case cmdSpeed9: case cmdSpeed10:
            comm = "скорость";
        break;
    }
    qint64 cb = socket->write(&msg, 1);
    if(cb > 0) count = 0;
    else ++count;
    qDebug() << msg << ": " << comm/*.toLatin1()*/ << "write result = " << cb;
    if(count > 3)
        stopClient();
}
//! [sendMessage]

//! [connected]
void BtClient::connectedSocket()
{
    emit devConnected(socket->peerName());
}
//! [connected]

void BtClient::disConnected()
{
//    stopClient();
    disconnected(true);
}

void BtClient::errorSocket(QBluetoothSocket::SocketError error)
{
    QString serr = socket->errorString();
    emit sendSocketState(serr);
//    qDebug() << serr;
    if(error != QBluetoothSocket::NoSocketError
    && error != QBluetoothSocket::OperationError)
    {
        stopClient();
    }
}

void BtClient::stateSocket(QBluetoothSocket::SocketState state)
{
    QString sstate;
    switch(state)
    {
        case QBluetoothSocket::UnconnectedState:
        sstate = "UnconnectedState";
        break;
        case QBluetoothSocket::ServiceLookupState:
        sstate = "ServiceLookupState";
        break;
        case QBluetoothSocket::ConnectingState:
        sstate = "ConnectingState";
        break;
        case QBluetoothSocket::ConnectedState:
        sstate = "ConnectedState";
        break;
        case QBluetoothSocket::BoundState:
        sstate = "BoundState";
        break;
        case QBluetoothSocket::ClosingState:
        sstate = "ClosingState";
        break;
        case QBluetoothSocket::ListeningState:
        sstate = "ListeningState";
        break;
    }
//    qDebug() << sstate;
    emit sendSocketState(sstate);
}
