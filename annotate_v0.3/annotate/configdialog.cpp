#include "configdialog.h"
#include "ui_configdialog.h"
#include "processdialog.h"
//#include "functions.h"

#include <QComboBox>
#include <QDebug>
#include <QSignalMapper>
#include <QString>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QTimer>

ConfigDialog::ConfigDialog(QWidget *parent, puhma::AttriConfig* const config,
        puhma::AttributeEmbed** ae) :
    QDialog(parent), config(config),ae(ae),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_OKButton_triggered()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //load config into ui
    QString datasetType = QString::fromStdString(config->dataset);
    int idx = ui->DataSetTypeCB->findText(datasetType, 
            static_cast<Qt::MatchFlags>(Qt::MatchExactly));
    if(idx!=-1){
        ui->DataSetTypeCB->setCurrentIndex(idx);
    }else{
        ui->DataSetTypeCB->addItem(datasetType);
        ui->DataSetTypeCB->setCurrentIndex(ui->DataSetTypeCB->count());
    }

    ui->ConfigOutput->setText(QString::fromStdString(config->outputdir));
    ui->ConfigInput->setText(QString::fromStdString(config->inputfolder));
    ui->ConfigGT->setText(QString::fromStdString(config->transcriptionfile));
    if(config->dataset == "PUHMA"){
        ui->ConfigGT->setReadOnly(true);
        ui->GtFileSelector->setEnabled(false);
    }

    ui->PHOCDigitTrue->setChecked(config->digitalInPHOC);
    ui->ImagesHeightSB->setValue(config->heightIm);
    ui->ConfigTrainSet->setText(QString::fromStdString(config->divisionTrainfile));
    ui->ConfigTestSet->setText(QString::fromStdString(config->divisionTestfile));
    ui->ConfigValiSet->setText(QString::fromStdString(config->divisionValifile));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

int ConfigDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    return QDialog::exec();
}

void ConfigDialog :: on_OKButton_triggered()
{
    config->dataset = ui->DataSetTypeCB->currentText().toStdString();
    config->outputdir = ui->ConfigOutput->text().toStdString();
    config->inputfolder = ui->ConfigInput->text().toStdString();
    config->transcriptionfile = ui->ConfigGT->text().toStdString();
    config->digitalInPHOC = ui->PHOCDigitTrue->isChecked();
    config->heightIm = ui->ImagesHeightSB->value();
    config->divisionTrainfile = ui->ConfigTrainSet->text().toStdString();
    config->divisionTestfile = ui->ConfigTestSet->text().toStdString();
    config->divisionValifile = ui->ConfigValiSet->text().toStdString();
    if(*ae == nullptr){
        *ae = new puhma::AttributeEmbed(*config);
    }

    std::cout<<"address:"<<config<<std::endl;
    std::cout<<"address:"<<&((*ae)->config)<<std::endl;

    ProcessDialog *process_dialog = new ProcessDialog(this, &puhma::AttributeEmbed::prepare, *ae);

    process_dialog->show();
    QTimer::singleShot(0,process_dialog,SLOT(process()));
    process_dialog->exec();
    delete process_dialog;
    this->accept();
}
void ConfigDialog :: dataset_type_changed()
{
    if(ui->DataSetTypeCB->currentText() == "PUHMA"){
        ui->ConfigGT->setText("");
        ui->ConfigGT->setReadOnly(true);
        ui->GtFileSelector->setEnabled(false);
    }else{
        ui->ConfigGT->setReadOnly(false);
        ui->GtFileSelector->setEnabled(true);
    }
}
void ConfigDialog :: output_folder_select()
{
    ui->ConfigOutput->setText(QFileDialog::getExistingDirectory(this, "Select Output Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}
void ConfigDialog :: input_folder_select()
{
    ui->ConfigInput->setText(QFileDialog::getExistingDirectory(this, "Select Input Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}
void ConfigDialog :: gt_file_select()
{
    ui->ConfigGT->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}
void ConfigDialog :: trainset_select()
{
    ui->ConfigTrainSet->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}
void ConfigDialog :: valiset_select()
{
    ui->ConfigValiSet->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}
void ConfigDialog :: testset_select()
{
    ui->ConfigTestSet->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}
