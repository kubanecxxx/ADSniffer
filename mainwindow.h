#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qextserialport.h>
#include <QTime>
#include <QFile>

namespace Ui {
class MainWindow;
}



#define CHANNEL_COUNT 7
class tableRow
{
public:
    tableRow()
    {
        visible = false;
    }
    QColor farba;
    bool visible;
    QString name;
    QString port;
};

class inidata_t
{
public:
    inidata_t()
    {
        autoscale = true;
        record = true;
    }

    bool autoscale;
    bool record;
    QString port;
    tableRow table[CHANNEL_COUNT];
};

class HLed;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_butOpen_clicked();
    void on_butClose_clicked();
    void on_butStart_clicked();
    void on_butStop_clicked();
    void comport_newData(void);
    void on_table_cellDoubleClicked(int row, int column);
    void on_table_cellClicked(int row, int column);
    void on_table_cellChanged(int row, int column);
    void on_butClean_clicked();
    void on_butRefresh_clicked();
    void saveSetup();

private:
    Ui::MainWindow *ui;

    bool rdy;
    QTime time;
    void FillCombo(void);
    void roznout(bool enable);
    void addRow(const tableRow & row);
    void ProcessValues(int * pole);
    QFile * data;
    QFile * inifile;

    QextSerialPort * comport;
    HLed * ledka;
    QByteArray buffer;
    void parseIni(QByteArray & data, inidata_t & ret);
    QString prevPort;


    QByteArray iniGetSection(const char * sec, const QByteArray & ini);
    QByteArray iniGetData(const char * par, const QByteArray & sec);
    QList<QByteArray> iniGetPar(const QByteArray & line);
    void iniCreate(const inidata_t & data);
};

#endif // MAINWINDOW_H
