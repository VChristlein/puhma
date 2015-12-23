#ifndef PREPARETYPEPAGE_H
#define PREPARETYPEPAGE_H

#include <QWizardPage>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif


namespace Ui {
class PrepareTypePage;
}

class PrepareTypePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareTypePage(puhma::AttributeEmbed *ae, QWidget *parent = 0);
    bool validatePage();
    ~PrepareTypePage();

private slots:
    void output_select();
    void input_select();

private:
    puhma::AttributeEmbed *ae;
    Ui::PrepareTypePage *ui;
};

#endif // PREPARETYPEPAGE_H
