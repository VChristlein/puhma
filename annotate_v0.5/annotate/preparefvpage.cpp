#include "preparefvpage.h"
#include "ui_preparefvpage.h"

#include <QErrorMessage>
#include <QTimer>
#include <QFileDialog>
#include <QAbstractButton>

PrepareFVPage::PrepareFVPage(puhma::AttributeEmbed* ae, QWidget *parent) :
    QWizardPage(parent),ae(ae),
    ui(new Ui::PrepareFVPage)
{
    ui->setupUi(this);
    ui->spin_gmmnumber->setValue(ae->config.numWordsTranGMM);
    ui->spin_imageheight->setValue(ae->config.heightIm);
    ui->spin_pcadim->setValue(ae->config.dimPCA);
    ui->spin_gmmcluster->setValue(ae->config.clusterGMM);
    ui->spin_spatialxnum->setValue(ae->config.numSpatialX);
    ui->spin_spatialynum->setValue(ae->config.numSpatialY);

    /*ui->lineEdit_input->setText(QString::fromStdString(ae->config.inputfolder));
    connect(ui->btn_input, SIGNAL(clicked()),this, SLOT(input_folder_select()));*/
}

PrepareFVPage::~PrepareFVPage()
{
    delete ui;
}

void PrepareFVPage::initializePage(){
    QTimer::singleShot(0,this,SLOT(disableBack()));
}

void PrepareFVPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

bool PrepareFVPage::validatePage(){
    /*if(ui->lineEdit_input->text().trimmed() == ""){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please choose a input folder");
        return false;
    }
    ae->config.inputfolder = ui->lineEdit_input->text().trimmed().toStdString();*/
    ae->config.numWordsTranGMM = ui->spin_gmmnumber->value();
    ae->config.heightIm = ui->spin_imageheight->value();
    ae->config.dimPCA = ui->spin_pcadim->value();
    ae->config.clusterGMM = ui->spin_gmmcluster->value();
    ae->config.numSpatialX = ui->spin_spatialxnum->value();
    ae->config.numSpatialY = ui->spin_spatialynum->value();
    ae->filePathManager->update();
    return true;
}

/*void PrepareFVPage :: input_folder_select()
{
    ui->lineEdit_input->setText(QFileDialog::getExistingDirectory(this, "Select Input Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}*/
