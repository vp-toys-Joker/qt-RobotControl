#include "declaration.h"

#include <QMessageBox>
#include <QKeyEvent>
#include "RobotControl.h"

#include "btclient.h"
#include <qbluetoothaddress.h>
#include <qbluetoothservicediscoveryagent.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <QMenu>
#include <QDebug>


RobotControlDlg::RobotControlDlg(QWidget *parent) :
      QDialog(parent)
    , deviceName("")
    , deviceAdress("")
    , ui(new Ui_RobotControl)
    , discoveryAgent(nullptr)
    , localDevice(new QBluetoothLocalDevice)
    , discoverySeviceAgent(nullptr)
    , client(nullptr)
    , forwardPressed(false)
    , backwardPressed(false)
    , leftPressed(false)
    , rightPressed(false)
    , pageupPressed(false)
    , pagednPressed(false)
    , homePressed(false)
    , endPressed(false)
    , connectedMode(false)
{
    ui->setupUi(this);
    Title = windowTitle();
    ui->speedControl->installEventFilter(this);
    ui->list->installEventFilter(this);

    client = new BtClient(this);

    /*
     * In case of multiple Bluetooth adapters it is possible to set adapter
     * which will be used. Example code:
     *
     * QBluetoothAddress address("XX:XX:XX:XX:XX:XX");
     * discoveryAgent = new QBluetoothDeviceDiscoveryAgent(address);
     *
     **/

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();

    connect(ui->quit, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->inquiryType, SIGNAL(toggled(bool)), this, SLOT(setGeneralUnlimited(bool)));
    connect(ui->scan, SIGNAL(clicked()), this, SLOT(startScan()));

    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(addDevice(QBluetoothDeviceInfo)));
    connect(discoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));

    connect(ui->list, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(itemActivated(QListWidgetItem*)));

    connect(localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)),
            this, SLOT(hostModeStateChanged(QBluetoothLocalDevice::HostMode)));

    hostModeStateChanged(localDevice->hostMode());
    // add context menu for devices to be able to pair device
    ui->list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->list, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayPairingMenu(QPoint)));
    connect(localDevice, SIGNAL(pairingFinished(QBluetoothAddress,QBluetoothLocalDevice::Pairing))
        , this, SLOT(pairingDone(QBluetoothAddress,QBluetoothLocalDevice::Pairing)));
}

bool RobotControlDlg::eventFilter(QObject *target, QEvent *event)
{
    bool res = false;
    if(target == ui->list || target == ui->speedControl)
    {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up
        || keyEvent->key() == Qt::Key_Down
        || keyEvent->key() == Qt::Key_Plus
        || keyEvent->key() == Qt::Key_Minus)
        {
          if(connectedMode)
          {
              res = true;
              if (event->type() == QEvent::KeyPress)
              {
                  keyPressEvent(keyEvent);
              }
              else /*if(event->type() == QEvent::KeyRelease) */
              {
                  keyReleaseEvent(keyEvent);
              }
          }
          else if(target == ui->speedControl
               || keyEvent->key() == Qt::Key_Up
               || keyEvent->key() == Qt::Key_Down) res = true;
        }
    }
    return res;
}

RobotControlDlg::~RobotControlDlg()
{
    if(client) delete client;
    if(discoverySeviceAgent) delete discoverySeviceAgent;
    if(discoveryAgent) delete discoveryAgent;
    if(ui) delete ui;
}

void RobotControlDlg::addDevice(const QBluetoothDeviceInfo &info)
{
    QString label = QString("%1 %2").arg(info.address().toString()).arg(info.name());
    QList<QListWidgetItem *> items = ui->list->findItems(label, Qt::MatchExactly);
    if (items.empty()) {
        QListWidgetItem *item = new QListWidgetItem(label);
        QBluetoothLocalDevice::Pairing pairingStatus = localDevice->pairingStatus(info.address());
        if (pairingStatus == QBluetoothLocalDevice::Paired || pairingStatus == QBluetoothLocalDevice::AuthorizedPaired )
            item->setTextColor(QColor(Qt::green));
        else
            item->setTextColor(QColor(Qt::black));

        ui->list->addItem(item);
    }

}

