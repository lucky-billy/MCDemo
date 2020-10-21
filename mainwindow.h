#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString convert10216(int num, int bit);

    QString convert16210(QString num);

//    char convertHexChar(char ch);

//    QByteArray string2Hex(QString str);

//    QString hexToString(unsigned char *in, int len);

private slots:

    void on_connectBtn_clicked();

    void on_writeBtn_clicked();

    void on_readBtn_clicked();

    void on_clearBtn_clicked();

    void readData();

private:
    Ui::MainWindow *ui;

    QTcpSocket *network;
    int timeout;
};

#endif // MAINWINDOW_H
