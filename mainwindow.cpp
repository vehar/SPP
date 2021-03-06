/***************************************************************************
**  This file is part of Serial Port Plotter                              **
**                                                                        **
**                                                                        **
**  Serial Port Plotter is a program for plotting integer data from       **
**  serial port using Qt and QCustomPlot                                  **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Borislav                                             **
**           Contact: b.kereziev@gmail.com                                **
**           Date: 29.12.14                                               **
****************************************************************************/

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

/******************************************************************************************************************/
/* Constructor */
/******************************************************************************************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    connected(false),
    plotting(false),
    dataPointNumber(0),
    numberOfAxes(10),
    STATE(WAIT_START),
    NUMBER_OF_POINTS(500),
    updatePeriod(200)
{
    qDebug() << "MainWindow(" << parent << ");\r";

    ui->setupUi(this);
    createUI(); // Create the UI

    // Populate axes combo box;                 // n axes maximum allowed
    ui->comboAxes->setValue(numberOfAxes);

    ui->plot->setBackground(QBrush(QColor(48,47,47)));// Background for the plot area
    setupPlot(); // Setup plot area
    // Plot button is disabled initially
    ui->stopPlotButton->setEnabled(false);
    // used for higher performance (see QCustomPlot real time example)
    ui->plot->setNotAntialiasedElements(QCP::aeAll);

    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plot->xAxis->setTickLabelFont(font);
    ui->plot->yAxis->setTickLabelFont(font);

    font.setPixelSize(15); //font size
    ui->plot->legend->setFont(font);
    ui->plot->legend->setVisible(true);
    ui->plot->legend->setBrush(QBrush(QColor(128, 128, 128, 128)));// backgroung!
    ui->plot->legend->setTextColor(QColor(200,255,200));
    //ui->plot->legend->clearItems();

    // User can change tick step with a spin box
    ui->plot->yAxis->setAutoTickStep(false);
    // Set initial tick step
    ui->plot->yAxis->setTickStep(500);
    // Tick labels color
    ui->plot->xAxis->setTickLabelColor(QColor(170,170,170));
    // See QCustomPlot examples / styled demo
    ui->plot->yAxis->setTickLabelColor(QColor(170,170,170));
    ui->plot->xAxis->grid()->setPen(QPen(QColor(170,170,170), 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setPen(QPen(QColor(170,170,170), 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible(true);
    ui->plot->yAxis->grid()->setSubGridVisible(true);
    ui->plot->xAxis->setBasePen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setBasePen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setTickPen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setTickPen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setSubTickPen(QPen(QColor(170,170,170)));
    ui->plot->yAxis->setSubTickPen(QPen(QColor(170,170,170)));
    ui->plot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->setInteraction(QCP::iRangeDrag, true);
    ui->plot->setInteraction(QCP::iRangeZoom, true);
                                                                                          // Slot for printing coordinates
    connect(ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(onMouseMoveInPlot(QMouseEvent*)));

    // Set serial port pointer to NULL initially
    serialPort = nullptr;
    // Connect update timer to replot slot
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(replot()));

    //все что ниже потом нужно удалить
    //enableControls(true);
    //ui->connectButton->setEnabled(true);
    //ui->saveJPGButton->setEnabled(true);
    ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////
    writer.setFileName("test.txt");
    writer.open(QIODevice::ReadWrite | QIODevice::Text);
    ui->StartStopReadFileBtn->setDisabled(true);
    connect(ui->OpenCloseFileBtn, SIGNAL(clicked(bool)), this, SLOT(slotOpenFile()));
    connect(ui->StartStopReadFileBtn, SIGNAL(clicked(bool)), this, SLOT(slotStartStopReadFile()));
    connect(&timerReadFile, SIGNAL(timeout()), this, SLOT(slotReadFile()));

}


/******************************************************************************************************************/
/* Destructor */
/******************************************************************************************************************/
MainWindow::~MainWindow()
{
    qDebug() << "~MainWindow();\r";
    if(serialPort != nullptr) delete serialPort;
    delete ui;
    writer.close();
    reader.close();
}

