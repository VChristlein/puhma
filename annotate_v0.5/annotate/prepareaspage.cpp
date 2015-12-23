#include "prepareaspage.h"
#include "ui_prepareaspage.h"

#include <QTimer>
#include <QAbstractButton>

PrepareASPage::PrepareASPage(puhma::AttributeEmbed* ae, QWidget *parent) :
    QWizardPage(parent),ae(ae),
    ui(new Ui::PrepareASPage)
{
    ui->setupUi(this);
    ui->spin_threshold->setValue(ae->config.discardThreshold);
}

PrepareASPage::~PrepareASPage()
{
    delete ui;
}

void PrepareASPage::initializePage(){
    QTimer::singleShot(0,this,SLOT(disableBack()));
}

void PrepareASPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

bool PrepareASPage::validatePage(){
    ae->config.discardThreshold = ui->spin_threshold->value();
    ae->filePathManager->update();
    return true;
}
