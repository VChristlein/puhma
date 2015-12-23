#include "preparetypepage.h"
#include "ui_preparetypepage.h"

#include <QFileDialog>
#include <QDebug>
#include <QErrorMessage>

#define USER_DEFINE "User defined"

PrepareTypePage::PrepareTypePage(puhma::AttributeEmbed *ae, QWidget *parent) :
    QWizardPage(parent), ae(ae),
    ui(new Ui::PrepareTypePage)
{
    ui->setupUi(this);


    QString datasetType = QString::fromStdString(ae->config.dataset);
    int idx = ui->datatype_combo->findText(datasetType,
            static_cast<Qt::MatchFlags>(Qt::MatchExactly));
    if(idx!=-1){
        ui->datatype_combo->setCurrentIndex(idx);
    }else{
        idx = ui->datatype_combo->findText(USER_DEFINE,
                                           static_cast<Qt::MatchFlags>(Qt::MatchExactly));
        ui->datatype_combo->setCurrentIndex(idx);
    }

    ui->output_line->setText(QString::fromStdString(
                                 ae->config.outputdir));
    ui->input_line->setText(QString::fromStdString(
                                ae->config.inputfolder));

    connect(ui->output_btn, SIGNAL(clicked()), this, SLOT(output_select()));
    connect(ui->input_btn, SIGNAL(clicked()),this,SLOT(input_select()));
}

bool PrepareTypePage::validatePage()
{
    if(ui->input_line->text().trimmed() == ""){
            QErrorMessage * error = new QErrorMessage(this);
            error->showMessage("Please choose a input folder");
            return false;
    }else if(ui->datatype_combo->currentIndex() == 0){
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Please choose a type");
        return false;
    }else{
        QString dt = ui->datatype_combo->currentText();
        if(dt == USER_DEFINE){
            dt = "USER";
        }
        ae->config.dataset = dt.toStdString();
    }
    ae->config.outputdir = ui->output_line->text().toStdString();
    ae->config.inputfolder = ui->input_line->text().toStdString();

    //update the path
    ae->filePathManager->update();
    return true;
}

void PrepareTypePage::output_select()
{
    ui->output_line->setText(QFileDialog::getExistingDirectory(
                    this, "Select Output Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}

void PrepareTypePage::input_select()
{
    ui->input_line->setText(QFileDialog::getExistingDirectory(
                    this, "Select Input Folder", "./", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks));
}

PrepareTypePage::~PrepareTypePage()
{
    delete ui;
}
