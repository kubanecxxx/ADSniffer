#include "hled.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include <qextserialenumerator.h>
#include <QColorDialog>
#include <QDir>
#include <QTimer>


#define SEPARATOR " ; "
QStringList list;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    rdy(false),
    data(NULL),
    inifile(new QFile)
{
    ui->setupUi(this);

    PortSettings nastaveni;
    nastaveni.BaudRate = BAUD1152000;
    nastaveni.DataBits = DATA_8;
    nastaveni.FlowControl = FLOW_HARDWARE;
    nastaveni.Parity = PAR_NONE;
    nastaveni.StopBits = STOP_1;
    nastaveni.Timeout_Millisec = 10;
    comport = new QextSerialPort(nastaveni,QextSerialPort::EventDriven,this);

    //parse ini file
    list << "PA1" << "PA2" << "PB0" << "PB1" << "PC1" << "PC2" << "PC5";
    list << "PC13"  << "PC14" << "PC15";
    list << "PE2" << "PE3" << "PE4" << "PE5" << "PE6";

    QString path = QCoreApplication::applicationDirPath();
    QString name = "sniffer.ini";
    path = path + "/" + name;
    inifile->setFileName(path);
    bool ok = inifile->open(QFile::ReadOnly);

    inidata_t inidata;
    if (ok)
    {
        QByteArray ini = inifile->readAll();
        inifile->close();
        if (ini.count())
            parseIni(ini,inidata);
    }
    prevPort = inidata.port;
    ui->checkAuto->setChecked(inidata.autoscale);
    ui->checkRec->setChecked(inidata.record);

    FillCombo();
    ledka = new HLed;
    statusBar()->addPermanentWidget(ledka);

    roznout(false);

    for (int i = 0 ; i < CHANNEL_COUNT; i++)
    {
        addRow(inidata.table[i]);
    }

    ui->plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->plot->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->plot->xAxis->setLabel(trUtf8("Čas"));
    ui->plot->yAxis->setLabel(trUtf8("Napětí"));
    ui->plot->legend->setVisible(true);
    ui->plot->legend->setPositionStyle(QCPLegend::psTopLeft);
    ui->plot->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    ui->plot->setRangeZoom(Qt::Horizontal | Qt::Vertical);

    rdy = true;

    inifile->close();

    QTimer::singleShot(100,this,SLOT(saveSetup()));
}

void MainWindow::saveSetup()
{
    inidata_t data;

    data.port = ui->comboPorts->currentText();
    data.autoscale = ui->checkAuto->isChecked();
    data.record = ui->checkRec->isChecked();

    for (int i = 0 ; i < CHANNEL_COUNT; i++)
    {
        tableRow row;
        row.farba = ui->table->item(i,4)->backgroundColor();
        row.name = ui->table->item(i,0)->text();
        row.port = ui->table->item(i,1)->text();
        row.visible = false;
        if (ui->table->item(i,3)->checkState() == Qt::Checked)
            row.visible = true;

        data.table[i] = row;
    }

    iniCreate(data);


    QTimer::singleShot(100,this,SLOT(saveSetup()));
}

void MainWindow::iniCreate(const inidata_t &data)
{
    inifile->open(QFile::WriteOnly);

    inifile->write("[Global]\n");
    inifile->write(QString("autoscale=%1\n").arg(data.autoscale).toUtf8());
    inifile->write(QString("port=%1\n").arg(data.port).toUtf8());
    inifile->write(QString("record=%1\n").arg(data.record).toUtf8());

    inifile->write("[Table]\n");

    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        QString line;
        tableRow row = data.table[i];

        line = row.port + "=" + row.name + ";";
        line += QString("%1;%2;\n").arg(row.visible).arg(row.farba.rgb(),0,16);
        inifile->write(line.toUtf8());
    }

    inifile->close();
}

