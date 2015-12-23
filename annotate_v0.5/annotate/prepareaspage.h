#ifndef PREPAREASPAGE_H
#define PREPAREASPAGE_H

#include <QWizardPage>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareASPage;
}

class PrepareASPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareASPage(puhma::AttributeEmbed* ae, QWidget *parent = 0);
    ~PrepareASPage();
    void initializePage();
    bool validatePage();

public slots:
    void disableBack();

private:
    puhma::AttributeEmbed *ae;

private:
    Ui::PrepareASPage *ui;
};

#endif // PREPAREASPAGE_H
