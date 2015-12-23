#include "preparedatasetpage.h"
#include "ui_preparedatasetpage.h"

#include<QTimer>
#include<QAbstractButton>
#include<QDebug>
#include<QString>
#include<QLabel>
#include<QLineEdit>
#include<QPushButton>
#include <QFileDialog>
#include <QErrorMessage>

PrepareDatasetPage::PrepareDatasetPage(puhma::AttributeEmbed *ae, QWidget *parent) :
    QWizardPage(parent), ae(ae),
    ui(new Ui::PrepareDatasetPage)
{
    ui->setupUi(this);
}

void PrepareDatasetPage::initializePage(){
    if(ae->config.dataset == "IAM"){
        //need transcription file
        QLabel *label_trans = new QLabel("Transcrip. Path",this);
        lineEdit_trans = new QLineEdit();
        ui->formLayout->addRow(label_trans,lineEdit_trans);
        QPushButton *button_trans = new QPushButton("Select");
        ui->formLayout->addRow(nullptr,button_trans);
        lineEdit_trans->setText(
                    QString::fromStdString(ae->config.transcriptionfile));
        connect(button_trans, SIGNAL(clicked()), this, SLOT(trans_select()));

        lineEdit_trainDiv = new QLineEdit();
        ui->formLayout->addRow("Train Div. Path", lineEdit_trainDiv);
        QPushButton *button_trainDiv = new QPushButton("Select");
        ui->formLayout->addRow(nullptr,button_trainDiv);
        lineEdit_trainDiv->setText(
                    QString::fromStdString(ae->config.divisionTrainfile));
        connect(button_trainDiv, SIGNAL(clicked()), this, SLOT(train_select()));

        lineEdit_testDiv = new QLineEdit();
        ui->formLayout->addRow("Test Div. Path", lineEdit_testDiv);
        QPushButton *button_testDiv = new QPushButton("Select");
        ui->formLayout->addRow(nullptr,button_testDiv);
        lineEdit_testDiv->setText(
                    QString::fromStdString(ae->config.divisionTestfile));
        connect(button_testDiv, SIGNAL(clicked()), this, SLOT(test_select()));

        lineEdit_valiDiv = new QLineEdit();
        ui->formLayout->addRow("Vali. Div. Path", lineEdit_valiDiv);
        QPushButton *button_valiDiv = new QPushButton("Select");
        ui->formLayout->addRow(nullptr,button_valiDiv);
        lineEdit_valiDiv->setText(QString::fromStdString(
                                      ae->config.divisionValifile));
        connect(button_valiDiv, SIGNAL(clicked()), this, SLOT(vali_select()));

        ui->verticalLayout->addStretch();

    }
    /*ui->lineEdit_input->setText(QString::fromStdString(
                                    ae->config.inputfolder));
    connect(ui->btn_input, SIGNAL(clicked()), this, SLOT(input_select()));*/
    QTimer::singleShot(0,this,SLOT(disableBack()));


}

void PrepareDatasetPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

/*void PrepareDatasetPage::input_select()
{
    ui->lineEdit_input->setText(QFileDialog::getExistingDirectory(
                    this, "Select Input Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}*/

void PrepareDatasetPage::trans_select()
{

    lineEdit_trans->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}

void PrepareDatasetPage::train_select()
{
    lineEdit_trainDiv->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}

void PrepareDatasetPage::test_select()
{
    lineEdit_testDiv->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}

void PrepareDatasetPage::vali_select()
{
    lineEdit_valiDiv->setText(QFileDialog::getOpenFileName(this, "Select File", "./", "Text (*.txt)"));
}

bool PrepareDatasetPage::validatePage()
{
    /*if(ui->lineEdit_input->text().trimmed() == ""){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please choose a input folder");
        return false;
    }else */if(ae->config.dataset == "IAM" &&
             lineEdit_trans->text().trimmed() == ""){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please choose a ground truth file");
        return false;
    }
    /*ae->config.inputfolder = ui->lineEdit_input->text().trimmed().toStdString();*/

    if(ae->config.dataset == "IAM"){
        ae->config.transcriptionfile =
                lineEdit_trans->text().trimmed().toStdString();
        ae->config.divisionTrainfile =
                lineEdit_trainDiv->text().trimmed().toStdString();
        ae->config.divisionTestfile =
                lineEdit_testDiv->text().trimmed().toStdString();
        ae->config.divisionValifile =
                lineEdit_valiDiv->text().trimmed().toStdString();
    }

    //update the path
    ae->filePathManager->update();
    return true;
}

PrepareDatasetPage::~PrepareDatasetPage()
{
    delete ui;
}
