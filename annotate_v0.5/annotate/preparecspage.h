#ifndef PREPARECSPAGE_H
#define PREPARECSPAGE_H

#include <QWizardPage>
#include <QTimer>
#include <QAbstractButton>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareCSPage;
}

class PrepareCSPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareCSPage(puhma::AttributeEmbed* ae, QWidget *parent = 0);
    ~PrepareCSPage();
    void initializePage();
    bool validatePage();

public slots:
    void disableBack();

private:
    puhma::AttributeEmbed *ae;
    Ui::PrepareCSPage *ui;
};

#endif // PREPARECSPAGE_H