void MainWindow::slotOpenFile(){
    static bool open = false;
    if(!open){
        reader.setFileName("test.txt");
        reader.open(QIODevice::ReadWrite | QIODevice::Text);
        ui->StartStopReadFileBtn->setEnabled(true);
        ui->comboAxes->setEnabled(true);
        ui->OpenCloseFileBtn->setText("Close File");
    }
    else{
        reader.close();
        ui->StartStopReadFileBtn->setDisabled(true);
        ui->comboAxes->setDisabled(true);
        ui->OpenCloseFileBtn->setText("Open File");
    }
    open = !open;
}

void MainWindow::slotStartStopReadFile(){
    static bool start = false;
    if(!start){
        ui->StartStopReadFileBtn->setText("Stop");
        timerReadFile.start(0); //TODO: reading speed
        updateTimer.start(updatePeriod);
    }
    else{
        ui->StartStopReadFileBtn->setText("Start");
        timerReadFile.stop();
        updateTimer.stop();
    }
    start = !start;
}

void MainWindow::slotReadFile()
{
    qDebug() << "MainWindow::slotReadFile();\r";
    QStringList newData;
    QString str;
    if(!reader.atEnd())
    {
        str = reader.readLine();
        qDebug() << str << ";\r";
        newData = str.split(",");
    }
    else
    {
        slotStartStopReadFile();
    }
    int dataListSize = newData.size();
    dataPointNumber++;
    drawPoint(newData, numberOfAxes, 0, dataListSize);
/*
    if(dataListSize > 5)
    {
       switch(numberOfAxes)
       {
            case 1:
            {
                drawPoint(newData, 3, 0, dataListSize);
            } break;

            case 2:
            {
                drawPoint(newData, 1, 3, dataListSize);
            } break;

            case 3:
            {
                drawPoint(newData, 1, 4, dataListSize);
            } break;

            case 4:
            {
                drawPoint(newData, 1, 5, dataListSize);
            } break;

           case 6:
           {
               drawPoint(newData, 6, 0, dataListSize);
           } break;

            default: break;
       }
    }
    */
}

/******************************************************************************************************************/
/* Slot for new data from serial port . Data is comming in QStringList and needs to be parsed */
/******************************************************************************************************************/
void MainWindow::onNewDataArrived(QStringList newData)
{
    qDebug() << "MainWindow::onNewDataArrived(" << newData << ");\r";
    QString str = "";
            str += newData.join(",");
            str += "\n";
            writer.write(str.toLocal8Bit());
    if(plotting)
    {
        // Get size of received list
        int dataListSize = newData.size();
        // Increment data number
        dataPointNumber++;
        drawPoint(newData, numberOfAxes, 0, dataListSize);
    /*
        if(dataListSize > 5)
        {
           switch(numberOfAxes)
           {
                case 1:
                {
                    drawPoint(newData, 3, 0, dataListSize);
                } break;

                case 2:
                {
                    drawPoint(newData, 1, 3, dataListSize);
                } break;

                case 3:
                {
                    drawPoint(newData, 1, 4, dataListSize);
                } break;

                case 4:
                {
                    drawPoint(newData, 1, 5, dataListSize);
                } break;

               case 6:
               {
                   drawPoint(newData, 6, 0, dataListSize);
               } break;

                default: break;
           }
        }
        */
    }
}

/******************************************************************************************************************/
/* Read data for inside serial port */
/******************************************************************************************************************/
void MainWindow::readData()
{
    qDebug() << "MainWindow::readData();\r";
    // If any bytes are available
    if(serialPort->bytesAvailable()) {
        // Read all data in QByteArray
        QByteArray data = serialPort->readAll();

        // If the byte array is not empty
        if(!data.isEmpty())
        {
            // Get a '\0'-terminated char* to the data
            char *temp = data.data();

            // Iterate over the char*
            for(int i = 0; temp[i] != '\0'; i++)
            {
                // Switch the current state of the message
                switch(STATE)
                {
                // If waiting for start [$], examine each char
                case WAIT_START:
                    // If the char is $, change STATE to IN_MESSAGE
                    if(temp[i] == START_MSG)
                    {
                        STATE = IN_MESSAGE;
                        // Clear temporary QString that holds the message
                        receivedData.clear();
                        // Break out of the switch
                        break;
                    }
                    break;
                // If state is IN_MESSAGE
                case IN_MESSAGE:
                    // If char examined is ;, switch state to END_MSG
                    if(temp[i] == END_MSG)
                    {
                        STATE = WAIT_START;
                        // Split string received from port and put it into list
                        QStringList incomingData = receivedData.split(' ');
                        // Emit signal for data received with the list
                        emit newData(incomingData);
                        break;
                    }
                    else if(isdigit(temp[i]) || isspace(temp[i]) )  // If examined char is a digit, and not '$' or ';', append it to temporary string
                    {
                        receivedData.append(temp[i]);
                    }
                    break;
                default: break;
                }
            }
        }
    }
}

