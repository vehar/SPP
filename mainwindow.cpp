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
    numberOfAxes(1),
    STATE(WAIT_START),
    NUMBER_OF_POINTS(500)
{
    qDebug() << "MainWindow(" << parent << ");\r";

    ui->setupUi(this);
    // Create the UI
    createUI();
    // Background for the plot area
    ui->plot->setBackground(QBrush(QColor(48,47,47)));
    // Setup plot area
    setupPlot();
    // Plot button is disabled initially
    ui->stopPlotButton->setEnabled(false);
    // used for higher performance (see QCustomPlot real time example)
    ui->plot->setNotAntialiasedElements(QCP::aeAll);
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plot->xAxis->setTickLabelFont(font);
    ui->plot->yAxis->setTickLabelFont(font);
    ui->plot->legend->setFont(font);

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
    serialPort = NULL;
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

      connect(ui->temperatureButton, SIGNAL(clicked()),this, SLOT(slotTextChange()));
     // connect(ui->humidityButton, SIGNAL(clicked(bool)), ui->temperatureButton, SLOT(setDisabled(bool)));
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Destructor */
/******************************************************************************************************************/
MainWindow::~MainWindow()
{
    qDebug() << "~MainWindow();\r";
    if(serialPort != NULL) delete serialPort;
    delete ui;
    writer.close();
    reader.close();
}
/******************************************************************************************************************/

void MainWindow::debugTimeOut(){
    qDebug() << "MainWindow::debugTimeOut();\r";
}
void MainWindow::slotStartStopReadFile(){
    static bool start = false;
    if(!start){
        ui->StartStopReadFileBtn->setText("Stop");
        timerReadFile.start();
    }
    else{
        ui->StartStopReadFileBtn->setText("Start");
        timerReadFile.stop();
    }
    start = !start;
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

void MainWindow::slotReadFile(){
    qDebug() << "MainWindow::slotReadFile();\r";
    QStringList newData;
    QString str;
    if(!reader.atEnd()){
        str = reader.readLine();
        qDebug() << str << ";\r";
        newData = str.split(",");
    }
    else{
        slotStartStopReadFile();
    }
    int dataListSize = newData.size();
    dataPointNumber++;
    for(int i = 0; i < numberOfAxes && dataListSize > i; ++i){
        ui->plot->graph(i)->addData(dataPointNumber, newData[i].toInt());
        ui->plot->graph(i)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
    }
    ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
    ui->plot->replot();
}

int cnt = 0;
void MainWindow::slotTextChange(){
qDebug() << "MainWindow::slotTextChange();\r";

QString msg;
msg += "cnt val ";
msg += msg.number(cnt);

cnt++;
ui->temperatureButton->setText(msg);
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
    // Select 9600 bits by default
    ui->comboBaud->setCurrentIndex(7);

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

    // Populate axes combo box;                 // 3 axes maximum allowed
    ui->comboAxes->setValue(1);
}
/******************************************************************************************************************/


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


/******************************************************************************************************************/
/* Setup the plot area */
/******************************************************************************************************************/
void MainWindow::setupPlot()
{
    qDebug() << "MainWindow::setupPlot();\r";
    // Remove everything from the plot
    ui->plot->clearItems();
    // Set tick step according to user spin box
    ui->plot->yAxis->setTickStep(ui->spinYStep->value());
    // Get number of axes from the user combo
    numberOfAxes = ui->comboAxes->value();
    // Set lower and upper plot range
    ui->plot->yAxis->setRange(ui->spinAxesMin->value(), ui->spinAxesMax->value());
    // Set x axis range for specified number of points
    ui->plot->xAxis->setRange(0, NUMBER_OF_POINTS);
    for(int i = 0; i < numberOfAxes; ++i){
        ui->plot->addGraph();
        switch(i%6){
        case 0:
            ui->plot->graph(i)->setPen(QPen(Qt::red));
            break;
        case 1:
            ui->plot->graph(i)->setPen(QPen(Qt::green));
            break;
        case 2:
            ui->plot->graph(i)->setPen(QPen(Qt::blue));
            break;
        case 3:
            ui->plot->graph(i)->setPen(QPen(Qt::cyan));
            break;
        case 4:
            ui->plot->graph(i)->setPen(QPen(Qt::magenta));
            break;
        case 5:
            ui->plot->graph(i)->setPen(QPen(Qt::yellow));
            break;
        }
    }

/*
    // If 1 axis selected
    if(numberOfAxes == 1) {
        // add Graph 0
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(Qt::red));
    // If 2 axes selected
    } else if(numberOfAxes == 2) {
        // add Graph 0
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(Qt::red));
        // add Graph 1
        ui->plot->addGraph();
        ui->plot->graph(1)->setPen(QPen(Qt::yellow));
    // If 3 axis selected
    } else if(numberOfAxes == 3) {
        // add Graph 0
        ui->plot->addGraph();
        ui->plot->graph(0)->setPen(QPen(Qt::red));
        // add Graph 1
        ui->plot->addGraph();
        ui->plot->graph(1)->setPen(QPen(Qt::yellow));
        // add Graph 2
        ui->plot->addGraph();
        ui->plot->graph(2)->setPen(QPen(Qt::green));
    }*/
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Open the inside serial port; connect its signals */
/******************************************************************************************************************/
void MainWindow::openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits)
{
    qDebug() << "MainWindow::openPort(portInfo, baudRate, dataBits, parity, stopBits);\r";
    // Create a new serial port
    serialPort = new QSerialPort(portInfo, 0);

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


/******************************************************************************************************************/
/* Connect Button clicked slot; handles connect and disconnect */
/******************************************************************************************************************/
void MainWindow::on_connectButton_clicked()
{
    qDebug() << "MainWindow::on_connectButton_clicked();\r";
    // If application is connected, disconnect
    if(connected) {
        // Close serial port
        serialPort->close();
        // Notify application
        emit portClosed();
        // Delete the pointer
        delete serialPort;
        // Assign NULL to dangling pointer
        serialPort = NULL;
        // Change Connect button text, to indicate disconnected
        ui->connectButton->setText("Connect");
        // Show message in status bar
        ui->statusBar->showMessage("Disconnected!");
        // Set connected status flag to false
        connected = false;
        // Not plotting anymore
        plotting = false;
        // Clear received string
        receivedData.clear();
        // Take care of controls
        ui->stopPlotButton->setEnabled(false);
        ui->saveJPGButton->setEnabled(false);
        enableControls(true);
    // If application is not connected, connect
                                                                                          // Get parameters from controls first
    } else {
        // Temporary object, needed to create QSerialPort
        QSerialPortInfo portInfo(ui->comboPort->currentText());
        // Get baud rate from combo box
        int baudRate = ui->comboBaud->currentText().toInt();
        // Get index of data bits combo box
        int dataBitsIndex = ui->comboData->currentIndex();
        // Get index of parity combo box
        int parityIndex = ui->comboParity->currentIndex();
        // Get index of stop bits combo box
        int stopBitsIndex = ui->comboStop->currentIndex();
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
        serialPort = new QSerialPort(portInfo, 0);
        // Open serial port and connect its signals
        openPort(portInfo, baudRate, dataBits, parity, stopBits);
    }
}
/******************************************************************************************************************/


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
    setupPlot();
    // Slot is refreshed 20 times per second
    updateTimer.start(20);
    // Set flags
    connected = true;
    plotting = true;
}
/******************************************************************************************************************/


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
/* Replot */
/******************************************************************************************************************/
void MainWindow::replot()
{
    qDebug() << "MainWindow::replot();\r";
    if(connected) {
        ui->plot->xAxis->setRange(dataPointNumber - NUMBER_OF_POINTS, dataPointNumber);
        ui->plot->replot();
    }
}
/******************************************************************************************************************/


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
        updateTimer.start();
        plotting = true;
        ui->stopPlotButton->setText("Stop Plot");
    }
}
/******************************************************************************************************************/


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
    if(plotting) {
        // Get size of received list
        int dataListSize = newData.size();
        // Increment data number
        dataPointNumber++;

// Add data to graphs according to number of axes
       /* for(int i = 0; i < numberOfAxes && dataListSize > i; ++i)
        {
            ui->plot->graph(i)->addData(dataPointNumber, newData[i].toInt());
            ui->plot->graph(i)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
        }*/
        switch(numberOfAxes)
            {
            case 1:
            {
                ui->plot->graph(0)->addData(dataPointNumber, newData[0].toInt());
                ui->plot->graph(0)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);

                ui->plot->graph(1)->addData(dataPointNumber, newData[1].toInt());
                ui->plot->graph(1)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            } break;

            case 2:
            {
                ui->plot->graph(1)->addData(dataPointNumber, newData[1].toInt());
                ui->plot->graph(1)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);

                ui->plot->graph(2)->addData(dataPointNumber, newData[2].toInt());
                ui->plot->graph(2)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            } break;

            case 3:
            {
                ui->plot->graph(0)->addData(dataPointNumber, newData[6].toInt());
                ui->plot->graph(0)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);

                ui->plot->graph(1)->addData(dataPointNumber, newData[4].toInt());
                ui->plot->graph(1)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);

                ui->plot->graph(2)->addData(dataPointNumber, newData[5].toInt());
                ui->plot->graph(2)->removeDataBefore(dataPointNumber - NUMBER_OF_POINTS);
            } break;

            default: break;
            }


    }
}
/******************************************************************************************************************/


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