void MainWindow::parseIni(QByteArray &data,inidata_t & ini)
{
    QByteArray global = iniGetSection("Global",data);
    QByteArray table = iniGetSection("Table",data);
    QByteArray temp;

    temp = iniGetData("autoscale",global);
    ini.autoscale = temp.toInt();
    temp = iniGetData("port",global);
    ini.port = temp;
    temp = iniGetData("record",global);
    ini.record = temp.toInt();

    for (int i = 0 ; i < CHANNEL_COUNT; i++)
    {
        temp = iniGetData(list.at(i).toUtf8().constData(),table);
        tableRow row;
        QList<QByteArray> lst;
        row.port = list.at(i);
        lst = iniGetPar(temp);
        row.name = lst.at(0);
        row.visible = lst.at(1).toInt();
        bool ok;
        quint32 co = lst.at(2).toLongLong(&ok,16);
        row.farba = QColor::fromRgb(co);

        ini.table[i] = row;
    }

    asm("nop");
}

QList<QByteArray> MainWindow::iniGetPar(const QByteArray &line)
{
    QList<QByteArray> ret;
    int i = 0;

    while(line.indexOf(";",i) != -1)
    {
        int j = line.indexOf(";",i);
        QByteArray par = line.mid(i,j-i);
        ret.push_back(par.trimmed());

            i = j+1;
    }

    return ret;
}

QByteArray MainWindow::iniGetData(const char *par, const QByteArray &sec)
{
    QByteArray line;
    QByteArray str;

    str = par;
    str += "=";

    int i = sec.indexOf(str);
    int end = sec.indexOf("\n",i);
    if (end == -1)
        end = sec.length();

    line = sec.mid(i,end-i);
    line = line.trimmed();
    i = 1 + line.indexOf("=");
    line.remove(0,i);
    //ret =

    return line.trimmed();
}

QByteArray MainWindow::iniGetSection(const char * sec,const QByteArray &ini)
{
    QString str = sec;
    str = "[" + str + "]";
    int start = ini.indexOf(str);
    int stop = ini.indexOf("\n[",start+1);

    if (stop == -1)
        stop = ini.length();

    QByteArray ret = ini.mid(start,stop-start);
    ret.replace(str," ");

    return ret.trimmed();
}

MainWindow::~MainWindow()
{
    on_butStop_clicked();
    on_butClose_clicked();
    delete ui;
}

void MainWindow::FillCombo()
{
    ui->comboPorts->clear();
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

    int i = ui->comboPorts->findText(prevPort);
    if (i != -1)
        ui->comboPorts->setCurrentIndex(i);
}

void MainWindow::roznout(bool enable)
{
    ledka->turnOn(enable);
    ui->butStart->setEnabled(enable);
    ui->butStop->setEnabled(enable);
    ui->butClose->setEnabled(enable);
    ui->butOpen->setDisabled(enable);
    ui->butRefresh->setDisabled(enable);
    ui->comboPorts->setDisabled(enable);
    ui->pushButton->setEnabled(enable);
    ui->spinBox->setEnabled(enable);
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

    QString path = QCoreApplication::applicationDirPath();
    QString name = QDateTime::currentDateTime().toString("dd.MM.yyyy_hh.mm.ss");
    name += ".csv";
    path = path + "/" + name;


    if (ui->checkRec->isChecked())
    {
        statusBar()->showMessage("soubor: " + name);
        if (data == NULL )
        {
            data = new QFile(path);
            data->open(QFile::WriteOnly);
            if (data->isWritable())
            {
                data->write("Time; Time formated; ");
                for (int i = 0 ; i < CHANNEL_COUNT; i++)
                {
                    data->write(list.at(i).toUtf8());
                    data->write(SEPARATOR);
                }
                data->write("\n");

                data->write("Time; Time formated; ");
                for (int i = 0 ; i < CHANNEL_COUNT; i++)
                {
                    data->write(ui->table->item(i,0)->text().toUtf8());
                    data->write(SEPARATOR);
                }
                data->write("\n");
                data->close();
            }
            else
            {
                statusBar()->showMessage(trUtf8("Nelze psát do souboru"),1000);
            }
        }
    }

    comport->write("start\n\r");
    on_butClean_clicked();
    ui->checkRec->setEnabled(false);
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
    if (data)
    {
        data->close();
        data->deleteLater();
        data = NULL;
    }
    ui->checkRec->setEnabled(true);
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

    //rozparsovat jednotlivy bity
    int temp = buf.toInt();
    for (int i = 0 ; i < 8; i++)
    {
        pole[NUMBER_COUNT - 1 + i] = 1000 * ((temp >> i) & 1) + i * 10;
    }

    ProcessValues(pole);

    asm("nop");
}