void MainWindow::drawPoint(QStringList newData, int axesNum, int startData, int size)
{
    int data = 0;
    // Add data to graphs according to number of axes
    for(int i = 0; ((i < axesNum) && (size > i)); ++i)
    {
        data = newData[i+startData].toInt();
        //if(startData == 5){data = data*10;}
        ui->plot->graph(i+startData)->addData(dataPointNumber, data);
       if(ui->savePrevDataCheck->isChecked() == false)
       {
        ui->plot->graph(i+startData)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
       }
    }
}

/******************************************************************************************************************/
/* Replot */
/******************************************************************************************************************/
void MainWindow::replot()
{
    qDebug() << "MainWindow::replot();\r";
   // if(connected)
    {
        if(ui->followDataCheck->isChecked())
        {
            ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
        }
        ui->plot->replot();
    }
}

/******************************************************************************************************************/
/* Create the GUI */
/******************************************************************************************************************/
void MainWindow::createUI()
{
    qDebug() << "MainWindow::createUI();\r";
    // Check if there are any ports at all; if not, disable controls and return
    if(QSerialPortInfo::availablePorts().size() == 0) {
        enableControls(false);
        ui->connectButton->setEnabled(false);
        ui->statusBar->showMessage("No ports detected.");
        ui->saveJPGButton->setEnabled(false);
        return;
    }

    // List all available serial ports and populate ports combo box
    for(QSerialPortInfo port : QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(port.portName());
    }

    // Populate baud rate combo box
    ui->comboBaud->addItem("1200");
    ui->comboBaud->addItem("2400");
    ui->comboBaud->addItem("4800");
    ui->comboBaud->addItem("9600");
    ui->comboBaud->addItem("19200");
    ui->comboBaud->addItem("38400");
    ui->comboBaud->addItem("57600");
    ui->comboBaud->addItem("115200");
    ui->comboBaud->addItem("460800");

    // Select 9600 bits by default
    ui->comboBaud->setCurrentIndex(3);

    // Populate data bits combo box
    ui->comboData->addItem("8 bits");
    ui->comboData->addItem("7 bits");

    // Populate parity combo box
    ui->comboParity->addItem("none");
    ui->comboParity->addItem("odd");
    ui->comboParity->addItem("even");

    // Populate stop bits combo box
    ui->comboStop->addItem("1 bit");
    ui->comboStop->addItem("2 bits");

}


/******************************************************************************************************************/
/* Enable/disable controls */
/******************************************************************************************************************/
void MainWindow::enableControls(bool enable)
{
    qDebug() << "MainWindow::enableControls(" << enable << ");\r";
    // Disable controls in the GUI
    ui->comboBaud->setEnabled(enable);
    ui->comboData->setEnabled(enable);
    ui->comboParity->setEnabled(enable);
    ui->comboPort->setEnabled(enable);
    ui->comboStop->setEnabled(enable);
    ui->comboAxes->setEnabled(enable);
}

