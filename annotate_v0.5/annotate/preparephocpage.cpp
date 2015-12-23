#include "preparephocpage.h"
#include "ui_preparephocpage.h"

#include <QTimer>
#include <QAbstractButton>

PreparePhocPage::PreparePhocPage(puhma::AttributeEmbed *ae, QWidget *parent) :
    QWizardPage(parent),ae(ae),
    ui(new Ui::PreparePhocPage)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(ae->config.digitalInPHOC);
}

void PreparePhocPage::initializePage(){
    QTimer::singleShot(0,this,SLOT(disableBack()));
}

bool PreparePhocPage::validatePage(){
    ae->config.digitalInPHOC = ui->checkBox->isChecked();
    ae->filePathManager->update();
    return true;
}

void PreparePhocPage::disableBack(){
    wizard()->button(QWizard::BackButton)->setEnabled(false);
}

PreparePhocPage::~PreparePhocPage()
{
    delete ui;
}