void MainWindow::readDocData(){
    qDebug() << "MainWindow::readDocData();\r";
    // If any bytes are available
    if(serialPort->bytesAvailable()) {
        // Read all data in QByteArray
        QByteArray data = serialPort->readAll();

        // If the byte array is not empty
        if(!data.isEmpty()) {
            // Get a '\0'-terminated char* to the data
            char *temp = data.data();

            // Iterate over the char*
            for(int i = 0; temp[i] != '\0'; i++) {
                // Switch the current state of the message
                switch(STATE) {
                // If waiting for start [$], examine each char
                case WAIT_START:
                    // If the char is $, change STATE to IN_MESSAGE
                    if(temp[i] == START_MSG) {
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
                    if(temp[i] == END_MSG) {
                        STATE = WAIT_START;
                        // Split string received from port and put it into list
                        QStringList incomingData = receivedData.split(' ');
                        // Emit signal for data received with the list
                        emit newData(incomingData);
                        break;
                    }
                    // If examined char is a digit, and not '$' or ';', append it to temporary string
                    else if(isdigit(temp[i]) || isspace(temp[i]) ) {
                        receivedData.append(temp[i]);
                    }
                    break;
                default: break;
                }
            }
        }
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
        if(!data.isEmpty()) {
            // Get a '\0'-terminated char* to the data
            char *temp = data.data();

            // Iterate over the char*
            for(int i = 0; temp[i] != '\0'; i++) {
                // Switch the current state of the message
                switch(STATE) {
                // If waiting for start [$], examine each char
                case WAIT_START:
                    // If the char is $, change STATE to IN_MESSAGE
                    if(temp[i] == START_MSG) {
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
                    if(temp[i] == END_MSG) {
                        STATE = WAIT_START;
                        // Split string received from port and put it into list
                        QStringList incomingData = receivedData.split(' ');
                        // Emit signal for data received with the list
                        emit newData(incomingData);
                        break;
                    }
                    // If examined char is a digit, and not '$' or ';', append it to temporary string
                    else if(isdigit(temp[i]) || isspace(temp[i]) ) {
                        receivedData.append(temp[i]);
                    }
                    break;
                default: break;
                }
            }
        }
    }
}
/******************************************************************************************************************/


/******************************************************************************************************************/
/* Number of axes combo; when changed, display axes colors in status bar */
/******************************************************************************************************************/
void MainWindow::on_comboAxes_valueChanged(int index)
{
    qDebug() << "MainWindow::on_comboAxes_valueChanged(" << index << ");\r";
    ui->statusBar->showMessage(QString::number(index) + "Axis");
    ////////////////////////////////////////////////////
    setupPlot();

}
/******************************************************************************************************************/


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


/******************************************************************************************************************/
/* Save a JPG image of the plot to current EXE directory */
/******************************************************************************************************************/
void MainWindow::on_saveJPGButton_clicked()
{
    qDebug() << "MainWindow::on_saveJPGButton_clicked();\r";
    ui->plot->saveJpg(QString::number(dataPointNumber) + ".jpg");
}
/******************************************************************************************************************/


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
/******************************************************************************************************************/
