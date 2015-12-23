#ifndef PREPAREPROCESSPAGE_H
#define PREPAREPROCESSPAGE_H

#include <QWizardPage>
#include <QFutureWatcher>
#include <QCloseEvent>
#include "processthread.h"

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

namespace Ui {
class PrepareProcessPage;
}

class PrepareProcessPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrepareProcessPage(puhma::AttributeEmbed *ae,
                                void (puhma::AttributeEmbed::*f)(),
                                QWidget *parent = 0);
    ~PrepareProcessPage();

    bool isComplete() const;
    void initializePage();

public slots:
    void disableBack();
    //bool close();
    void rejectclicked();
    void updateStatusLabel();
    void killSC();

protected:
    //void closeEvent(QCloseEvent * event);

signals:
    //void rejectclicked();#
    void scKilled();

private:
    void (puhma::AttributeEmbed::*f)();
    Ui::PrepareProcessPage *ui;
    puhma::AttributeEmbed *ae;
    ProcessThread *pt;
    StatusChecker *sc;
    //QThread tup; //qthread to update label
    //QFutureWatcher<void> watcher;
};

#endif // PREPAREPROCESSPAGE_H
