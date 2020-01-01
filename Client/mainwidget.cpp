#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QSound>

/**
 * @brief 主窗体
 * @param parent
 */
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    //init TCP Server
    this->tcpClient = new QTcpSocket(this);   //实例化tcpClient
    this->tcpClient->abort();                 //取消原有连接
    connect(tcpClient, SIGNAL(readyRead()), this, SLOT(ReadData()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)), \
            this, SLOT(ReadError(QAbstractSocket::SocketError)));

    ui->edit_ip->setText("192.168.137.77");
    ui->edit_port->setText("9000");

    //未连接时，功能按钮禁止使用
    ui->btn_conn->setEnabled(true);
    ui->btn_led1->setEnabled(false);
    ui->btn_led2->setEnabled(false);
    //初始化LED按钮颜色
    ui->btn_led1->setStyleSheet("QPushButton{background:white}");
    ui->btn_led2->setStyleSheet("QPushButton{background:white}");
    this->led1 = 1;
    this->led2 = 1;

    adSwitch = true; //AD读取开关，默认开启

    //init Plot
    initPlot(ui->plot);
    ui->plot->replot();
}

MainWidget::~MainWidget()
{
    delete ui;
}

/**
 * @brief QCustomPlot初始化
 * @param customPlot
 */
void MainWidget::initPlot(QCustomPlot *customPlot)
{
  customPlot->addGraph();
  customPlot->graph(0)->setPen(QPen(Qt::red));
  customPlot->graph(0)->setName("Vol");

  //横坐标
  customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
  customPlot->xAxis->setDateTimeFormat("hh:mm:ss");
  customPlot->xAxis->setAutoTickStep(false);
  customPlot->xAxis->setTickStep(2);
  customPlot->axisRect()->setupFullAxesBox();
  customPlot->legend->setVisible(true);//右上角小图标

}

/**
 * @brief 更新Plot
 */
void MainWidget::updatePlot(int value)
{
    //横轴：key 时间 单位s
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    //纵轴：value 电压模拟量

    ui->lcd_vol->setDigitCount(4);
    ui->lcd_vol->setMode(QLCDNumber::Dec);
    ui->lcd_vol->display(QString::number(value));

    //添加数据到曲线0
    ui->plot->graph(0)->addData(key, value);


    //删除8秒之前的数据。这里的8要和下面设置横坐标宽度的8配合起来
    //才能起到想要的效果，可以调整这两个值，观察显示的效果。
    ui->plot->graph(0)->removeDataBefore(key-8);

    //自动设定graph曲线y轴的范围，如果不设定，有可能看不到图像
    //也可以用ui->plot->yAxis->setRange(up,low)手动设定y轴范围
    ui->plot->yAxis->setRange(500, 7000);

    //这里的8，是指横坐标时间宽度为8秒，如果想要横坐标显示更多的时间
    //就把8调整为比较大到值，比如要显示60秒，那就改成60。
    //这时removeDataBefore(key-8)中的8也要改成60，否则曲线显示不完整。
    ui->plot->xAxis->setRange(key+0.25, 8, Qt::AlignRight);//设定x轴的范围
    ui->plot->replot();
}

