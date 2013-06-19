#include "hled.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include <qextserialenumerator.h>
#include <QColorDialog>

#define CHANNEL_COUNT 7

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    rdy(false)
{
    ui->setupUi(this);

    FillCombo();
    roznout(false);


    QStringList list;
    list << "PA1" << "PA2" << "PB0" << "PB1" << "PC1" << "PC2" << "PC5";

    foreach (QString str, list) {
        addRow(str);
    }


    //ui->plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    //ui->plot->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->plot->xAxis->setLabel(trUtf8("Čas"));
    ui->plot->yAxis->setLabel(trUtf8("Napětí"));
    ui->plot->legend->setVisible(true);
    ui->plot->legend->setPositionStyle(QCPLegend::psTopLeft);
    ui->plot->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    ui->plot->setRangeZoom(Qt::Horizontal | Qt::Vertical);

    rdy = true;

}

MainWindow::~MainWindow()
{
    on_butStop_clicked();
    on_butClose_clicked();
    delete ui;
}

void MainWindow::FillCombo()
{
    ledka = new HLed;
    statusBar()->addPermanentWidget(ledka);

    PortSettings nastaveni;
    nastaveni.BaudRate = BAUD1152000;
    nastaveni.DataBits = DATA_8;
    nastaveni.FlowControl = FLOW_HARDWARE;
    nastaveni.Parity = PAR_NONE;
    nastaveni.StopBits = STOP_1;
    nastaveni.Timeout_Millisec = 10;

    comport = new QextSerialPort(nastaveni,QextSerialPort::EventDriven,this);
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    QStringList list;

    for (int i = 0 ; i < ports.count() ; i++)
    {

    #ifdef Q_OS_WIN
         list << ports.at(i).portName;
    #else
        list << ports.at(i).physName;

    #endif

    }

    #ifndef Q_OS_WIN
    list << "/tmp/interceptty";
    #endif

    ui->comboPorts->addItems(list);
}

void MainWindow::roznout(bool enable)
{
    ledka->turnOn(enable);
    ui->butStart->setEnabled(enable);
    ui->butStop->setEnabled(enable);
    ui->butClose->setEnabled(enable);
    ui->butOpen->setDisabled(enable);
}

void MainWindow::on_butOpen_clicked()
{
    comport->setPortName(ui->comboPorts->currentText());
    comport->open(QextSerialPort::ReadWrite);

    roznout(comport->isOpen());

    if (comport->isOpen())
    {
        statusBar()->showMessage(trUtf8("Připojeno k zařízení"));
    }
    else
    {
        statusBar()->showMessage(trUtf8("Nepovedlo se připojit k zařízení"));
    }
}

void MainWindow::on_butClose_clicked()
{
    statusBar()->showMessage(trUtf8("odpojeno od zařízení"));
    roznout(false);
    comport->close();
}

void MainWindow::on_butStart_clicked()
{
    connect(comport,SIGNAL(readyRead()),this,SLOT(comport_newData()));
    if (!comport->isWritable())
    {
        statusBar()->showMessage(trUtf8("nelze psát do COM"));
        return;
    }
    comport->write("start\n\r");
    on_butClean_clicked();


}

void MainWindow::on_butStop_clicked()
{
    disconnect(comport,SIGNAL(readyRead()),this,SLOT(comport_newData()));
    if (!comport->isWritable())
    {
        statusBar()->showMessage(trUtf8("nelze psát do COM"));
        return;
    }
    comport->write("stop\n\r");

}

void MainWindow::comport_newData()
{
    static unsigned int jo = 0;
    buffer += comport->readAll();

    if (jo++ < 2)
    {
        buffer.clear();
        return;
    }

    if (!buffer.contains('\n'))
        return;

    int pole[CHANNEL_COUNT];
    QByteArray buf = buffer.left(buffer.indexOf('\n'));
    buffer.remove(0,buffer.indexOf('\n') + 2);

    int i = 0;
    while(buf.contains(' '))
    {
        QByteArray num;
        num = buf.left(buf.indexOf(' '));
        buf.remove(0,buf.indexOf(' ') + 1);
        pole[i++] = num.toInt();
    }

    ProcessValues(pole);
}


void MainWindow::ProcessValues(int *pole)
{
    for (int i = 0 ; i < CHANNEL_COUNT; i++)
    {
        ui->plot->graph(i)->addData(time.elapsed()/1000.0, pole[i]);
        ui->table->item(i,2)->setData(Qt::DisplayRole,pole[i]);
    }

    ui->plot->rescaleAxes();
    ui->plot->replot();
}

void MainWindow::addRow(const QString & port)
{
    //tabulka
    int i = ui->table->rowCount();
    ui->table->insertRow(i);

    QTableWidgetItem * it ;
    it = new QTableWidgetItem;
    ui->table->setItem(i,0,it);

    it = new QTableWidgetItem(port);
    it->setFlags(Qt::ItemIsEnabled);
    ui->table->setItem(i,1,it );

    it = new QTableWidgetItem;
    ui->table->setItem(i,2,new QTableWidgetItem);

    //checkbox
    it = new QTableWidgetItem;
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    it->setCheckState(Qt::Unchecked);
    ui->table->setItem(i,3,it);

    //barvička
    it = new QTableWidgetItem;
    it->setFlags(Qt::ItemIsEnabled );
    it->setBackgroundColor(random());
    ui->table->setItem(i,4,it);

    //grafy
    QCPGraph * gr = ui->plot->addGraph();
    gr->setPen(it->backgroundColor());
    gr->setVisible(false);
    gr->setName(port);

}

void MainWindow::on_table_cellDoubleClicked(int row, int column)
{
    if (column != 4)
        return;

    QColor color = QColorDialog::getColor(ui->table->item(row,column)->backgroundColor());

    if (color.isValid())
    {
        ui->table->item(row,column)->setBackgroundColor(color);
        ui->plot->graph(row)->setPen(color);
        ui->plot->replot();
    }
}

void MainWindow::on_table_cellClicked(int row, int column)
{
    if (column != 3)
     return;

    asm("nop");

    QTableWidgetItem * it = ui->table->item(row,column);
    bool show;

    if (it->checkState() == Qt::Checked)
    {
        show = true;
    }
    else if (it->checkState() == Qt::Unchecked)
    {
        show = false;
    }

    ui->plot->graph(row)->setVisible(show);
    ui->plot->replot();
}

void MainWindow::on_table_cellChanged(int row, int column)
{
    if (!rdy)
        return;

    if (column != 0)
        return;

    QTableWidgetItem * it = ui->table->item(row,column);
    ui->plot->graph(row)->setName(it->text());
    ui->plot->replot();
}

void MainWindow::on_butClean_clicked()
{
    for (int i = 0 ; i < ui->plot->graphCount(); i++)
    {
        ui->plot->graph(i)->clearData();
    }
    time.restart();
}
