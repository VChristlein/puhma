#include "preparecspage.h"
#include "ui_preparecspage.h"

PrepareCSPage::PrepareCSPage(puhma::AttributeEmbed* ae, QWidget *parent) :
    QWizardPage(parent), ae(ae),
    ui(new Ui::PrepareCSPage)
{
    ui->setupUi(this);
    ae->config.calibrateMethod="cca";
    ui->spin_dimcca->setValue(ae->config.dimCCA);
}

PrepareCSPage::~PrepareCSPage()
{
    delete ui;
}

void PrepareCSPage::initializePage(){
    QTimer::singleShot(0,this,SLOT(disableBack()));
}

void PrepareCSPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

bool PrepareCSPage::validatePage(){
    ae->config.calibrateMethod="cca";
    ae->config.discardThreshold = ui->spin_dimcca->value();
    ae->filePathManager->update();
    return true;
}
