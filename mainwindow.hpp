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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QTextDocumentWriter>
#include <QFile>

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QPen>
#include "helpwindow.hpp"

#define START_MSG       '$'
#define END_MSG         ';'

#define WAIT_START      1
#define IN_MESSAGE      2
#define UNDEFINED       3

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    // Slot displays message on status bar
    void on_comboPort_currentIndexChanged(const QString &arg1);
    // Manages connect/disconnect
    void on_connectButton_clicked();
    // Called when port opens OK
    void portOpenedSuccess();
    // Called when port fails to open
    void portOpenedFail();
    // Called when closing the port
    void onPortClosed();
    // Slot for repainting the plot
    void replot();
    // Starts and stops plotting
    void on_stopPlotButton_clicked();
    // Slot for new data from serial port
    void onNewDataArrived(QStringList newData);
    // Changing lower limit for the plot
    void on_spinAxesMin_valueChanged(int arg1);
    // Changing upper limit for the plot
    void on_spinAxesMax_valueChanged(int arg1);
    // Slot for inside serial port
    void readData();
    // Display number of axes and colors in status bar
    void on_comboAxes_valueChanged(int index);
    // Spin box for changing Y axis tick step
    void on_spinYStep_valueChanged(int arg1);
    // Button for saving JPG
    void on_saveJPGButton_clicked();
    // Resets plot to initial zoom and coordinates
    void on_resetPlotButton_clicked();
    // Displays coordinates of mouse pointer when clicked in plot in status bar
    void onMouseMoveInPlot(QMouseEvent *event);
    // Spin box controls how many data points are collected and displayed
    void on_spinPoints_valueChanged(int arg1);
    void on_actionHow_to_use_triggered();

    //just for debug
    void debugTimeOut();

    void readDocData();

    void on_temperatureButton_clicked();
    void on_humidityButton_clicked();
    void on_gasButton_clicked();
    void on_dustButton_clicked();

signals:
    // Emitted when cannot open port
    void portOpenFail();
    // Emitted when port is open
    void portOpenOK();
    // Emitted when port is closed
    void portClosed();
    // Emitted when new data has arrived
    void newData(QStringList data);

private:
    Ui::MainWindow *ui;

    QFile writer;
    QFile reader;

    // Status connection variable
    bool connected;
    // Status plotting variable
    bool plotting;
    // Keep track of data points
    int dataPointNumber;
    // Timer used for replotting the plot
    QTimer updateTimer;
    // Number of axes for the plot
    int numberOfAxes;
    // Record the time of the first data point
    QTime timeOfFirstData;
    // Store time between samples
    double timeBetweenSamples;
    // Serial port; runs in this thread
    QSerialPort *serialPort;
    // Used for reading from the port
    QString receivedData;
    // State of recieiving message from port
    int STATE;
    // Number of points plotted
    int NUMBER_OF_POINTS;
    HelpWindow *helpWindow;

    // Populate the controls
    void createUI();
    // Enable/disable controls
    void enableControls(bool enable);
    // Setup the QCustomPlot
    void setupPlot();
                                                                                          // Open the inside serial port with these parameters
    void openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits);

    void drawPoint(QStringList newData, int axesNum, int startData, int size);
    ///////////////////////////////////////////////////////////////
    QTimer timerReadFile;
private slots:

    void slotOpenFile();
    void slotReadFile();
    void slotStartStopReadFile();
    void slotTextChange(int num);
};


#endif                                                                                    // MAINWINDOW_HPP
