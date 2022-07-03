#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>
#include <QPaintEvent>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QNetworkInterface>
#include <QInputDialog>
#include <QPoint>
#include <QTimer>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QVector>
#include <QDateTime>
#include <cmath>
#include <QMediaPlayer>

#include "newserverdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionCreate_Server_triggered();    //创建服务器
    void initiateserver(int portnum);           //初始化服务器
    void acceptconnection();                    //收到连接
    void cancelconnection();                    //取消连接
    void on_actionConnect_triggered();          //连接到服务器
    void recvMessage();                         //接收数据
    void sendMessage();                         //发送数据
    void disconnected();                        //断开连接
    void on_actionNew_Game_triggered();         //新开始游戏
    void timeup();                              //倒计时归零，分出胜负
    void on_move_clicked();                     //走子
    void timerupdate();                         //更新倒计时
    void on_actionSurrender_triggered();        //投降
    void on_actionOpen_File_triggered();        //导入残局文件
    void on_actionQuit_triggered();             //退出游戏
    void on_actionSave_File_triggered();        //保存残局文件

signals:
    void completeconnection();

private:
    Ui::MainWindow *ui;
    int selected;   //表示选中的棋子
    int dest;       //表示棋子移到的位置
    int winner;     //表示胜者
    int type;       //表示服务器端1还是客户端-1
    int turn;       //表示轮到谁出
    bool connected; //表示连接完成
    bool started;   //表示游戏已开始
    bool saved;     //表示已保存
    int requestforsaving;       //表示提出保存请求
    int checkmatestate;         //表示当前棋局是否可将军
    int port;                   //存储端口号
    QString IpAddress;          //存储IP
    QTcpServer *listenSocket;   //监听套接字
    QTcpSocket *readWriteSocket;//读写套接字
    QTimer *timer;              //倒计时
    QTimer *sectimer;           //每秒倒计时
    void paintEvent(QPaintEvent* e);    //绘制事件
    void closeEvent(QCloseEvent *event);//关闭窗口的事件
    void click(const QPoint &p);        //鼠标点击事件中调用的处理坐标的函数
    void mousePressEvent(QMouseEvent*); //鼠标点击事件
    bool isIpAddr(const QString &ip);   //判断IP地址合法性
    bool canmove(int from, int to, int movetype);//判断是否可以移动
    void settimer();    //重置倒计时
    bool checkmate();   //判断当前棋局是否可将军
    int board[90];      //存储棋盘与棋子信息
};

#endif // MAINWINDOW_H