void RobotControlDlg::startScan()
{
    discoveryAgent->start();
    ui->scan->setEnabled(false);
    ui->inquiryType->setEnabled(false);
}

void RobotControlDlg::scanFinished()
{
    ui->scan->setEnabled(true);
    ui->inquiryType->setEnabled(true);
}

void RobotControlDlg::setGeneralUnlimited(bool unlimited)
{
    if (unlimited)
        discoveryAgent->setInquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry);
    else
        discoveryAgent->setInquiryType(QBluetoothDeviceDiscoveryAgent::LimitedInquiry);
}

void RobotControlDlg::itemActivated(QListWidgetItem *item)
{
    QString text = item->text();

    int index = text.indexOf(' ');

    if (index == -1)
        return;
    deviceAdress = text.left(index);
    deviceName = text.mid(index + 1);
    QBluetoothAddress address(deviceAdress);
    QBluetoothLocalDevice localDevice;
    QBluetoothAddress adapterAddress = localDevice.address();

    if(!discoverySeviceAgent)
        discoverySeviceAgent = new QBluetoothServiceDiscoveryAgent(adapterAddress);
    if(discoverySeviceAgent)
    {
        discoverySeviceAgent->setRemoteAddress(address);

        QString title("%1 %2 %3");
        setWindowTitle(title.arg(Title, "ожидает соединения с", deviceName));

        connect(discoverySeviceAgent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)),
                client, SLOT(beginClient(QBluetoothServiceInfo)));
        connect(client, SIGNAL(devConnected(QString)), this, SLOT(clientReady(QString)));
//        connect(client, SIGNAL(failConnected()), this, SLOT(clientFail()));
        connect(client, SIGNAL(disconnected(bool)), this, SLOT(disconnect(bool)));
//        connect(client, SIGNAL(sendSocketError(QString)), this, SLOT(logWrite(QString)));
        connect(client, SIGNAL(sendSocketState(QString)), this, SLOT(logWrite(QString)));

        discoverySeviceAgent->start();
    }
}

