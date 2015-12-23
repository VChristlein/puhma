#ifndef PROCESSTHREAD_H
#define PROCESSTHREAD_H

#include <QThread>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#endif

class ProcessThread : public QThread
{
    Q_OBJECT

public:
    ProcessThread(puhma::AttributeEmbed *ae,
                  void (puhma::AttributeEmbed::*f)());
private:
    void run();
    puhma::AttributeEmbed *ae;
    void (puhma::AttributeEmbed::*f)();

};

class StatusChecker : public QThread
{
    Q_OBJECT
public:
    StatusChecker(puhma::AttributeEmbed *ae, ProcessThread *pt);
private:
    void run();
    puhma::AttributeEmbed *ae;
    ProcessThread *pt;
signals:
    void doUp();
};

#endif // PROCESSTHREAD_H
