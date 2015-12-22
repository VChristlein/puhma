#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextStream>
#include <QtGui>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    index = 0;
//    this->resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    setCentralWidget(ui->scrollArea);
    log_file = NULL;
}

MainWindow::~MainWindow()
{
    if ( log_file ) {
        log_file->close();
        delete log_file;
    }
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void MainWindow::showCurrentImage() {
    qDebug() << index << files.size();
    if ( index >= files.size() ) {
        log_file->close();
        this->close();
        return;
    }
    QImage image(files.at(index));
    ui->label_pic->setPixmap(QPixmap::fromImage(image));
    ui->label_pic->adjustSize();

    ui->label_progress->setText(QString::number(index+1) + " / "
                                + QString::number(files.size()));
}

void MainWindow::on_pushButtonBrowse_clicked()
{
    // ask the user to add files
    files = QFileDialog::getOpenFileNames(
                this,
                "Process Image Files",
                "", // last added dir
                "Images (*tif *.png *.jpg)");

    if ( files.size() <= 0 )
        return;

    ui->label_imgpath->setText( QFileInfo(files.at(0)).absolutePath() );
    showCurrentImage();
}

void MainWindow::on_pushButton_logFile_clicked()
{
    if ( log_file ) {
        if ( log_file->isOpen()) {
            log_file->close();
        }
        delete log_file;
    }
    QString log = QFileDialog::getOpenFileName(this, "Fail-Log");
    log_file = new QFile(log);
    log_file->open(QIODevice::Append | QIODevice::Text);
    ui->label_log->setText(log);
}

void MainWindow::on_pushButton_ok_clicked()
{
    if (!log_file || !log_file->isOpen()){
        on_pushButton_logFile_clicked();
    }
    QTextStream out(log_file);
    out << files.at(index) << " 0\n";

    index++;
    showCurrentImage();
}

void MainWindow::on_pushButton_fail_clicked()
{
    if (!log_file || !log_file->isOpen()){
        on_pushButton_logFile_clicked();
    }
    QTextStream out(log_file);
    out << files.at(index) << " 1\n";

    index++;
    showCurrentImage();
}


void MainWindow::on_pushButton_notPerfect_clicked()
{
    if (!log_file || !log_file->isOpen()){
        on_pushButton_logFile_clicked();
    }
    QTextStream out(log_file);
    out << files.at(index) << " 2\n";

    index++;
    showCurrentImage();
}