void RobotControlDlg::clientReady(const QString peerName)
{
    QString title;
    if(deviceAdress == peerName.mid(1,peerName.size()-2))
    {
        title = QString("%1 соединен с %2").arg(Title).arg(deviceName);
    }
    else
    {
        title = QString("%1 соединен с %2").arg(Title).arg(peerName.mid(1,peerName.size()-2));
    }
    setWindowTitle(title);
//    connect(this,SIGNAL(sendMessage(char)),client,SLOT(sendMessage(char)));
//    connect(ui->forwardButton, SIGNAL(pressed()), this,  SLOT(sendForwardPressButtonMessage()));
//    connect(ui->backwardButton, SIGNAL(pressed()), this, SLOT(sendBackwardPressButtonMessage()));
//    connect(ui->leftButton, SIGNAL(pressed()), this,     SLOT(sendLeftPressButtonMessage()));
//    connect(ui->rightButton, SIGNAL(pressed()), this,    SLOT(sendRightPressButtonMessage()));
//    connect(ui->forwardButton, SIGNAL(released()), this, SLOT(sendForwardReleasedButtonMessage()));
//    connect(ui->backwardButton,SIGNAL(released()), this, SLOT(sendBackwardReleasedButtonMessage()));
//    connect(ui->leftButton,  SIGNAL(released()), this,   SLOT(sendLeftReleasedButtonMessage()));
//    connect(ui->rightButton, SIGNAL(released()), this,   SLOT(sendRightReleasedButtonMessage()));
//    connect(ui->forwardrightButton,  SIGNAL(pressed()),  this, SLOT(sendForwardRightPressButtonMessage()));
//    connect(ui->forwardrightButton,  SIGNAL(released()), this, SLOT(sendForwardRightReleasedButtonMessage()));
//    connect(ui->forwardleftButton,   SIGNAL(pressed()),  this, SLOT(sendForwardLeftPressButtonMessage()));
//    connect(ui->forwardleftButton,   SIGNAL(released()), this, SLOT(sendForwardLeftReleasedButtonMessage()));
//    connect(ui->backwardrightButton, SIGNAL(pressed()),  this, SLOT(sendBackwardRightPressButtonMessage()));
//    connect(ui->backwardrightButton, SIGNAL(released()), this, SLOT(sendBackwardRightReleasedButtonMessage()));
//    connect(ui->backwardleftButton,  SIGNAL(pressed()),  this, SLOT(sendBackwardLeftPressButtonMessage()));
//    connect(ui->backwardleftButton,  SIGNAL(released()), this, SLOT(sendBackwardLeftReleasedButtonMessage()));
    connect(this,&RobotControlDlg::sendMessage,client,&BtClient::sendMessage);
    connect(ui->forwardButton, &QPushButton::pressed, this,  &RobotControlDlg::sendForwardPressButtonMessage);
    connect(ui->backwardButton, &QPushButton::pressed, this, &RobotControlDlg::sendBackwardPressButtonMessage);
    connect(ui->leftButton, &QPushButton::pressed, this,     &RobotControlDlg::sendLeftPressButtonMessage);
    connect(ui->rightButton, &QPushButton::pressed, this,    &RobotControlDlg::sendRightPressButtonMessage);
    connect(ui->forwardButton, &QPushButton::released, this,  &RobotControlDlg::sendForwardReleasedButtonMessage);
    connect(ui->backwardButton, &QPushButton::released, this, &RobotControlDlg::sendBackwardReleasedButtonMessage);
    connect(ui->leftButton, &QPushButton::released, this,     &RobotControlDlg::sendLeftReleasedButtonMessage);
    connect(ui->rightButton, &QPushButton::released, this,    &RobotControlDlg::sendRightReleasedButtonMessage);
    connect(ui->forwardrightButton, &QPushButton::pressed,   this, &RobotControlDlg::sendForwardRightPressButtonMessage);
    connect(ui->forwardrightButton, &QPushButton::released,  this, &RobotControlDlg::sendForwardRightReleasedButtonMessage);
    connect(ui->forwardleftButton,   &QPushButton::pressed, this,  &RobotControlDlg::sendForwardLeftPressButtonMessage);
    connect(ui->forwardleftButton,   &QPushButton::released, this, &RobotControlDlg::sendForwardLeftReleasedButtonMessage);
    connect(ui->backwardrightButton, &QPushButton::pressed, this,  &RobotControlDlg::sendBackwardRightPressButtonMessage);
    connect(ui->backwardrightButton, &QPushButton::released ,this, &RobotControlDlg::sendBackwardRightReleasedButtonMessage);
    connect(ui->backwardleftButton,  &QPushButton::pressed, this,  &RobotControlDlg::sendBackwardLeftPressButtonMessage);
    connect(ui->backwardleftButton,  &QPushButton::released, this, &RobotControlDlg::sendBackwardLeftReleasedButtonMessage);
    connect(ui->speedControl,  &QSlider::sliderMoved, this, &RobotControlDlg::sendSpeedMessage);
    connect(ui->speedControl,  &QSlider::valueChanged, this, &RobotControlDlg::sendSpeedMessage);
}

void RobotControlDlg::clientFail()
{
    QString title = QString("%1 ошибка соединения с %2").arg(Title).arg(deviceName);
    setWindowTitle(title);
    disconnect(false);
}