/******************************************************************************************************************/
/* Setup the plot area */
/******************************************************************************************************************/
void MainWindow::setupPlot()
{
    qDebug() << "MainWindow::setupPlot();\r";

    ui->plot->clearItems();// Remove everything from the plot
    ui->plot->yAxis->setTickStep(ui->spinYStep->value());// Set tick step according to user spin box

    // Get number of axes from the user combo
    numberOfAxes = ui->comboAxes->value();

    // Set lower and upper plot range
    ui->plot->yAxis->setRange(ui->spinAxesMin->value(), ui->spinAxesMax->value());
    // Set x axis range for specified number of points
    ui->plot->xAxis->setRange(0, NUMBER_OF_POINTS);

    QColor color = QColor(255, 0, 0, 255);

    //removeGraph();
    for(int i = 0; i < numberOfAxes; ++i)
    {
        ui->plot->addGraph();
        switch(i%10){
        case 0:
            ui->plot->graph(i)->setPen(QPen(color, 2, Qt::SolidLine, Qt::FlatCap));
            break;
        case 1:
            ui->plot->graph(i)->setPen(QPen(Qt::green));
            break;
        case 2:
            ui->plot->graph(i)->setPen(QPen(Qt::blue));
            break;
        case 3:
            ui->plot->graph(i)->setPen(QPen(Qt::white));
            break;
        case 4:
            ui->plot->graph(i)->setPen(QPen(Qt::magenta));
            break;
        case 5:
            ui->plot->graph(i)->setPen(QPen(Qt::yellow));
            break;
        case 6:
            ui->plot->graph(i)->setPen(QPen(Qt::cyan));
            break;
        case 7:
            ui->plot->graph(i)->setPen(QPen(Qt::gray));
            break;
        case 8:
            ui->plot->graph(i)->setPen(QPen(color, 2, Qt::DotLine, Qt::FlatCap));
            break;
        case 9:
            ui->plot->graph(i)->setPen(QPen(color, 3, Qt::DashLine, Qt::FlatCap));
            break;
        case 10:
            ui->plot->graph(i)->setPen(QPen(color, 6, Qt::DotLine, Qt::FlatCap));
            break;
        default: ui->plot->graph(i)->setPen(QPen(Qt::white));
            break;
        }
    }
    ui->plot->graph(0)->setName("PM_NP_2.5");
    ui->plot->graph(1)->setName("PM_NP_5 х10");
    ui->plot->graph(2)->setName("PM_NP_10 х8");
    ui->plot->graph(3)->setName("T 1");
    ui->plot->graph(4)->setName("T 2 ");
    ui->plot->graph(5)->setName("T 3");
    ui->plot->graph(6)->setName("T 4");
    ui->plot->graph(7)->setName("MCU Temp");
    ui->plot->graph(8)->setName("CO2 level");
    ui->plot->graph(9)->setName("...");
   // ui->plot->graph(6)->setName("Temperature 1");
   // ui->plot->graph(7)->setName("Temperature 2");
}


/******************************************************************************************************************/
/* Open the inside serial port; connect its signals */
/******************************************************************************************************************/
void MainWindow::openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits)
{
    qDebug() << "MainWindow::openPort(portInfo, baudRate, dataBits, parity, stopBits);\r";
    // Create a new serial port
    serialPort = new QSerialPort(portInfo, nullptr);

    // Connect port signals to GUI slots
    connect(this, SIGNAL(portOpenOK()), this, SLOT(portOpenedSuccess()));
    connect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    connect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));

    if(serialPort->open(QIODevice::ReadWrite) ) {
        serialPort->setBaudRate(baudRate);
        serialPort->setParity(parity);
        serialPort->setDataBits(dataBits);
        serialPort->setStopBits(stopBits);
        emit portOpenOK();
    } else {
        emit portOpenedFail();
        qDebug() << serialPort->errorString();
    }

}

/******************************************************************************************************************/
/* Port Combo Box index changed slot; displays info for selected port when combo box is changed */
/******************************************************************************************************************/
void MainWindow::on_comboPort_currentIndexChanged(const QString &arg1)
{
    qDebug() << "MainWindow::on_comboPort_currentIndexChanged(" << arg1 << ");\r";
    // Dislplay info for selected port
    QSerialPortInfo selectedPort(arg1);
    ui->statusBar->showMessage(selectedPort.description());
}

