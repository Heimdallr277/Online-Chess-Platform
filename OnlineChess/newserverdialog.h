#ifndef NEWSERVERDIALOG_H
#define NEWSERVERDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QValidator>

namespace Ui {
class NewServerDialog;
}

class NewServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewServerDialog(QWidget *parent = 0);
    ~NewServerDialog();
    QString getHostIpAddress();
    int getPort();

signals:
    void connecting(int portnum);
    void cancelconnecting();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void connected();

private:
    Ui::NewServerDialog *ui;
    bool isconnecting;
    int port;
};

#endif // NEWSERVERDIALOG_H
