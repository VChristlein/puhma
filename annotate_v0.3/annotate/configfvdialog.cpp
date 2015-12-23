#include "configfvdialog.h"
#include "ui_configfvdialog.h"
//#include "processdialog.h"
//#include "functions.h"

#include <QComboBox>
#include <QDebug>
#include <QSignalMapper>
#include <QString>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QTimer>
#include <QtConcurrentRun>

ConfigFVDialog::ConfigFVDialog(QWidget *parent, puhma::AttriConfig* const config,
        puhma::AttributeEmbed** ae) :
    QDialog(parent), config(config),ae(ae),
    ui(new Ui::ConfigFVDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_OKButton_triggered()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //load config into ui
    ui->NumTrainGMMSB->setValue(config->numWordsTranGMM);
    ui->ConfigOutput->setText(QString::fromStdString(config->outputdir));
    ui->ConfigInput->setText(QString::fromStdString(config->inputfolder));
    ui->ImagesHeightSB->setValue(config->heightIm);
    ui->DimPCASB->setValue(config->dimPCA);
    ui->ClusterGMMSB->setValue(config->clusterGMM);
    ui->NumSpatialXSB->setValue(config->numSpatialX);
    ui->NumSpatialYSB->setValue(config->numSpatialY);

}

ConfigFVDialog::~ConfigFVDialog()
{
    delete ui;
}

int ConfigFVDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    return QDialog::exec();
}

void ConfigFVDialog :: on_OKButton_triggered()
{
    config->numWordsTranGMM = ui->NumTrainGMMSB->value();
    config->outputdir = ui->ConfigOutput->text().toStdString();
    config->inputfolder = ui->ConfigInput->text().toStdString();
    config->heightIm = ui->ImagesHeightSB->value();
    config->dimPCA = ui->DimPCASB->value();
    config->clusterGMM = ui->ClusterGMMSB->value();
    config->numSpatialX = ui->NumSpatialXSB->value();
    config->numSpatialY = ui->NumSpatialYSB->value();

    //std::cout<<config->inputfolder<<std::endl;

    ProcessDialog *process_dialog = new ProcessDialog(this, &puhma::AttributeEmbed::fvRep, *ae);
    process_dialog->show();

    QFuture<void> future = QtConcurrent :: run(process_dialog, &ProcessDialog::process);
    while(!(future.isFinished())){
        sleep(1);
        //get the statues information from statistic
        //update the statusLabel
        QString status = QString::fromStdString((*ae)->getStatistic()->getStatus());
        process_dialog->updateStatusLabel2(status);
        QApplication::processEvents();
    }
    process_dialog->exec();
    delete process_dialog;
    this->accept();
}

void ConfigFVDialog :: output_folder_select()
{
    ui->ConfigOutput->setText(QFileDialog::getExistingDirectory(this, "Select Output Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}
void ConfigFVDialog :: input_folder_select()
{
    ui->ConfigInput->setText(QFileDialog::getExistingDirectory(this, "Select Input Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}

QString ConfigFVDialog :: updateStatus(ProcessDialog *pd)
{
    return "called";
}
