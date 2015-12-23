#pragma once

#include "attributes_model.h"
#include "trans_phoc.h"
#include "attributes_config.h"
#include "util.h"

namespace puhma{

class Calibrate{
public:
    Calibrate(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp);
    virtual void compute()=0;
    virtual void evaluate()=0;
protected:
    TransPhoc *tp;
    const AttriConfig config;
    const FileHandler *filePathManager;
    std::vector<std::vector<std::string> > dataTrain;
    std::vector<std::vector<std::string> > dataValidation;
    std::vector<std::vector<std::string> > dataTest;
};

typedef Calibrate* (*initalCaliFn)(const AttriConfig &, const FileHandler *, const DataPrepare *);

class CSR: public Calibrate{
public:
    CSR(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp):Calibrate(c,fpm,dp){};
    void compute();
    void evaluate();

    static Calibrate * initialize(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp){
        return new CSR(c, fpm, dp); 
    };
protected:
    Embedding embedding;
};

class Platt: public Calibrate{
public:
    Platt(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp):Calibrate(c,fpm,dp){};
    void compute();
    void evaluate();

    static Calibrate * initialize(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp) {
        return new Platt(c, fpm, dp); 
    };
private:
    ///rows:2 row1->A  row2->B
    cv::Mat1d platts;
};

class Direct: public Calibrate{
public:
    Direct(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp):Calibrate(c,fpm,dp){};
    void compute(){};//do not need to compute calibrate here.
    void evaluate();

    static Calibrate * initialize(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp) {
        return new Direct(c, fpm, dp); 
    };
};

///factory for calibrate functions. singleton pattern here.
class CalibrateFac{
private:
    CalibrateFac();
    CalibrateFac(const CalibrateFac &) { }
    CalibrateFac &operator=(const CalibrateFac &) { return *this; }

    typedef std::map<std::string, initalCaliFn> FactoryMap;
    FactoryMap m_FactoryMap;
public:
    ~CalibrateFac() { m_FactoryMap.clear(); }

    static CalibrateFac *get()
    {
        static CalibrateFac instance;
        return &instance;
    }

    void registerCalimethod(const std::string &calimethod, initalCaliFn pfnIntial);
    Calibrate *initalCali(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp);
};

}
