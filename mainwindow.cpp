#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QHostAddress>
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // init
    network = new QTcpSocket(this);
    timeout = 1000;

    connect(network, &QTcpSocket::connected, [](){ qDebug() << "Connected to PLC successfully !" << endl; });
    connect(network, &QTcpSocket::disconnected, [](){ qDebug() << "Disconnected from plc !" << endl; });
    connect(network, &QTcpSocket::stateChanged, [](QAbstractSocket::SocketState socketState){
        qDebug() << "SocketState changed: " << socketState;
    });
    connect(network, &QTcpSocket::readyRead, [&](){ readData(); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_connectBtn_clicked()
{
    if ( ui->connectBtn->text() == "connect" )
    {
        network->connectToHost(ui->ip->text(), ui->port->text().toInt());
        if ( network->waitForConnected(timeout) ) {
            QMessageBox::information(this, "Infomation", "Connected to PLC successfully !");
            ui->connectBtn->setText("disconnect");

            qDebug() << "localAddress: " << network->localAddress();
            qDebug() << "localPort: " << network->localPort();
            qDebug() << "peerAddress: " << network->peerAddress();
            qDebug() << "peerPort: " << network->peerPort();
            qDebug() << "peerName: " << network->peerName();
            qDebug() << "isOpen: " << network->isOpen();
            qDebug() << "isValid: " << network->isValid();
        } else {
            QMessageBox::critical(this, "Error", "Connect failed !");
        }
    }
    else
    {
        network->disconnectFromHost();
        ui->connectBtn->setText("connect");
    }
}

void MainWindow::on_writeBtn_clicked()
{
    if ( !network->isWritable() ) {
        QMessageBox::critical(this, "Error", "Network is unable to write !");
        return;
    }

    /*
     * MC 协议解析
     * 以二进制代码进行数据通信时，将2字节的数值从低位字节(L:位0～7)进行发送
     * 以ASCII代码进行数据通信时，将数值转换为ASCII代码4位(16进制数)后从高位进行发送
     *
     * 二进制格式：
     * 1. 帧头 - 是以太网的帧头。通常自动被添加
     * 2. 副帧头 - 3E帧，固定请求报文5000，响应报文D000                                   50 00
     * 3. 访问路径 - 指定访问路径
     *    网络编号 - 上位访问下位，固定00                                                00
     *    可编程控制器编号 - 上位访问下位，固定FF                                          FF
     *    请求目标模块I/O编号 - 固定03 FF，十进制是1023                                   FF 03
     *    请求目标模块站号 - 上位访问下位，固定00                                          00
     * 4. 请求数据长 - 二进制方式两个数字为一个字，ASCII为一个数字一个字（18个字，0x0012）       12 00
     * 5. 监视定时器 - 等待时间（0000H(0): 无限等待；0001H～FFFFH(1～65535)(单位: 250ms)）  10 00
     * 6. 请求数据 - 请求数据设置表示请求内容的指令等
     *    指令 - 批量读取 0401，批量写入 1401                                           01 14
     *    子指令 - 字单位(1个字=16位) 0000，位单位 0001                                  00 00
     *    起始软元件编号 - 100                                                         64 00 00
     *    软元件代码 - A8-D点，90-M点；9C-X点；9D-Y点；B0-ZR外部存储卡                     A8
     *    软元件点数 - 3                                                              03 00
     *    写入数据 - 6549 4610 4400                                                   95 19 02 12 30 11
     *
     * ASCII格式：
     * 1. 帧头
     * 2. 副帧头 - 35 30 30 30
     * 3. 访问路径 - 30 30 46 46 30 33 46 46 30 30
     * 4. 请求数据长 - 30 30 34 38
     * 5. 监视定时器 - 30 30 31 30
     * 6. 请求数据
     *    指令 - 31 34 30 31
     *    子指令 - 30 30 30 30
     *    软元件代码 - 44 2A
     *    起始软元件编号 - 30 30 30 31 30 30
     *    软元件点数 - 30 30 30 33
     *    写入数据 - 31 39 39 35 31 32 30 32 31 31 33 30
     */

    /*
    QString ret = "50 00 00 FF FF 03 00 0C 00 10 00 01 04 00 00 64 00 00 A8 01 00";                     // 二进制读取
    QString ret = "50 00 00 FF FF 03 00 12 00 10 00 01 14 00 00 64 00 00 A8 03 00 95 19 02 12 30 11";   // 二进制写入

    // ASCII 读取
    QString ret = "35 30 30 30 "
                  "30 30 46 46 30 33 46 46 30 30 "
                  "30 30 33 30 "
                  "30 30 31 30 "
                  "30 34 30 31 "
                  "30 30 30 30 "
                  "44 2A "
                  "30 30 30 31 30 30 "
                  "30 30 30 31";

    // ASCII 写入
    QString ret = "35 30 30 30 "
                  "30 30 46 46 30 33 46 46 30 30 "
                  "30 30 34 38 "
                  "30 30 31 30 "
                  "31 34 30 31 "
                  "30 30 30 30 "
                  "44 2A "
                  "30 30 30 31 30 30 "
                  "30 30 30 33 "
                  "31 39 39 35 31 32 30 32 31 31 33 30";
    */

    QString writeData = ui->writeData->toPlainText();
    QStringList list = writeData.split(" ");

    QString str = "";
    for ( int i = 0; i < ui->length->text().toInt(); ++i )
    {
        if ( i < list.length() ) {
            str += convert10216(list.at(i).toInt(), 4) + " ";
        } else {
            str += convert10216(0, 4) + " ";
        }
    }

    QString head = "50 00 00 FF FF 03 00";
    QString length;
    QString timeout = "10 00";
    QString command = "01 14";
    QString subCommand = "00 00";
    QString address = convert10216(ui->address->text().toInt(), 6);
    QString soft = ui->comboBox->currentIndex() == 0 ? "A8" : "90";
    QString count = convert10216(ui->length->text().toInt(), 4);

    QString data = timeout + " " + command + " " + subCommand + " " + address + " " + soft + " " + count + " " + str;
    QString temp = data;
    QRegExp regExp("[^a-fA-F0-9]");
    temp.replace(regExp, "");
    int len = temp.length()/2;
    length = convert10216(len, 4);

    QString ret = head + " " + length + " " + data;

    QDateTime datetime(QDateTime::currentDateTime());
    QString time = "[" + datetime.toString("hh:mm:ss") + "]";
    ui->logData->append(time + " [WriteData] " + ret);

//    QByteArray array = string2Hex(ret);
    QByteArray array = QByteArray::fromHex(ret.toLatin1());
    network->write(array);
    network->waitForBytesWritten();
    network->flush();
}

void MainWindow::on_readBtn_clicked()
{
    if ( !network->isWritable() ) {
        QMessageBox::critical(this, "Error", "Network is unable to write !");
        return;
    }

    ui->readData->clear();

    QString head = "50 00 00 FF FF 03 00";
    QString length;
    QString timeout = "10 00";
    QString command = "01 04";
    QString subCommand = "00 00";
    QString address = convert10216(ui->address->text().toInt(), 6);
    QString soft = ui->comboBox->currentIndex() == 0 ? "A8" : "90";
    QString count = convert10216(ui->length->text().toInt(), 4);

    QString data = timeout + " " + command + " " + subCommand + " " + address + " " + soft + " " + count + " ";
    QString temp = data;
    QRegExp regExp("[^a-fA-F0-9]");
    temp.replace(regExp, "");
    int len = temp.length()/2;
    length = convert10216(len, 4);

    QString ret = head + " " + length + " " + data;

    QDateTime datetime(QDateTime::currentDateTime());
    QString time = "[" + datetime.toString("hh:mm:ss") + "]";
    ui->logData->append(time + " [ReadData] " + ret);

//    QByteArray array = string2Hex(ret);
    QByteArray array = QByteArray::fromHex(ret.toLatin1());
    network->write(array);
    network->waitForBytesWritten();
    network->flush();
}

void MainWindow::on_clearBtn_clicked()
{
    ui->logData->clear();
}

void MainWindow::readData()
{
    QByteArray array = network->readAll();
//    QString ret = hexToString((unsigned char *)array.data(), array.size());
    QString ret = array.toHex().toUpper();
    int count = 0;
    for ( int i = 0; i < ret.size(); ++i )
    {
        int pos = 2 * (i + 1) + count;
        ret.replace(pos, 0, " ");
        count++;
    }

    QDateTime datetime(QDateTime::currentDateTime());
    QString time = "[" + datetime.toString("hh:mm:ss") + "]";
    ui->logData->append(time + " [Response] " + ret + "\n");

    QRegExp regExp("[^a-fA-F0-9]");
    ret.replace(regExp, "");

    QString str = ret.mid(18);
    if ( str.left(4) != "0000" ) {
        QMessageBox::critical(this, "Error", "An error occurred !");
    } else {
        str = ret.mid(14);
        int length = convert16210(str.left(4)).toInt();
        if ( length > 2 ) {
            str = ret.mid(22);
            QString data = "";
            for ( int i = 0; i < str.length(); i = i + 4 )
            {
                data += convert16210(str.mid(i, 4)) + " ";
            }

            ui->readData->append(data);
        }
    }
}

QString MainWindow::convert10216(int num, int bit)
{
    QString str = QString::number(num, 16).toUpper();

    int len = bit - str.length();
    for ( int i = 0; i < len; ++i )
    {
        str.push_front("0");
    }

    if ( bit == 4 ) {
        return str.right(2) + " " + str.left(2);
    } else if ( bit == 6 ) {
        return str.right(2) + " " + str.mid(2, 2) + " " + str.left(2);
    } else {
        return "";
    }
}

QString MainWindow::convert16210(QString num)
{
    QString str = num.right(2) + num.left(2);
    bool ok;
    int hex = str.toInt(&ok, 16);
    return ok ? QString::number(hex) : "";
}

/*
char MainWindow::convertHexChar(char ch)
{
    if ( (ch >= '0') && (ch <= '9') )
    {
        return ch - 0x30;
    }
    else if ( (ch >= 'A') && (ch <= 'F') )
    {
        return ch - 'A' + 10;
    }
    else if ( (ch >= 'a') && (ch <= 'f') )
    {
        return ch - 'a' + 10;
    }
    else
    {
        return (-1);
    }
}

QByteArray MainWindow::string2Hex(QString str)
{
    QByteArray send_data;
    int hex_data,low_hex_data;
    int hex_data_len = 0;
    int len = str.length();
    send_data.resize(len/2);
    char lstr, hstr;

    for ( int i = 0 ; i < len; )
    {
        hstr = str[i].toLatin1();
        if ( hstr == ' ' ) {
            i++;
            continue;
        }

        i++;
        if ( i >= len ) {
            break;
        }

        lstr = str[i].toLatin1();
        hex_data = convertHexChar(hstr);
        low_hex_data = convertHexChar(lstr);

        if ( (hex_data == 16) || (low_hex_data == 16) ) {
            break;
        } else {
            hex_data = hex_data * 16 + low_hex_data;
        }

        i++;
        send_data[hex_data_len] = (char)hex_data;
        hex_data_len++;
    }

    send_data.resize(hex_data_len);
    return send_data;
}

QString MainWindow::hexToString(unsigned char *in, int len)
{
    int i;
    unsigned char inChar, hi, lo;
    QString s;

    for ( i = 0; i < len; ++i )
    {
        inChar = in[i];

        hi = (inChar & 0xF0) >> 4;
        if (hi > 9)
            hi = 'A' + (hi - 0x0A);
        else
            hi += 0x30;
        s.append(hi);

        lo = inChar & 0x0F;
        if (lo > 9)
            lo = 'A' + (lo - 0x0A);
        else
            lo += 0x30;
        s.append(lo);

        s.append(0x20);
    }

    return s;
}
*/
