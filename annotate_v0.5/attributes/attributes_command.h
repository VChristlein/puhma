#pragma once

#include <command.h>

#include "attributes_config.h"

#include <opencv2/core/core.hpp>

namespace puhma {

class AttributesCommand : public vole::Command
{
public:
    AttributesCommand(void);
    ~AttributesCommand(void) {}

    int execute(void); 

    void printShortHelp(void) const;

    void printHelp(void) const;

    void printConfig(void);

    AttriConfig config;
};

}
