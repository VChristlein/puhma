#ifndef PREPAREDATASETPAGE_H
#define PREPAREDATASETPAGE_H

#include <QWizardPage>
#include<QLineEdit>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareDatasetPage;
}

class PrepareDatasetPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareDatasetPage(puhma::AttributeEmbed *ae, QWidget *parent = 0);
    ~PrepareDatasetPage();
    void initializePage();
    bool validatePage();

public slots:
    void disableBack();
    //void input_select();
    void trans_select();
    void train_select();
    void test_select();
    void vali_select();

private:
    puhma::AttributeEmbed *ae;
    Ui::PrepareDatasetPage *ui;
    QLineEdit *lineEdit_trans;
    QLineEdit *lineEdit_trainDiv;
    QLineEdit *lineEdit_testDiv;
    QLineEdit *lineEdit_valiDiv;
};

#endif // PREPAREDATASETPAGE_H
