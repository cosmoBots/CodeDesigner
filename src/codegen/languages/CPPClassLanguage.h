#ifndef udCPPClassLanguage_H
#define udCPPClassLanguage_H

#include "codegen/languages/CLanguage.h"

class udCPPClassLanguage : public udCLanguage
{
public:
    DECLARE_DYNAMIC_CLASS(udCPPClassLanguage);

    udCPPClassLanguage();
    virtual ~udCPPClassLanguage();

    // public virtual functions
    virtual wxString True() const {return wxT("true");}
    virtual wxString False() const {return wxT("false");}
	
	virtual wxString ImplExt(){return wxT(".cpp");}
	virtual wxString DeclExt(){return wxT(".h");}
	
	virtual void SingleLineCommentCmd(const wxString& msg);
	virtual void ClassDeclCmd(const wxString& name, const wxString& parents);
	virtual void ClassConstructorDeclCmd(const wxString& name, const wxString& params, const wxArrayString& bases, const wxArrayString& basesparams);
	virtual void ClassConstructorDefCmd(const wxString& name, const wxString& params, const wxArrayString& bases, const wxArrayString& basesparams);
	virtual void ClassDestructorDeclCmd(const wxString& modif, const wxString& name, const wxString& parent);
	virtual void ClassDestructorDefCmd(const wxString& modif, const wxString& name, const wxString& parent);
	virtual void ClassMemberFcnDeclCmd(const wxString& modif, const wxString& type, const wxString& classname, const wxString& name, const wxString& params);
	virtual void ClassMemberFcnDefCmd(const wxString& modif, const wxString& type, const wxString& classname, const wxString& name, const wxString& params);
	virtual void ClassInstanceCmd(const wxString& instname, const wxString& classname, const wxString& params, bool dynamic);
	virtual void FunctionDefCmd(const wxString& rettype, const wxString& name, const wxString& args);
	virtual wxString MakeValidIdentifier(const wxString& name) const;
};

#endif // udCPPClassLanguage_H