void MainWidget::analysisMessage(QString msg)
{
    //解析命令
    QStringList cmd = msg.split("#");
    qDebug() << "命令个数:" << cmd.length()-1;
    for(int i = 1; i < cmd.length(); i++)
    {
        qDebug() << "CMD[" << i <<"]=" << cmd[i];
        if (cmd[i][0] == 'B')
        {
            int data = cmd[i].right(5).toInt();
            updatePlot(data);
        }
        else if(cmd[i][0] == 'L')
        {
            int data = cmd[i].right(2).toInt();
            qDebug() << "LED控制命令:" << data;

            switch(data)
            {
                case 11:
                {
                    this->led1 = 1;
                    this->led2 = 1;
                    ui->btn_led2->setStyleSheet("QPushButton{background:white}");
                    ui->btn_led1->setStyleSheet("QPushButton{background:white}");
                    break;
                }
                case 00:
                {
                    this->led1 = 0;
                    this->led2 = 0;
                    ui->btn_led2->setStyleSheet("QPushButton{background:green}");
                    ui->btn_led1->setStyleSheet("QPushButton{background:green}");
                    break;
                }
                case 01:
                {
                    this->led2 = 0;
                    this->led1 = 1;
                    ui->btn_led2->setStyleSheet("QPushButton{background:green}");
                    ui->btn_led1->setStyleSheet("QPushButton{background:white}");
                    break;
                }
                case 10:
                {
                    this->led2 = 1;
                    this->led1 = 0;
                    ui->btn_led2->setStyleSheet("QPushButton{background:white}");
                    ui->btn_led1->setStyleSheet("QPushButton{background:green}");
                    break;
                }
            }
        }
        else if (cmd[i][0] == 'R')
        {
            int data = cmd[i].right(1).toInt();
            qDebug() << "读取AD命令:" << data;

            switch(data)
            {
                case 1: this->adSwitch = true; break;
                case 0: this->adSwitch = false; break;
            }
        }
        else if (cmd[i][0] == 'W')
        {
            int data = cmd[i].right(1).toInt();
            qDebug() << "报警提示命令:" << data;

            switch(data)
            {
                case 1: QSound::play(":/media/warning.wav"); break;
            }
        }
    }
}

void MainWidget::ReadData()
{
    QString message = tcpClient->readAll();
    if(!message.isEmpty())
    {
        analysisMessage(message);
    }
}

void MainWidget::ReadError(QAbstractSocket::SocketError)
{
    tcpClient->disconnectFromHost();
    ui->btn_conn->setText(tr("连接"));
    QMessageBox msgBox;
    msgBox.setText(tr("failed to connect server because %1").arg(tcpClient->errorString()));
    msgBox.exec();
}

/**
 * @brief LED1按钮点击事件
 */
void MainWidget::on_btn_led1_clicked()
{
    if (led1)
    {
        QString buf = QString("#L%1%2").arg(this->led2).arg(0);
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
        ui->btn_led1->setStyleSheet("QPushButton{background:green}");
        this->led1 = 0;
    }
    else
    {
        QString buf = QString("#L%1%2").arg(this->led2).arg(1);
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
        ui->btn_led1->setStyleSheet("QPushButton{background:white}");
        this->led1 = 1;
    }
}

/**
 * @brief LED2按钮点击事件
 */
void MainWidget::on_btn_led2_clicked()
{
    if (led2)
    {
        QString buf = QString("#L%1%2").arg(0).arg(this->led1);
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
        ui->btn_led2->setStyleSheet("QPushButton{background:green}");
        this->led2 = 0;
    }
    else
    {
        QString buf = QString("#L%1%2").arg(1).arg(this->led1);
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
        ui->btn_led2->setStyleSheet("QPushButton{background:white}");
        this->led2 = 1;
    }
}

/**
 * @brief 退出按钮点击事件
 */
void MainWidget::on_btn_quit_clicked()
{
    qApp->quit();
}

/**
 * @brief 连接按钮点击事件
 */
void MainWidget::on_btn_conn_clicked()
{
    if(ui->btn_conn->text()=="连接")
    {
        tcpClient->connectToHost(ui->edit_ip->text(), ui->edit_port->text().toInt());
        if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
        {
            ui->btn_conn->setText("断开");
            ui->btn_led1->setEnabled(true);
            ui->btn_led2->setEnabled(true);
        }
    }
    else
    {
        tcpClient->disconnectFromHost();
        if (tcpClient->state() == QAbstractSocket::UnconnectedState \
                || tcpClient->waitForDisconnected(1000))  //已断开连接则进入if{}
        {
            ui->btn_conn->setText("连接");
        }
    }
}

void MainWidget::on_btn_adSwitch_clicked()
{
    if (adSwitch)
    {
        adSwitch = false;
        ui->btn_adSwitch->setText("开启");
        QString buf = QString("#R0");
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
    }
    else
    {
        adSwitch = true;
        ui->btn_adSwitch->setText("关闭");
        QString buf = QString("#R1");
        qDebug() << buf;
        this->tcpClient->write(buf.toLatin1());
    }
}