void RobotControlDlg::disconnect(bool state)
{
//    QObject::disconnect(this,SIGNAL(sendMessage),client,SLOT(sendMessage(char)));
//    QObject::disconnect(ui->forwardButton, SIGNAL(pressed()), this,  SLOT(sendForwardPressButtonMessage()));
//    QObject::disconnect(ui->backwardButton, SIGNAL(pressed()), this, SLOT(sendBackwardPressButtonMessage()));
//    QObject::disconnect(ui->leftButton, SIGNAL(pressed()), this,     SLOT(sendLeftPressButtonMessage()));
//    QObject::disconnect(ui->rightButton, SIGNAL(pressed()), this,    SLOT(sendRightPressButtonMessage()));
//    QObject::disconnect(ui->forwardButton, SIGNAL(released()), this, SLOT(sendForwardReleasedButtonMessage()));
//    QObject::disconnect(ui->backwardButton,SIGNAL(released()), this, SLOT(sendBackwardReleasedButtonMessage()));
//    QObject::disconnect(ui->leftButton,  SIGNAL(released()), this,   SLOT(sendLeftReleasedButtonMessage()));
//    QObject::disconnect(ui->rightButton, SIGNAL(released()), this,   SLOT(sendRightReleasedButtonMessage()));
//    QObject::disconnect(ui->forwardrightButton,  SIGNAL(pressed()),  this, SLOT(sendForwardRightPressButtonMessage()));
//    QObject::disconnect(ui->forwardrightButton,  SIGNAL(released()), this, SLOT(sendForwardRightReleasedButtonMessage()));
//    QObject::disconnect(ui->forwardleftButton,   SIGNAL(pressed()),  this, SLOT(sendForwardLeftPressButtonMessage()));
//    QObject::disconnect(ui->forwardleftButton,   SIGNAL(released()), this, SLOT(sendForwardLeftReleasedButtonMessage()));
//    QObject::disconnect(ui->backwardrightButton, SIGNAL(pressed()),  this, SLOT(sendBackwardRightPressButtonMessage()));
//    QObject::disconnect(ui->backwardrightButton, SIGNAL(released()), this, SLOT(sendBackwardRightReleasedButtonMessage()));
//    QObject::disconnect(ui->backwardleftButton,  SIGNAL(pressed()),  this, SLOT(sendBackwardLeftPressButtonMessage()));
//    QObject::disconnect(ui->backwardleftButton,  SIGNAL(released()), this, SLOT(sendBackwardLeftReleasedButtonMessage()));
    QObject::disconnect(this,&RobotControlDlg::sendMessage,client,&BtClient::sendMessage);
    QObject::disconnect(ui->forwardButton, &QPushButton::pressed, this,  &RobotControlDlg::sendForwardPressButtonMessage);
    QObject::disconnect(ui->backwardButton, &QPushButton::pressed, this, &RobotControlDlg::sendBackwardPressButtonMessage);
    QObject::disconnect(ui->leftButton, &QPushButton::pressed, this,     &RobotControlDlg::sendLeftPressButtonMessage);
    QObject::disconnect(ui->rightButton, &QPushButton::pressed, this,    &RobotControlDlg::sendRightPressButtonMessage);
    QObject::disconnect(ui->forwardButton, &QPushButton::released, this,  &RobotControlDlg::sendForwardReleasedButtonMessage);
    QObject::disconnect(ui->backwardButton, &QPushButton::released, this, &RobotControlDlg::sendBackwardReleasedButtonMessage);
    QObject::disconnect(ui->leftButton, &QPushButton::released, this,     &RobotControlDlg::sendLeftReleasedButtonMessage);
    QObject::disconnect(ui->rightButton, &QPushButton::released, this,    &RobotControlDlg::sendRightReleasedButtonMessage);
    QObject::disconnect(ui->forwardrightButton, &QPushButton::pressed,this, &RobotControlDlg::sendForwardRightPressButtonMessage);
    QObject::disconnect(ui->forwardrightButton, &QPushButton::released,this, &RobotControlDlg::sendForwardRightReleasedButtonMessage);
    QObject::disconnect(ui->forwardleftButton, &QPushButton::pressed,this,  &RobotControlDlg::sendForwardLeftPressButtonMessage);
    QObject::disconnect(ui->forwardleftButton, &QPushButton::released,this,  &RobotControlDlg::sendForwardLeftReleasedButtonMessage);
    QObject::disconnect(ui->backwardrightButton, &QPushButton::pressed,this,&RobotControlDlg::sendBackwardRightPressButtonMessage);
    QObject::disconnect(ui->backwardrightButton, &QPushButton::released,this,&RobotControlDlg::sendBackwardRightReleasedButtonMessage);
    QObject::disconnect(ui->backwardleftButton, &QPushButton::pressed,this, &RobotControlDlg::sendBackwardLeftPressButtonMessage);
    QObject::disconnect(ui->backwardleftButton, &QPushButton::released,this, &RobotControlDlg::sendBackwardLeftReleasedButtonMessage);
    QObject::disconnect(ui->speedControl,  &QSlider::sliderMoved, this, &RobotControlDlg::sendSpeedMessage);
    QObject::disconnect(ui->speedControl,  &QSlider::valueChanged, this, &RobotControlDlg::sendSpeedMessage);
    setWindowTitle(Title);
    if(!state)
    {
        QMessageBox::StandardButton n = QMessageBox::critical(this,"Critical error","Memory error! Continue?",QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(n == QMessageBox::No)
        {
            ui->quit->setDown(true);
            ui->quit->pressed();
        }
    }
}

void RobotControlDlg::logWrite(QString text)
{
    qDebug() << text;
}

void RobotControlDlg::on_discoverable_clicked(bool clicked)
{
    if (clicked)
        localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    else
        localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
}

void RobotControlDlg::on_power_clicked(bool clicked)
{
    if (clicked)
        localDevice->powerOn();
    else
        localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
}

void RobotControlDlg::hostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
    if (mode != QBluetoothLocalDevice::HostPoweredOff)
        ui->power->setChecked(true);
    else
       ui->power->setChecked( false);

    if (mode == QBluetoothLocalDevice::HostDiscoverable)
        ui->discoverable->setChecked(true);
    else
        ui->discoverable->setChecked(false);

    bool on = !(mode == QBluetoothLocalDevice::HostPoweredOff);


    ui->scan->setEnabled(on);
    ui->discoverable->setEnabled(on);
}

