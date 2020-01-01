#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "qcustomplot.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();
    //设置折线属性
    void initPlot(QCustomPlot *customPlot);
    void updatePlot(int value); //更新曲线
    void analysisMessage(QString msg);

private slots:
    void ReadData();
    void ReadError(QAbstractSocket::SocketError);

    void on_btn_led1_clicked();
    void on_btn_led2_clicked();
    void on_btn_quit_clicked();
    void on_btn_conn_clicked();
    void on_btn_adSwitch_clicked();

private:
    Ui::MainWidget *ui;

    QTcpSocket *tcpClient;

    int led1;
    int led2;
    int adSwitch; //AD数据发送给客户端的开关

};

#endif // MAINWIDGET_H
