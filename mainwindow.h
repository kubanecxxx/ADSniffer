#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qextserialport.h>
#include <QTime>

namespace Ui {
class MainWindow;
}

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

private:
    Ui::MainWindow *ui;

    bool rdy;
    QTime time;
    void FillCombo(void);
    void roznout(bool enable);
    void addRow(const QString & port);
    void ProcessValues(int * pole);

    QextSerialPort * comport;
    HLed * ledka;
    QByteArray buffer;
};

#endif // MAINWINDOW_H