void RobotControlDlg::displayPairingMenu(const QPoint &pos)
{
    if (ui->list->count() == 0)
        return;
    QMenu menu(this);
    QAction *pairAction = menu.addAction("Pair");
    QAction *removePairAction = menu.addAction("Remove Pairing");
    QAction *chosenAction = menu.exec(ui->list->viewport()->mapToGlobal(pos));
    QListWidgetItem *currentItem = ui->list->currentItem();

    QString text = currentItem->text();
    int index = text.indexOf(' ');
    if (index == -1)
        return;

    QBluetoothAddress address (text.left(index));
    if (chosenAction == pairAction) {
        localDevice->requestPairing(address, QBluetoothLocalDevice::Paired);
    } else if (chosenAction == removePairAction) {
        localDevice->requestPairing(address, QBluetoothLocalDevice::Unpaired);
    }
}

void RobotControlDlg::pairingDone(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing)
{
    QList<QListWidgetItem *> items = ui->list->findItems(address.toString(), Qt::MatchContains);

    if (pairing == QBluetoothLocalDevice::Paired || pairing == QBluetoothLocalDevice::AuthorizedPaired ) {
        for (int var = 0; var < items.count(); ++var) {
            QListWidgetItem *item = items.at(var);
            item->setTextColor(QColor(Qt::green));
        }
    } else {
        for (int var = 0; var < items.count(); ++var) {
            QListWidgetItem *item = items.at(var);
            item->setTextColor(QColor(Qt::red));
        }
    }
}

