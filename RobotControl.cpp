#include "declaration.h"

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
{
    ui->setupUi(this);
    Title = windowTitle();

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
        connect(client, SIGNAL(connected(QString)), this, SLOT(clientReady(QString)));
        connect(client, SIGNAL(failConnected()), this, SLOT(clientFail()));
        connect(client, SIGNAL(disconnected()), this, SLOT(disconnect()));

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
}

void RobotControlDlg::clientFail()
{
    QString title = QString("%1 ошибка соединения с %2").arg(Title).arg(deviceName);
    setWindowTitle(title);
    disconnect();
}

void RobotControlDlg::disconnect()
{
//    QObject::disconnect(this,SIGNAL(sendMessage),client,SLOT(sendMessage(char)));
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
        emit sendMessage(cmdRightBackward);
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
    ui->forwardleftButton->setEnabled(true);
    ui->backwardleftButton->setEnabled(true);
    ui->backwardrightButton->setEnabled(true);
}

void RobotControlDlg::sendBackwardRightReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    ui->forwardleftButton->setEnabled(true);
    ui->backwardleftButton->setEnabled(true);
    ui->forwardrightButton->setEnabled(true);
}

void RobotControlDlg::sendForwardLeftReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    ui->backwardleftButton->setEnabled(true);
    ui->forwardrightButton->setEnabled(true);
    ui->backwardrightButton->setEnabled(true);
}

void RobotControlDlg::sendBackwardLeftReleasedButtonMessage()
{
    emit sendMessage(cmdStop);
    setBaseButtonsEnable(true);
    ui->forwardleftButton->setEnabled(true);
    ui->forwardrightButton->setEnabled(true);
    ui->backwardrightButton->setEnabled(true);
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
        if(!isBaseButtonsDown() && !rightPressed &&  !AutoRepeat)
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
                ui->forwardrightButton->pressed();
            }
            break;
        case Qt::Key_PageDown:
            if(!isBaseButtonsDown() && pagednPressed &&  !AutoRepeat)
            {
                pagednPressed = false;
                ui->backwardrightButton->setDown(false);
                ui->backwardrightButton->pressed();
            }
            break;
        case Qt::Key_Home:
            if(!isBaseButtonsDown() && homePressed &&  !AutoRepeat)
            {
                homePressed = false;
                ui->forwardleftButton->setDown(false);
                ui->forwardleftButton->pressed();
            }
            break;
        case Qt::Key_End:
            if(!isBaseButtonsDown() && endPressed &&  !AutoRepeat)
            {
                endPressed = false;
                ui->backwardleftButton->setDown(false);
                ui->backwardleftButton->pressed();
            }
            break;
        default: QDialog::keyReleaseEvent(pe);
    }
}
