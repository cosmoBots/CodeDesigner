#ifndef udCPPClassProjectGenerator_H
#define udCPPClassProjectGenerator_H

#include "codegen/base/ProjectGenerator.h"


class udCPPClassProjectGenerator : public udProjectGenerator
{
public:
    DECLARE_DYNAMIC_CLASS(udCPPClassProjectGenerator);

    udCPPClassProjectGenerator();
    virtual ~udCPPClassProjectGenerator();

protected:
    // protected virtual functions
    virtual void Initialize();
    virtual void ProcessProject(udProject *src);
    virtual void CleanUp();
};



#endif // udCPPClassProjectGenerator_H