/******************************************************************************************************************/
/* Connect Button clicked slot; handles connect and disconnect */
/******************************************************************************************************************/
void MainWindow::on_connectButton_clicked()
{
    qDebug() << "MainWindow::on_connectButton_clicked();\r";
    // If application is connected, disconnect
    if(connected)
    {
        serialPort->close();// Close serial port
        emit portClosed();// Notify application
        delete serialPort;// Delete the pointer
        serialPort = nullptr;// Assign NULL to dangling pointer
        ui->connectButton->setText("Connect");// Change Connect button text, to indicate disconnected
        ui->statusBar->showMessage("Disconnected!");// Show message in status bar
        connected = false;// Set connected status flag to false
        plotting = false;// Not plotting anymore
        receivedData.clear(); // Clear received string
        // Take care of controls
        ui->stopPlotButton->setEnabled(false);
        ui->saveJPGButton->setEnabled(false);
        enableControls(true);
    // If application is not connected, connect
    // Get parameters from controls first
    }
    else
    {
        QSerialPortInfo portInfo(ui->comboPort->currentText());// Temporary object, needed to create QSerialPort
        int baudRate = ui->comboBaud->currentText().toInt();// Get baud rate from combo box
        int dataBitsIndex = ui->comboData->currentIndex(); // Get index of data bits combo box
        int parityIndex = ui->comboParity->currentIndex();// Get index of parity combo box
        int stopBitsIndex = ui->comboStop->currentIndex();// Get index of stop bits combo box
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;

        // Set data bits according to the selected index
        if(dataBitsIndex == 0) {
            dataBits = QSerialPort::Data8;
        } else {
            dataBits = QSerialPort::Data7;
        }

        // Set parity according to the selected index
        if(parityIndex == 0) {
            parity = QSerialPort::NoParity;
        } else if(parityIndex == 1) {
            parity = QSerialPort::OddParity;
        } else {
            parity = QSerialPort::EvenParity;
        }

        // Set stop bits according to the selected index
        if(stopBitsIndex == 0) {
             stopBits = QSerialPort::OneStop;
        } else {
            stopBits = QSerialPort::TwoStop;
        }

        // Use local instance of QSerialPort; does not crash
        serialPort = new QSerialPort(portInfo, nullptr);
        // Open serial port and connect its signals
        openPort(portInfo, baudRate, dataBits, parity, stopBits);
    }
}

/******************************************************************************************************************/
/* Slot for port opened successfully */
/******************************************************************************************************************/
void MainWindow::portOpenedSuccess()
{
    qDebug() << "MainWindow::portOpenedSuccess();\r";
    //qDebug() << "Port opened signal received!";
    // Change buttons
    ui->connectButton->setText("Disconnect");
    ui->statusBar->showMessage("Connected!");
    // Disable controls if port is open
    enableControls(false);
    // Enable button for stopping plot
    ui->stopPlotButton->setText("Stop Plot");
    ui->stopPlotButton->setEnabled(true);
    // Enable button for saving plot
    ui->saveJPGButton->setEnabled(true);
    // Create the QCustomPlot area
 //   setupPlot();
    // Slot is refreshed n times per second
    updateTimer.start(updatePeriod);
    // Set flags
    connected = true;
    plotting = true;
}

/******************************************************************************************************************/
/* Slot for fail to open the port */
/******************************************************************************************************************/
void MainWindow::portOpenedFail()
{
    qDebug() << "MainWindow::portOpenedFail();\r";
    //qDebug() << "Port cannot be open signal received!";
    ui->statusBar->showMessage("Cannot open port!");
}

/******************************************************************************************************************/
/* Slot for closing the port */
/******************************************************************************************************************/
void MainWindow::onPortClosed()
{
    qDebug() << "MainWindow::onPortClosed();\r";
    //qDebug() << "Port closed signal received!";
    updateTimer.stop();
    connected = false;
    disconnect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
    // Disconnect port signals to GUI slots
    disconnect(this, SIGNAL(portOpenOK()), this, SLOT(portOpenedSuccess()));
    disconnect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    disconnect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
}

/******************************************************************************************************************/
/* Stop Plot Button */
/******************************************************************************************************************/
void MainWindow::on_stopPlotButton_clicked()
{
    qDebug() << "MainWindow::on_stopPlotButton_clicked();\r";
    // Stop plotting
    if(plotting) {
        // Stop updating plot timer
        updateTimer.stop();
        plotting = false;
        ui->stopPlotButton->setText("Start Plot");
    // Start plotting
    } else {
        // Start updating plot timer
        updateTimer.start(updatePeriod);
        plotting = true;
        ui->stopPlotButton->setText("Stop Plot");
    }
}

/******************************************************************************************************************/
/* Slot for spin box for plot minimum value on y axis */
/******************************************************************************************************************/
void MainWindow::on_spinAxesMin_valueChanged(int arg1)
{
    qDebug() << "MainWindow::on_spinAxesMin_valueChanged(" << arg1 << ");\r";
    ui->plot->yAxis->setRangeLower(arg1);
    ui->plot->replot();
}

/******************************************************************************************************************/
/* Slot for spin box for plot maximum value on y axis */
/******************************************************************************************************************/
void MainWindow::on_spinAxesMax_valueChanged(int arg1)
{
    qDebug() << "MainWindow::on_spinAxesMax_valueChanged(" << arg1 << ");\r";
    ui->plot->yAxis->setRangeUpper(arg1);
    ui->plot->replot();
}

