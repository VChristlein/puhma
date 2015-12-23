#ifndef PREPAREPHOCPAGE_H
#define PREPAREPHOCPAGE_H

#include <QWizardPage>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PreparePhocPage;
}

class PreparePhocPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PreparePhocPage(puhma::AttributeEmbed *ae, QWidget *parent = 0);
    ~PreparePhocPage();
    void initializePage();
    bool validatePage();

public slots:
    void disableBack();

private:
    puhma::AttributeEmbed *ae;
    Ui::PreparePhocPage *ui;
};

#endif // PREPAREPHOCPAGE_H
