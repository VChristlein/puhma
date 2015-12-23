#include "processdialog.h"
#include "ui_processdialog.h"
//#include "functions.h"

#include <QDebug>
#include <QSignalMapper>
#include <QString>
#include <QTimer>

ProcessDialog::ProcessDialog(  QWidget *parent, void (puhma::AttributeEmbed::*f)(),
        puhma::AttributeEmbed *ae) :
    QDialog(parent), f(f), ae(ae),
    ui(new Ui::ProcessDialog)
{
    ui->setupUi(this);
    ui->StatusLabel->setText("Processing...");
}

ProcessDialog::~ProcessDialog()
{
    delete ui;
}

void ProcessDialog :: process(){
    //ui->StatusLabel->setText("changed");
    ui->ConfirmButton->blockSignals(true);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    (ae->*f)();

    ui->StatusLabel->setText("finished");
    QApplication::restoreOverrideCursor();
    ui->ConfirmButton->blockSignals(false);
}

void ProcessDialog :: updateStatusLabel2(QString st){
    ui->StatusLabel_2->setText(st);
}

/*void ProcessDialog :: upLabel(QString st){
    ui->StatusLabel->setText(st);
}*/
