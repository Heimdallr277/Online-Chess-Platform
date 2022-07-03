#include "newserverdialog.h"
#include "ui_newserverdialog.h"

NewServerDialog::NewServerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewServerDialog),
    isconnecting(false), port(0)
{
    ui->setupUi(this);
    ui->lineEdit->setText(getHostIpAddress());
    setWindowTitle(tr("Creating the connection"));
    QRegExp regx("[0-9]+$");
    QValidator* validator = new QRegExpValidator(regx, ui->port);
    ui->port->setValidator(validator);
}

NewServerDialog::~NewServerDialog()
{
    delete ui;
}

QString NewServerDialog::getHostIpAddress() {
    QString strIpAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // 获取第一个本主机的IPv4地址
    int nListSize = ipAddressesList.size();
    for (int i = 0; i < nListSize; ++i)
    {
           if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
               ipAddressesList.at(i).toIPv4Address()) {
               strIpAddress = ipAddressesList.at(i).toString();
               break;
           }
     }
     // 如果没有找到，则以本地IP地址为IP
     if (strIpAddress.isEmpty())
        strIpAddress = QHostAddress(QHostAddress::LocalHost).toString();
     return strIpAddress;
}


void NewServerDialog::on_pushButton_clicked()
{
    int tmp = ui->port->text().toInt();
    if (tmp<=1024||tmp>=65535) {
        QMessageBox::information(this, "Error!", "无效的端口号！");
        ui->port->clear();
        return;
    }
    if (isconnecting) {
        QMessageBox::information(this, "Error!", "正在连接！");
    } else {
        ui->status->setText(tr("状态: 连接中..."));
        isconnecting = true;
        port = tmp;
        emit connecting(tmp);
    }
}

void NewServerDialog::on_pushButton_2_clicked()
{
    if (isconnecting) {
        ui->status->setText(tr("状态: 未连接"));
        isconnecting = false;
        port = 0;
        emit cancelconnecting();
    } else {
        reject();
    }
}

void NewServerDialog::connected() {
    QMessageBox::information(this, "Connected!", "连接成功！");
    ui->status->setText(tr("状态: 已连接"));
    accept();
}

int NewServerDialog::getPort() {
    return port;
}
