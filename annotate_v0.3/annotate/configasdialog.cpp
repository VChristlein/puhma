#include "configasdialog.h"
#include "ui_configasdialog.h"
//#include "processdialog.h"
//#include "functions.h"

#include <QComboBox>
#include <QDebug>
#include <QString>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QTimer>
#include <QtConcurrentRun>

ConfigASDialog::ConfigASDialog(QWidget *parent, puhma::AttriConfig* const config,
        puhma::AttributeEmbed** ae) :
    QDialog(parent), config(config),ae(ae),
    ui(new Ui::ConfigASDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_OKButton_triggered()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //load config into ui
    ui->ThresholdRateSB->setValue(config->discardThreshold);
}

ConfigASDialog::~ConfigASDialog()
{
    delete ui;
}

int ConfigASDialog :: exec()
{    
    layout()->setSizeConstraint(QLayout::SetMinimumSize);

    return QDialog::exec();
}

void ConfigASDialog :: on_OKButton_triggered()
{
    config->discardThreshold = ui->ThresholdRateSB->value();

    ProcessDialog *process_dialog = new ProcessDialog(this, &puhma::AttributeEmbed::trainAS, *ae);
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
