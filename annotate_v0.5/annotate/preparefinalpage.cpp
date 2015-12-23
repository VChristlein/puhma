#include "preparefinalpage.h"
#include "ui_preparefinalpage.h"

#include<QTimer>
#include<QAbstractButton>

PrepareFinalPage::PrepareFinalPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::PrepareFinalPage)
{
    ui->setupUi(this);
}

void PrepareFinalPage::initializePage(){
    QTimer::singleShot(0,this,SLOT(disableBack()));
}

void PrepareFinalPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

PrepareFinalPage::~PrepareFinalPage()
{
    delete ui;
}
