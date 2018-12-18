#ifndef RobotControl_H
#define RobotControl_H

#include "declaration.h"

#include <ui_RobotControl.h>
#include <qbluetoothlocaldevice.h>

#include <QDialog>

#include <qlistwidget.h>

class QKeyEvent;
QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothServiceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceInfo)

QT_USE_NAMESPACE

QT_FORWARD_DECLARE_CLASS(BtClient)

//namespace Ui {
//class RobotControlDlg;
//}

class RobotControlDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RobotControlDlg(QWidget *parent = nullptr);
    ~RobotControlDlg() override;
signals:
    void sendMessage(const char&);
public slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void on_power_clicked(bool clicked);
    void on_discoverable_clicked(bool clicked);
    void displayPairingMenu(const QPoint &pos);
    void pairingDone(const QBluetoothAddress&, QBluetoothLocalDevice::Pairing);
    void sendForwardPressButtonMessage();
    void sendBackwardPressButtonMessage();
    void sendLeftPressButtonMessage();
    void sendRightPressButtonMessage();
    void sendForwardRightPressButtonMessage();
    void sendBackwardRightPressButtonMessage();
    void sendForwardLeftPressButtonMessage();
    void sendBackwardLeftPressButtonMessage();
    void sendForwardReleasedButtonMessage();
    void sendBackwardReleasedButtonMessage();
    void sendLeftReleasedButtonMessage();
    void sendRightReleasedButtonMessage();
    void sendForwardRightReleasedButtonMessage();
    void sendBackwardRightReleasedButtonMessage();
    void sendForwardLeftReleasedButtonMessage();
    void sendBackwardLeftReleasedButtonMessage();
    void sendSpeedMessage(int val);
    void clientReady(const QString peerName);
    void clientFail();
    void disconnect(bool state);
    void logWrite(QString text);
private slots:
    void startScan();
    void scanFinished();
    void setGeneralUnlimited(bool unlimited);
    void itemActivated(QListWidgetItem *item);
    void hostModeStateChanged(QBluetoothLocalDevice::HostMode);


private:
    QString Title;
    QString deviceName;
    QString deviceAdress;
    Ui_RobotControl *ui;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothLocalDevice *localDevice;
    QBluetoothServiceDiscoveryAgent *discoverySeviceAgent;
    BtClient *client;
    bool forwardPressed;
    bool backwardPressed;
    bool leftPressed;
    bool rightPressed;
    bool pageupPressed;
    bool pagednPressed;
    bool homePressed;
    bool endPressed;
    inline void setBaseButtonsEnable(bool state)
    {
        ui->forwardButton->setEnabled(state);
        ui->rightButton->setEnabled(state);
        ui->leftButton->setEnabled(state);
        ui->backwardButton->setEnabled(state);
    }
    inline bool isBaseButtonsEnable()
    {
        return (ui->forwardButton->isEnabled()
                && ui->rightButton->isEnabled()
                && ui->leftButton->isEnabled()
                && ui->backwardButton->isEnabled());
    }
    inline bool isBaseButtonsDown()
    {
        return (ui->forwardButton->isDown()
                || ui->rightButton->isDown()
                || ui->leftButton->isDown()
                || ui->backwardButton->isDown());
    }
    inline void setAlterButtonsEnable(bool state)
    {
        ui->forwardleftButton->setEnabled(state);
        ui->forwardrightButton->setEnabled(state);
        ui->backwardrightButton->setEnabled(state);
        ui->backwardleftButton->setEnabled(state);
    }
    inline bool isAlterButtonsDown()
    {
        return (ui->forwardrightButton->isDown()
                || ui->backwardrightButton->isDown()
                || ui->forwardleftButton->isDown()
                || ui->backwardleftButton->isDown());
    }
    inline bool isAlterButtonsNoPressed()
    {
        return (!pageupPressed
                && !pagednPressed
                && !homePressed
                && !endPressed);
    }
protected:
    /*virtual*/ void keyPressEvent(QKeyEvent *pe) override;
    /*virtual*/ void keyReleaseEvent(QKeyEvent *pe) override;
};

#endif // RobotControl_H
