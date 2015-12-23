#include "statistic.h"
#include "util.h"

#include <QDebug>
#include <QDir>

namespace puhma{

Statistic :: Statistic(const AttriConfig &c)
    :config(c)
{
    status = "";
}

void Statistic :: dumpData(const std::string &k) const
{
    auto itdata = data.find(k);
    if(itdata != data.end()){
        std::string fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir)),
                    QString::fromStdString(k+".txt")).filePath()).toStdString();
        Util::dumpVector(fn,itdata->second);
    }else{
        qDebug()<<"Statistic:: key do not exists.";
    }
}

}
