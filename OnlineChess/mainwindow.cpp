#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    selected(-1), dest(-1),
    winner(0), type(0), turn(0),
    connected(0), started(0), saved(0),
    requestforsaving(0), checkmatestate(0),
    port(0)
{
    ui->setupUi(this);
    setWindowTitle("OnlineChess - Offline");
    ui->lcdNumber->setDecMode();

    timer = new QTimer;
    sectimer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(timeup()));
    connect(sectimer, SIGNAL(timeout()), this, SLOT(timerupdate()));
    connect(timer, SIGNAL(timeout()), sectimer, SLOT(stop()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.translate(100,100);
//    QImage bg(":/pics/BG");
//    p.drawPixmap(0,0,QPixmap::fromImage(bg));
    p.setPen(QPen(Qt::black, 2));
    p.drawRect(-2, -2, 405, 455);
    p.setPen(QPen(Qt::black, 1));
    p.drawRect(0, 0, 400, 450);
    for (int i = 1; i <= 8; i++) {
        p.drawLine(0, 50*i, 400, 50*i);
    }
    for (int i = 1; i <= 7; i++) {
        p.drawLine(50*i, 0, 50*i, 200);
        p.drawLine(50*i, 250, 50*i, 450);
    }
    p.drawLine(150, 0, 250, 100);
    p.drawLine(150, 100, 250, 0);
    p.drawLine(150, 350, 250, 450);
    p.drawLine(150, 450, 250, 350);

    if (started) {
        for (int i = 0; i < 90; i++) {
            int id = board[i];
            int x = i%9;
            int y;
            if (type==1) {
                y = 9-i/9;
            } else if (type==-1){
                y = i/9;
            }
            if (id!=0) {
                QString path = ":/pics/" + QString::number(id);
                if (selected==i||dest==i) {
                    path = path + "s";
                }
                QImage *m = new QImage(path);
                p.drawPixmap(x*50-25, y*50-25, 50, 50, QPixmap::fromImage(*m));
            } else {
                if (dest==i) {
                    QImage *n = new QImage(":/pics/0s");
                    p.drawPixmap(x*50-25, y*50-25, 50, 50, QPixmap::fromImage(*n));
                }
            }
        }

    }
    if (canmove(selected, dest, type)&&turn==type) {
        ui->move->setEnabled(true);
    } else {
        ui->move->setEnabled(false);
    }
}


void MainWindow::on_actionCreate_Server_triggered()
{
    if (connected) {
        QMessageBox::information(this, "Error!", "???????????????????????????");
    } else {
        NewServerDialog *dialog = new NewServerDialog(this);
        connect(dialog, SIGNAL(connecting(int)), this, SLOT(initiateserver(int)));
        connect(dialog, SIGNAL(cancelconnecting()), this, SLOT(cancelconnection()));
        connect(this, SIGNAL(completeconnection()), dialog, SLOT(connected()));
        IpAddress = dialog->getHostIpAddress();
        dialog->exec();
    }

}

void MainWindow::initiateserver(int portnum) {
    listenSocket = new QTcpServer();
    port = portnum;
    if (listenSocket->listen(QHostAddress::Any, portnum)) {
        connect(listenSocket, SIGNAL(newConnection()), this, SLOT(acceptconnection()));
    }
}

void MainWindow::acceptconnection() {
    readWriteSocket = new QTcpSocket();
    readWriteSocket = listenSocket->nextPendingConnection();
    if (readWriteSocket) {
        connected = true;
        type = 1;
        setWindowTitle("OnlineChess - Connected / Server");
        connect(readWriteSocket, SIGNAL(readyRead()), this, SLOT(recvMessage()));
        connect(readWriteSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
        emit completeconnection();
    }
}

void MainWindow::cancelconnection() {
    if (listenSocket->isListening()) {
        listenSocket->close();
    }
}

void MainWindow::disconnected() {
    connected = false;
    started = false;
    winner = 0;
    selected = -1;
    dest = -1;
    requestforsaving = 0;
    checkmatestate = 0;
    saved = 0;
    type = 0;
    turn = 0;
    timer->stop();
    sectimer->stop();
    ui->turn->setText(tr("???????????????"));
    ui->lcdNumber->display(0);
    setWindowTitle("OnlineChess - Offline");
    QMessageBox::information(this, "Disconnected!", "???????????????");
    return;
}

bool MainWindow::isIpAddr(const QString &ip)
{
    QRegExp rx2("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
    if(rx2.exactMatch(ip))
    {
        return true;
    }
    return false;
}

void MainWindow::on_actionConnect_triggered()
{
    QString input = QInputDialog::getText(this, "Connecting...", "Enter IP: ");
    if (!isIpAddr(input)) {
        QMessageBox::information(this, "Error!", "?????????IP?????????");
    } else {
        QString inputport = QInputDialog::getText(this, "Connecting...", "Enter Port No.: ");
        int tmp = inputport.toInt();
        if (tmp<=1024||tmp>=65535) {
            QMessageBox::information(this, "Error!", "?????????????????????");
            return;
        }
        readWriteSocket = new QTcpSocket();
        readWriteSocket->connectToHost(input, tmp);
        if (!readWriteSocket->waitForConnected(2000)) {
            QMessageBox::information(this, "Error!", "???????????????");
            return;
        } else {
            IpAddress = input;
            connected = true;
            type = -1;
            setWindowTitle("OnlineChess - Connected / Client");
            QMessageBox::information(this, "Connected!", "???????????????");
            connect(readWriteSocket, SIGNAL(readyRead()), this, SLOT(recvMessage()));
            connect(readWriteSocket, SIGNAL(disconnected()), this, SLOT(disconnected()));
        }
    }
}

void MainWindow::recvMessage() {
    QByteArray info = readWriteSocket->readAll();
    QTextStream in(&info);
    for (int i = 0; i < 90; i++) {
        in >> board[i];
    }
    in >> turn;
    in >> winner;
    in >> requestforsaving;
    in >> checkmatestate;
    update();
    if (!started) {
        started = true;
    }
    if (checkmatestate) {
        QMediaPlayer *p = new QMediaPlayer;
        p->setMedia(QUrl("qrc:/audio/checkmate"));
        p->setVolume(100);
        p->play();
    }
    if (winner!=0) {
        started = false;
        if (sectimer->isActive()) {
            sectimer->stop();
        }
        if (timer->isActive()) {
            timer->stop();
            ui->turn->setText(tr("???????????????"));
            ui->lcdNumber->display(0);
            if (winner==type) {
                QMessageBox::information(this, "???????????????", "????????????");
            } else {
                QMessageBox::information(this, "???????????????", "????????????");
            }
        }
    } else {
        if (requestforsaving) {
            on_actionSave_File_triggered();
        } else {
            settimer();
        }
    }
    update();
}

void MainWindow::sendMessage() {
    QString info="";
    QTextStream out(&info);
    for (int i = 0; i < 90; i++) {
        out << board[i] << " ";
    }
    out << turn << " ";
    out << winner << " ";
    out << requestforsaving << " ";
    out << checkmatestate;
    QByteArray *array =new QByteArray;
    array->clear();
    array->append(info);
    this->readWriteSocket->write(array->data());
}


void MainWindow::on_actionNew_Game_triggered() {
    if (!connected) {
        QMessageBox::information(this, "Error!", tr("????????????????????????"));
        return;
    }
    if (started) {
        QMessageBox::information(this, "Error!", tr("??????????????????"));
        return;
    }
    winner = 0;
    int m[90] = {5,4,3,2,1,2,3,4,5,
                 0,0,0,0,0,0,0,0,0,
                 0,6,0,0,0,0,0,6,0,
                 7,0,7,0,7,0,7,0,7,
                 0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,
                 -7,0,-7,0,-7,0,-7,0,-7,
                 0,-6,0,0,0,0,0,-6,0,
                 0,0,0,0,0,0,0,0,0,
                 -5,-4,-3,-2,-1,-2,-3,-4,-5};
    for (int i = 0; i < 90; i++) {
        board[i] = m[i];
    }
    started = true;
    saved = 0;
    checkmatestate = 0;
    turn = type;
    settimer();
    sendMessage();
    update();
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    click(event->pos());
}

void MainWindow::click(const QPoint &p) {
    if (turn != type) {
        return;
    }
    int x = p.x()-75;
    int y = p.y()-75;
    if (x<0||x>450||y<0||y>500) {
        return;
    }
    int a = x/50;
    int b = y/50;
    int chosen;
    if (type == 1) {
       chosen = 9*(9-b)+a;
    } else if (type==-1){
       chosen = 9*b+a;
    }
//    qDebug()<<chosen;

    if (selected==-1) { //???????????????????????????????????????
        if (board[chosen]*turn>0) {
            selected = chosen;
            update();
        }
    } else {    //???????????????????????????
        if (chosen==selected) { //???????????????????????????????????????
            selected = -1;
            dest = -1;
        } else {
            dest = chosen; //????????????????????????????????????????????????
        }
        update();
    }




}

void MainWindow::closeEvent(QCloseEvent *event) {
    bool exit;
    if (started&&!saved) {
        exit = QMessageBox::question(this,
                                      tr("????????????"),
                                      tr("??????????????????????????????????????????"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No) == QMessageBox::Yes;
    } else {
        exit = QMessageBox::question(this,
                                      tr("????????????"),
                                      tr("???????????????"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No) == QMessageBox::Yes;
    }

    if (exit) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::canmove(int from, int to, int movetype) {
    if (!connected||!started||from==-1||to==-1||board[to]*movetype>0) {
        return false;
    }
    int fx = from%9;
    int fy = from/9;
    int tx = to%9;
    int ty = to/9;

    if (board[from]==1) {
        if (((fx==tx&&abs(fy-ty)==1)||(fy==ty&&abs(fx-tx)==1))&&tx>=3&&tx<=5&&ty>=0&&ty<=2) {
            return true;
        } else {
            return false;
        }
    }

    if (board[from]==-1) {
        if (((fx==tx&&abs(fy-ty)==1)||(fy==ty&&abs(fx-tx)==1))&&tx>=3&&tx<=5&&ty>=7&&ty<=9) {
            return true;
        } else {
            return false;
        }
    }

    if (board[from]==2) {
        if (from==3||from==5||from==21||from==23) {
            if (to==13)
                return true;
            else
                return false;
        }
        else if (from==13) {
            if (to==3||to==5||to==21||to==23)
                return true;
            else
                return false;
        }
    }

    if (board[from]==-2) {
        if (from==66||from==68||from==84||from==86) {
            if (to==76)
                return true;
            else
                return false;
        }
        else if (from==76) {
            if (to==66||to==68||to==84||to==86)
                return true;
            else
                return false;
        }
    }

    if (board[from]==3) {
        if (abs(fx-tx)==2&&abs(fy-ty)==2&&board[(fx+tx)/2+(fy+ty)/2*9]==0&&ty<=4) {
            return true;
        } else
            return false;
    }

    if (board[from]==-3) {
        if (abs(fx-tx)==2&&abs(fy-ty)==2&&board[(fx+tx)/2+(fy+ty)/2*9]==0&&ty>=5) {
            return true;
        } else
            return false;
    }

    if (board[from]==4||board[from]==-4) {
        if (tx+2==fx&&ty-1==fy&&board[fx-1+fy*9]==0) {
            return true;
        }
        else if (tx+1==fx&&ty-2==fy&&board[fx+(fy+1)*9]==0) {
            return true;
        }
        else if (tx-1==fx&&ty-2==fy&&board[fx+(fy+1)*9]==0) {
            return true;
        }
        else if (tx-2==fx&&ty-1==fy&&board[fx+1+fy*9]==0) {
            return true;
        }
        else if (tx-2==fx&&ty+1==fy&&board[fx+1+fy*9]==0) {
            return true;
        }
        else if (tx-1==fx&&ty+2==fy&&board[fx+(fy-1)*9]==0) {
            return true;
        }
        else if (tx+1==fx&&ty+2==fy&&board[fx+(fy-1)*9]==0) {
            return true;
        }
        else if (tx+2==fx&&ty+1==fy&&board[fx-1+fy*9]==0) {
            return true;
        }
        else
            return false;
    }

    if (board[from]==5||board[from]==-5) {
        int min, max;
        if (tx==fx) {
            if (ty<fy) {min=ty;max=fy;} else {min=fy;max=ty;}
            for (int i = min+1; i < max; i++) {
                if (board[fx+i*9]!=0)
                    return false;
            }
            return true;
        }
        if (ty==fy) {
            if (tx<fx) {min=tx;max=fx;} else {min=fx;max=tx;}
            for (int i = min+1; i < max; i++) {
                if (board[i+fy*9]!=0)
                    return false;
            }
            return true;
        }
    }

    if (board[from]==6||board[from]==-6) {
        if (board[to]==0) {
            int min, max;
            if (tx==fx) {
                if (ty<fy) {min=ty;max=fy;} else {min=fy;max=ty;}
                for (int i = min+1; i < max; i++) {
                    if (board[fx+i*9]!=0)
                        return false;
                }
                return true;
            } else if (ty==fy) {
                if (tx<fx) {min=tx;max=fx;} else {min=fx;max=tx;}
                for (int i = min+1; i < max; i++) {
                    if (board[i+fy*9]!=0)
                        return false;
                }
            } else
                return false;
        } else if (board[to]!=0) {
            int min, max;
            int sum = 0;
            if (tx==fx) {
                if (ty<fy) {min=ty;max=fy;} else {min=fy;max=ty;}
                for (int i = min+1; i < max; i++) {
                    if (board[fx+i*9]!=0)
                        sum++;
                }
                if (sum==1)
                    return true;
                else
                    return false;
            } else if (ty==fy) {
                if (tx<fx) {min=tx;max=fx;} else {min=fx;max=tx;}
                for (int i = min+1; i < max; i++) {
                    if (board[i+fy*9]!=0)
                        sum++;
                }
                if (sum==1)
                    return true;
                else
                    return false;
            } else
                return false;
        }
    }

    if (board[from]==7) {
        if (fy<=4) {
            if (tx==fx&&ty==fy+1) {
                return true;
            } else
                return false;
        }
        else if (fy>=5) {
            if ((abs(tx-fx)==1&&fy==ty)||(ty-fy==1&&tx==fx)) {
                return true;
            } else
                return false;
        }
    }

    if (board[from]==-7) {
        if (fy>=5) {
            if (tx==fx&&ty==fy-1) {
                return true;
            } else
                return false;
        }
        else if (fy<=4) {
            if ((abs(tx-fx)==1&&fy==ty)||(fy-ty==1&&tx==fx)) {
                return true;
            } else
                return false;
        }
    }
}

void MainWindow::on_move_clicked()
{
    if (!connected||!started) {
        QMessageBox::information(this, "Error!", tr("???????????????????????????????????????"));
        return;
    }
    if (type!=turn) {
        QMessageBox::information(this, "Error!", tr("??????????????????????????????"));
        return;
    }

    if (board[selected]==type) {
        int a = dest;
        int b;

        for (int i = 0; i < 90; i++) {
            if (board[i]==-type) {
                b = i;
                break;
            }
        }
        if (a%9==b%9) {
            bool lose = true;
            int y1 = a/9;
            int y2 = b/9;
            int min, max;
            if (y1<y2){min=y1;max=y2;} else {min=y2;max=y1;}
            for (int i = min+1; i < max; i++) {
                if (board[a%9+i*9]!=0) {
                    lose = false;
                    break;
                }
            }
            if (lose) {
                board[dest] = board[selected];
                board[selected] = 0;
                update();
                timeup();
                return;
            }
        }
    }

    if (board[dest]==-type) {
        board[dest] = board[selected];
        board[selected] = 0;
        int t = -turn;
        turn = t;
        timeup();
        return;
        //?????????
    }

    board[dest] = board[selected];
    board[selected] = 0;
    int t = -turn;
    turn = t;
    selected = -1;
    dest = -1;
    bool flag = checkmate();
    if (flag) {
        checkmatestate = 1;
        QMediaPlayer *p = new QMediaPlayer;
        p->setMedia(QUrl("qrc:/audio/checkmate"));
        p->setVolume(100);
        p->play();
    } else {
        checkmatestate = 0;
    }
    sendMessage();
    settimer();
    update();
}

bool MainWindow::checkmate() {
    int redpos = -1;
    bool findred = false;
    int blackpos = -1;
    bool findblack = false;
    QVector<int> red;
    red.clear();
    QVector<int> black;
    black.clear();
    //?????????????????????????????????
    for (int i = 0; i < 90; i++) {
        if (board[i]==1) {
            redpos = i;
            findred = true;
            continue;
        }
        if (board[i]>1) {
            red.push_back(i);
            continue;
        }
        if (board[i]==-1) {
            blackpos = i;
            findblack = true;
            continue;
        }
        if (board[i]<-1) {
            black.push_back(i);
            continue;
        }
    }
    if (!findred||!findblack) {
        return false;
    }

    bool flag = false;

    //???????????????????????????
    for (auto it = red.begin(); it!=red.end(); it++) {
        int id = *it;
        if (canmove(id, blackpos, 1)){
           flag = true;
        }
    }

    for (auto it = black.begin(); it!=black.end(); it++) {
        int id = *it;
        if (canmove(id, redpos, -1)){
            flag = true;
        }
    }

    if (flag) {
        return true;
    } else {
        return false;
    }
}

void MainWindow::settimer() {
    if (turn == type) {
        ui->turn->setText(tr("??????????????? ???"));
    } else {
        ui->turn->setText(tr("??????????????? ??????"));
    }
    if (timer->isActive()) {
        timer->stop();
    }
    if (sectimer->isActive()) {
        sectimer->stop();
    }

    timer->setSingleShot(true);
    timer->start(20*1000);
    ui->lcdNumber->display(20);

    sectimer->start(1000);
}

void MainWindow::timeup() {
    sectimer->stop();
    timer->stop();
    winner = -turn;
    started = false;
    sendMessage();
    if (winner==type) {
        QMessageBox::information(this, "???????????????", "????????????");
    } else {
        QMessageBox::information(this, "???????????????", "????????????");
    }
    ui->turn->setText(tr("???????????????"));
    ui->lcdNumber->display(0);
    update();
}

void MainWindow::timerupdate() {
    int remainingtime= ui->lcdNumber->intValue();
    remainingtime = remainingtime-1;
    if (remainingtime<0) {
        remainingtime = 0;
    }
    ui->lcdNumber->display(remainingtime);
}

void MainWindow::on_actionSurrender_triggered()
{
    if (!connected||!started) {
        QMessageBox::information(this, "Error!", tr("???????????????????????????????????????"));
        return;
    }
    if (type!=turn) {
        QMessageBox::information(this, "Error!", tr("??????????????????????????????"));
        return;
    }
    timeup();
}



void MainWindow::on_actionOpen_File_triggered()
{
    if (!connected) {
        QMessageBox::information(this, "Error!", tr("????????????????????????"));
        return;
    }
    if (started) {
        QMessageBox::information(this, "Error!", tr("??????????????????"));
        return;
    }
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("??????????????????"),
                                                ".",
                                                tr("????????????(*.txt)"));
    if(!path.isEmpty()) {//???????????? path ????????????????????????????????????????????????????????????
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //????????????????????????????????? QFile::open()??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????true
            QMessageBox::warning(this, tr("Warning!"),
                                 tr("????????????:\n%1").arg(path));
            return;
        }
        QTextStream in(&file);
        QString firsthand = in.readLine();
        for (int i = 0; i < 90; i++) {
            board[i] = 0;
        }
        if (firsthand=="red") {
            turn = 1;
        }
        if (firsthand=="black") {
            turn = -1;
        }
        for (int i = 1; i <= 7; i++) {
            QString currentline = in.readLine();
            if (currentline[0]=="0") {
                continue;
            }
            int id = 3;
            while (id+2<currentline.length()-1) {
                int x = currentline.mid(id,1).toInt();
                int y = currentline.mid(id+2,1).toInt();
                int num = y*9+x;
                board[num] = i*turn;
                id = id + 6;
            }
        }
        QString secondhand = in.readLine();
        for (int i = 1; i <= 7; i++) {
            QString currentline = in.readLine();
            if (currentline[0]=="0") {
                continue;
            }
            int id = 3;
            while (id+2<currentline.length()-1) {
                int x = currentline.mid(id,1).toInt();
                int y = currentline.mid(id+2,1).toInt();
                int num = y*9+x;
                board[num] = -i*turn;
                id = id + 6;
            }
        }
        QMessageBox::information(this, tr("Done!"), tr("???????????????????????????"));
        file.close();
        winner = 0;
        started = true;
        checkmatestate = checkmate();
        if (checkmatestate) {
            QMediaPlayer *p = new QMediaPlayer;
            p->setMedia(QUrl("qrc:/audio/checkmate"));
            p->setVolume(100);
            p->play();
        }
        saved = 0;
        settimer();
        sendMessage();
        update();
    } else {
        QMessageBox::warning(this, tr("Error!"),
                             tr("????????????????????????"));
    }
}


void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionSave_File_triggered()
{
    if (!connected||!started) {
        QMessageBox::information(this, "Error!", tr("???????????????????????????????????????"));
        return;
    }
    if (requestforsaving==0) {
        requestforsaving = 1;
        sendMessage();
    }
    if (requestforsaving==1) {
        timer->stop();
        sectimer->stop();
        QDateTime time = QDateTime::currentDateTime();
        QString str = time.toString("yyyyMMdd_hhmmss");
        QString t = "_Server.txt";
        if (type==-1) {
            t = "_Client.txt";
        }
        QString path = str + t;
        if(!path.isEmpty()) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("Warning!"),
                                           tr("????????????:\n%1").arg(path));
                return;
            }
            QTextStream out(&file);
            QString firsthand="black";
            QString secondhand="red";
            int flag = -1;
            if (turn==1) {
                firsthand="red";
                secondhand="black";
                flag = 1;
            }
            out << firsthand << "\n";
            for (int i = 1; i <= 7; i++) {
                QVector<int> l;
                l.clear();
                for (int j = 0; j < 90; j++) {
                    if(board[j]==i*flag) {
                        l.push_back(j);
                    }
                }
                out << l.length() << " ";
                for (auto it = l.begin(); it!=l.end(); it++) {
                    int a = (*it)%9;
                    int b = (*it)/9;
                    out <<"<"<<a<<","<<b<<">"<<" ";
                }
                out << "\n";
            }
            out << secondhand << "\n";
            for (int i = 1; i <= 7; i++) {
                QVector<int> l;
                l.clear();
                for (int j = 0; j < 90; j++) {
                    if(board[j]==-i*flag) {
                        l.push_back(j);
                    }
                }
                out << l.length() << " ";
                for (auto it = l.begin(); it!=l.end(); it++) {
                    int a = (*it)%9;
                    int b = (*it)/9;
                    out <<"<"<<a<<","<<b<<">"<<" ";
                }
                out << "\n";
            }
            saved = true;
            QMessageBox::information(this, tr("Done!"), tr("?????????????????????"));
            winner = 0;
            started = false;
            requestforsaving = 0;
            checkmatestate = 0;
            sectimer->stop();
            timer->stop();
            ui->turn->setText(tr("???????????????"));
            ui->lcdNumber->display(0);
            file.close();
        } else {
            QMessageBox::warning(this, tr("Error!"),
                                 tr("?????????????????????"));
        }
    }
}
