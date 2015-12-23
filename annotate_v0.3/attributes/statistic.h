#pragma once

#include "attributes_config.h"

namespace puhma {

/*! \brief This Class gather all kind of double value data create by framework, 
 *          provide functions to print them on the screen or store them in file,
 *          notice the data collected only available at its life-time. 
 *          once its life-time ended, the data will lost.
 */
class Statistic
{
public:
    Statistic(const AttriConfig &c);
    ~Statistic(){};
    ///add data to key (at the end), if no key exist, create one
    //void addData(const std::string k, const double d);
    //void addData(const std::string k, const std::list<double> &d); 
    ///add describe to one key for print, if no key exist, create one
    //void addDesc(const std::string k, const std::string desc);
    ///print data on the screen
    //void printData(const std::string k) const;
    ///change the status
    void setStatus(const std::string &s){
        status = s;
    }
    const std::string getStatus() const{
        return status;
    }
    ///dump data
    void dumpData(const std::string &k) const;
    //void dumpData(const std::vector<std::string> ks, const std::string fp) const;
    std::map<std::string, std::vector<double> > data;
    std::map<std::string, std::string > datadesc;
private:
    const AttriConfig config;
    std::string status;
};

}