/******************************************************************************************************************/
/* Number of axes combo; when changed, display axes colors in status bar */
/******************************************************************************************************************/
void MainWindow::on_comboAxes_valueChanged(int index)
{
    qDebug() << "MainWindow::on_comboAxes_valueChanged(" << index << ");\r";
    ui->statusBar->showMessage(QString::number(index) + "Axis");
    ////////////////////////////////////////////////////
    // Get number of axes from the user combo
    numberOfAxes = ui->comboAxes->value();
    qDebug() << "numberOfAxes(" << numberOfAxes << ");\r";
    //setupPlot();
}

/******************************************************************************************************************/
/* Spin box for changing the Y Tick step */
/******************************************************************************************************************/
void MainWindow::on_spinYStep_valueChanged(int arg1)
{
    qDebug() << "MainWindow::on_spinYStep_valueChanged(" << arg1 << ");\r";
    ui->plot->yAxis->setTickStep(arg1);
    ui->plot->replot();
}


/******************************************************************************************************************/
/* Save a JPG image of the plot to current EXE directory */
/******************************************************************************************************************/
void MainWindow::on_saveJPGButton_clicked()
{
    qDebug() << "MainWindow::on_saveJPGButton_clicked();\r";
    ui->plot->saveJpg(QString::number(dataPointNumber) + ".jpg");
}

void MainWindow::on_temperatureButton_clicked()
{
    numberOfAxes = 1;
    ui->plot->yAxis->setRange(2000, 4000);// NEW
    ui->plot->yAxis->setTickStep(500);
}
void MainWindow::on_humidityButton_clicked()
{
    numberOfAxes = 2;
     ui->plot->yAxis->setRange(10, 200);// NEW
     ui->plot->yAxis->setTickStep(25);
}
void MainWindow::on_gasButton_clicked()
{
    numberOfAxes = 3;
     ui->plot->yAxis->setRange(500, 1500);// NEW
     ui->plot->yAxis->setTickStep(100);
}
void MainWindow::on_dustButton_clicked()
{
    numberOfAxes = 4;
    ui->plot->yAxis->setRange(0, 50);// NEW!!!
    ui->plot->yAxis->setTickStep(2);
}
void MainWindow::on_allButton_clicked()
{
    numberOfAxes = 6;
    ui->plot->yAxis->setRange(0, 4000);// NEW
    ui->plot->yAxis->setTickStep(500);
}

/******************************************************************************************************************/
/* Reset the zoom of the plot to the initial values */
/******************************************************************************************************************/
void MainWindow::on_resetPlotButton_clicked()
{
    qDebug() << "MainWindow::on_resetPlotButton_clicked();\r";
    ui->plot->yAxis->setRange(0, 4095);
    ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
    ui->plot->yAxis->setTickStep(500);
    ui->plot->replot();
}

/******************************************************************************************************************/
/* Prints coordinates of mouse pointer in status bar on mouse release */
/******************************************************************************************************************/
void MainWindow::onMouseMoveInPlot(QMouseEvent *event)
{
    qDebug() << "MainWindow::onMouseMoveInPlot(" << event << ");\r";
    int xx = ui->plot->xAxis->pixelToCoord(event->x());
    int yy = ui->plot->yAxis->pixelToCoord(event->y());
    QString coordinates("X: %1 Y: %2");
    coordinates = coordinates.arg(xx).arg(yy);
    ui->statusBar->showMessage(coordinates);
}

/******************************************************************************************************************/
/* Spin box controls how many data points are collected and displayed */
/******************************************************************************************************************/
void MainWindow::on_spinPoints_valueChanged(int arg1)
{
    qDebug() << "MainWindow::on_spinPoints_valueChanged(" << arg1 << ");\r";
    NUMBER_OF_POINTS = arg1;
    ui->plot->replot();
}

/******************************************************************************************************************/
/* Shows a window with instructions */
/******************************************************************************************************************/
void MainWindow::on_actionHow_to_use_triggered()
{
    qDebug() << "MainWindow::on_actionHow_to_use_triggered();\r";
    helpWindow = new HelpWindow(this);
    helpWindow->setWindowTitle("How to use this application");
    helpWindow->show();
}
