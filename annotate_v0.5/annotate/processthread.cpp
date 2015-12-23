#include "processthread.h"

#include <QDebug>

ProcessThread::ProcessThread(puhma::AttributeEmbed *ae,
                             void (puhma::AttributeEmbed::*f)()):
    ae(ae),f(f)
{
}

void ProcessThread::run(){
    (ae->*f)();
    //exec();
}

StatusChecker::StatusChecker(puhma::AttributeEmbed *ae, ProcessThread *pt):
    ae(ae),pt(pt)
{
}

void StatusChecker::run(){
    while(!pt->isFinished()){
        emit doUp();
        sleep(1);
    }
}