void RobotControlDlg::sendForwardPressButtonMessage()
{
    if(!ui->backwardButton->isDown())
    {
        char msg;
        setAlterButtonsEnable(false);
        if(ui->leftButton->isDown())
            msg = cmdLeftForward;//влево вперед
        else if(ui->rightButton->isDown())
            msg = cmdRightForward;//вправо вперед
        else msg = cmdForward;//вперед
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendBackwardPressButtonMessage()
{
    if(!ui->forwardButton->isDown())
    {
        char msg;
        setAlterButtonsEnable(false);
        if(ui->leftButton->isDown())
            msg = cmdLeftBackward;//влево назад
        else if(ui->rightButton->isDown())
            msg = cmdRightBackward;//вправо назад
        else msg = cmdBackward;//назад
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendLeftPressButtonMessage()
{
    if(!ui->rightButton->isDown())
    {
        char msg;
        setAlterButtonsEnable(false);
        if(!ui->backwardButton->isDown()
        && !ui->forwardButton->isDown())
            msg = cmdLeftRotate;//разворот влево
        else if(ui->backwardButton->isDown()
             && !ui->forwardButton->isDown())
                msg = cmdLeftBackward;//влево назад
        else /*if(!ui->backwardButton->isDown()
             && ui->forwardButton->isDown())*/
                msg = cmdLeftForward;//влево вперед
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendRightPressButtonMessage()
{
    if(!ui->leftButton->isDown())
    {
        char msg;
        setAlterButtonsEnable(false);
        if(!ui->backwardButton->isDown()
        && !ui->forwardButton->isDown())
            msg = cmdRightRotate;//разворот вправо
        else if(ui->backwardButton->isDown()
             && !ui->forwardButton->isDown())
                msg = cmdRightBackward;//вправо назад
        else /*if(!ui->backwardButton->isDown()
             && ui->forwardButton->isDown())*/
                msg = cmdRightForward;//вправо вперед
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendForwardRightPressButtonMessage()
{
    if(!isBaseButtonsDown()
    && !ui->forwardleftButton->isDown()
    && !ui->backwardleftButton->isDown()
    && !ui->backwardrightButton->isDown())
    {
        setBaseButtonsEnable(false);
        ui->forwardleftButton->setEnabled(false);
        ui->backwardleftButton->setEnabled(false);
        ui->backwardrightButton->setEnabled(false);
        emit sendMessage(cmdRightForward);
    }
}
void RobotControlDlg::sendBackwardRightPressButtonMessage()
{
    if(!isBaseButtonsDown()
    && !ui->forwardleftButton->isDown()
    && !ui->backwardleftButton->isDown()
    && !ui->forwardrightButton->isDown())
    {
        setBaseButtonsEnable(false);
        ui->forwardleftButton->setEnabled(false);
        ui->backwardleftButton->setEnabled(false);
        ui->forwardrightButton->setEnabled(false);
        emit sendMessage(cmdRightBackward);
    }
}
void RobotControlDlg::sendForwardLeftPressButtonMessage()
{
    if(!isBaseButtonsDown()
    && !ui->forwardrightButton->isDown()
    && !ui->backwardleftButton->isDown()
    && !ui->backwardrightButton->isDown())
    {
        setBaseButtonsEnable(false);
        ui->backwardleftButton->setEnabled(false);
        ui->forwardrightButton->setEnabled(false);
        ui->backwardrightButton->setEnabled(false);
        emit sendMessage(cmdLeftForward);
    }
}
void RobotControlDlg::sendBackwardLeftPressButtonMessage()
{
    if(!isBaseButtonsDown()
    && !ui->forwardleftButton->isDown()
    && !ui->forwardrightButton->isDown()
    && !ui->backwardrightButton->isDown())
    {
        setBaseButtonsEnable(false);
        ui->forwardleftButton->setEnabled(false);
        ui->forwardrightButton->setEnabled(false);
        ui->backwardrightButton->setEnabled(false);
        emit sendMessage(cmdLeftBackward);
    }
}

void RobotControlDlg::sendForwardReleasedButtonMessage()
{
    if(!ui->backwardButton->isDown())
    {
        char msg;
        if(ui->leftButton->isDown())
            msg = cmdLeftRotate;//разворот влево
        else if(ui->rightButton->isDown())
            msg = cmdRightRotate;//разворот вправо
        else
        {
            msg = cmdStop;//стоп
            setAlterButtonsEnable(true);
            setBaseButtonsEnable(true);
        }
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendBackwardReleasedButtonMessage()
{
    if(!ui->forwardButton->isDown())
    {
        char msg;
        if(ui->leftButton->isDown())
            msg = cmdLeftRotate;//разворот влево
        else if(ui->rightButton->isDown())
            msg = cmdRightRotate;//разворот вправо
        else
        {
            msg = cmdStop;//стоп
            setAlterButtonsEnable(true);
            setBaseButtonsEnable(true);
        }
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendLeftReleasedButtonMessage()
{
    if(!ui->rightButton->isDown())
    {
        char msg;
        if(!ui->backwardButton->isDown()
        && !ui->forwardButton->isDown())
        {
            msg = cmdStop;//стоп
            setAlterButtonsEnable(true);
            setBaseButtonsEnable(true);
        }
        else if(ui->backwardButton->isDown()
             && !ui->forwardButton->isDown())
                msg = cmdBackward;//назад
        else /*if(!ui->backwardButton->isDown()
             && ui->forwardButton->isDown())*/
                msg = cmdForward;//вперед
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendRightReleasedButtonMessage()
{
    if(!ui->leftButton->isDown())
    {
        char msg;
        if(!ui->backwardButton->isDown()
        && !ui->forwardButton->isDown())
        {
            msg = cmdStop;//стоп
            setAlterButtonsEnable(true);
            setBaseButtonsEnable(true);
        }
        else if(ui->backwardButton->isDown()
             && !ui->forwardButton->isDown())
                msg = cmdBackward;//назад
        else /*if(ui->backwardButton->isDown()
             && !ui->forwardButton->isDown())*/
                msg = cmdForward;//вперед
        emit sendMessage(msg);
    }
}

void RobotControlDlg::sendForwardRightReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    setAlterButtonsEnable(true);
}

void RobotControlDlg::sendBackwardRightReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    setAlterButtonsEnable(true);
}

void RobotControlDlg::sendForwardLeftReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    setAlterButtonsEnable(true);
}

void RobotControlDlg::sendBackwardLeftReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    setAlterButtonsEnable(true);
}

void RobotControlDlg::sendSpeedMessage(int val)
{
    switch(val)
    {
        case 0:
            emit sendMessage(cmdSpeed0);
        break;
        case 1:
            emit sendMessage(cmdSpeed1);
        break;
        case 2:
            emit sendMessage(cmdSpeed2);
        break;
        case 3:
            emit sendMessage(cmdSpeed3);
        break;
        case 4:
            emit sendMessage(cmdSpeed4);
        break;
        case 5:
            emit sendMessage(cmdSpeed5);
        break;
        case 6:
            emit sendMessage(cmdSpeed6);
        break;
        case 7:
            emit sendMessage(cmdSpeed7);
        break;
        case 8:
            emit sendMessage(cmdSpeed8);
        break;
        case 9:
            emit sendMessage(cmdSpeed9);
        break;
        case 10:
            emit sendMessage(cmdSpeed10);
        break;
    }
}

void RobotControlDlg::keyPressEvent(QKeyEvent *pe)
{
    bool AutoRepeat = pe->isAutoRepeat();
    switch(pe->key())
    {
        case Qt::Key_Up:
        if(!isAlterButtonsDown() && !forwardPressed && !backwardPressed &&  !AutoRepeat)
        {
            forwardPressed = true;
            ui->forwardButton->setDown(true);
            ui->forwardButton->pressed();
        }
        break;
        case Qt::Key_Down:
        if(!isAlterButtonsDown() && !backwardPressed && !forwardPressed &&  !AutoRepeat)
        {
            backwardPressed = true;
            ui->backwardButton->setDown(true);
            ui->backwardButton->pressed();
        }
        break;
        case Qt::Key_Left:
        if(!isAlterButtonsDown() && !leftPressed &&  !AutoRepeat)
        {
            leftPressed = true;
            ui->leftButton->setDown(true);
            ui->leftButton->pressed();
        }
        break;
        case Qt::Key_Right:
        if(!isAlterButtonsDown() && !rightPressed &&  !AutoRepeat)
        {
            rightPressed = true;
            ui->rightButton->setDown(true);
            ui->rightButton->pressed();
        }
        break;
        case Qt::Key_PageUp:
        if(!isBaseButtonsDown() && isAlterButtonsNoPressed() &&  !AutoRepeat)
        {
            pageupPressed = true;
            ui->forwardrightButton->setDown(true);
            ui->forwardrightButton->pressed();
        }
        break;
        case Qt::Key_PageDown:
        if(!isBaseButtonsDown() && isAlterButtonsNoPressed() &&  !AutoRepeat)
        {
            pagednPressed = true;
            ui->backwardrightButton->setDown(true);
            ui->backwardrightButton->pressed();
        }
        break;
        case Qt::Key_Home:
        if(!isBaseButtonsDown() && isAlterButtonsNoPressed() &&  !AutoRepeat)
        {
            homePressed = true;
            ui->forwardleftButton->setDown(true);
            ui->forwardleftButton->pressed();
        }
        break;
        case Qt::Key_End:
        if(!isBaseButtonsDown() && isAlterButtonsNoPressed() &&  !AutoRepeat)
        {
            endPressed = true;
            ui->backwardleftButton->setDown(true);
            ui->backwardleftButton->pressed();
        }
        break;
        case Qt::Key_Plus:
            ui->speedControl->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        break;
        case Qt::Key_Minus:
            ui->speedControl->triggerAction(QAbstractSlider::SliderSingleStepSub);
        break;
        default: QDialog::keyPressEvent(pe);
    }
}

void RobotControlDlg::keyReleaseEvent(QKeyEvent *pe)
{
    bool AutoRepeat = pe->isAutoRepeat();
    switch(pe->key())
    {
        case Qt::Key_Up:
            if(!isAlterButtonsDown() && forwardPressed &&  !AutoRepeat)
            {
                forwardPressed = false;
                ui->forwardButton->setDown(false);
                ui->forwardButton->released();
            }
        break;
        case Qt::Key_Down:
            if(!isAlterButtonsDown() && backwardPressed &&  !AutoRepeat)
            {
                backwardPressed = false;
                ui->backwardButton->setDown(false);
                ui->backwardButton->released();
            }
        break;
        case Qt::Key_Left:
            if(!isAlterButtonsDown() && leftPressed &&  !AutoRepeat)
            {
                leftPressed = false;
                ui->leftButton->setDown(false);
                ui->leftButton->released();
            }
        break;
        case Qt::Key_Right:
            if(!isAlterButtonsDown() && rightPressed &&  !AutoRepeat)
            {
                rightPressed = false;
                ui->rightButton->setDown(false);
                ui->rightButton->released();
            }
        break;
        case Qt::Key_PageUp:
            if(!isBaseButtonsDown() && pageupPressed &&  !AutoRepeat)
            {
                pageupPressed = false;
                ui->forwardrightButton->setDown(false);
                ui->forwardrightButton->released();
            }
            break;
        case Qt::Key_PageDown:
            if(!isBaseButtonsDown() && pagednPressed &&  !AutoRepeat)
            {
                pagednPressed = false;
                ui->backwardrightButton->setDown(false);
                ui->backwardrightButton->released();
            }
            break;
        case Qt::Key_Home:
            if(!isBaseButtonsDown() && homePressed &&  !AutoRepeat)
            {
                homePressed = false;
                ui->forwardleftButton->setDown(false);
                ui->forwardleftButton->released();
            }
            break;
        case Qt::Key_End:
            if(!isBaseButtonsDown() && endPressed &&  !AutoRepeat)
            {
                endPressed = false;
                ui->backwardleftButton->setDown(false);
                ui->backwardleftButton->released();
            }
            break;
        default: QDialog::keyReleaseEvent(pe);
    }
}
