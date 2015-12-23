#include "configcsdialog.h"
#include "ui_configcsdialog.h"
//#include "processdialog.h"
//#include "functions.h"

#include <QComboBox>
#include <QDebug>
#include <QString>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QTimer>
#include <QtConcurrentRun>

ConfigCSDialog::ConfigCSDialog(QWidget *parent, puhma::AttriConfig* const config,
        puhma::AttributeEmbed** ae) :
    QDialog(parent), config(config),ae(ae),
    ui(new Ui::ConfigCSDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_OKButton_triggered()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //load config into ui
    QString caliMethod = QString::fromStdString(config->calibrateMethod);
    int idx = -1;
    if(caliMethod == "cca"){
        idx = ui->CaliMethodCB->findText("CSR",
                static_cast<Qt::MatchFlags>(Qt::MatchExactly));
    }
    if(idx!=-1){
        ui->CaliMethodCB->setCurrentIndex(idx);
    }else{
        ui->CaliMethodCB->setCurrentIndex(0);
    }
    ui->DimCCASB->setValue(config->dimCCA);
}

ConfigCSDialog::~ConfigCSDialog()
{
    delete ui;
}

int ConfigCSDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    return QDialog::exec();
}

void ConfigCSDialog :: on_OKButton_triggered()
{
    config->discardThreshold = ui->DimCCASB->value();
    std::string caliM = ui->CaliMethodCB->currentText().toStdString();
    if(caliM == "CSR"){
        config->calibrateMethod = "cca";
    }
    else{
        config->calibrateMethod = "";
    }
    

    ProcessDialog *process_dialog = new ProcessDialog(this, &puhma::AttributeEmbed::calibrateNoEva, *ae);
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