void MainWindow::ProcessValues(int *be)
{
    float pole[CHANNEL_COUNT];

    QDateTime time = QDateTime::currentDateTime();
    if (ui->checkRec->isChecked())
    {
        data->open(QFile::Append);
        if (data->isWritable())
        {
        data->write(QString("%1").arg(time.toTime_t() + time.time().msec() / 1000.0).toUtf8());
        data->write(SEPARATOR);
        data->write(time.time().toString("hh:mm:ss.zzz").toUtf8());
        data->write(SEPARATOR);
        }
    }

    for (int i = 0 ; i < CHANNEL_COUNT; i++)
    {
        pole[i] = be[i] /4096.0 * 2.048;


        if (pole[i] < 0)
            continue;
        if (pole[i] > 10)
            continue;

        ui->plot->graph(i)->addData(time.toTime_t() + time.time().msec() / 1000.0, pole[i]);
        ui->table->item(i,2)->setData(Qt::DisplayRole,pole[i]);

        //file
        if (ui->checkRec->isChecked())
        {
        if (data->isWritable())
        {
            data->write(QString("%1").arg(pole[i]).toUtf8());
            data->write(SEPARATOR);
        }
        else
        {
            statusBar()->showMessage(trUtf8("Nelze psát do souboru"),1000);
        }
        }
    }

    if (ui->checkRec->isChecked())
    {
        if (data->isWritable())
            data->write("\n");
        data->close();
    }

    if (ui->checkAuto->isChecked())
        ui->plot->rescaleAxes();
    ui->plot->replot();
}

void MainWindow::addRow(const tableRow & row)
{
    //tabulka
    int i = ui->table->rowCount();
    ui->table->insertRow(i);

    QTableWidgetItem * it ;
    it = new QTableWidgetItem(row.name);
    ui->table->setItem(i,0,it);

    it = new QTableWidgetItem(row.port);
    it->setFlags(Qt::ItemIsEnabled);
    ui->table->setItem(i,1,it );

    it = new QTableWidgetItem;
    ui->table->setItem(i,2,new QTableWidgetItem);

    //checkbox
    it = new QTableWidgetItem;
    it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    it->setCheckState(Qt::Unchecked);
    if (row.visible)
        it->setCheckState(Qt::Checked);
    ui->table->setItem(i,3,it);

    //barvička
    it = new QTableWidgetItem;
    it->setFlags(Qt::ItemIsEnabled );
    it->setBackgroundColor(row.farba);
    ui->table->setItem(i,4,it);

    //grafy
    QCPGraph * gr = ui->plot->addGraph();
    gr->setPen(it->backgroundColor());
    gr->setVisible(row.visible);
    gr->setName(row.port);
    if (row.name.count())
        gr->setName(row.name);

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

void MainWindow::on_butRefresh_clicked()
{
    FillCombo();
}

void MainWindow::on_pushButton_clicked()
{
    if (!comport->isWritable())
    {
        statusBar()->showMessage(trUtf8("nelze psát do COM"));
        return;
    }
    QByteArray arr = QString("speed %1\n\r").arg(ui->spinBox->value()).toAscii();
    comport->write(arr);
}
