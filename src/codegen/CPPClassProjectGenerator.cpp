#include "wx_pch.h"

#ifdef _DEBUG_MSVC
#define new DEBUG_NEW
#endif

#include <wx/sstream.h>
#include <wx/dcsvg.h>
#include <map>

#include "Settings.h"
#include "Project.h"
#include "UMLDesignerApp.h"
#include "UMLDesignerMain.h"
#include "projectbase/gui/ProgressDialog.h"
#include "projectbase/codegen/Generator.h"
#include "codegen/CPPClassProjectGenerator.h"
#include "projectbase/codegen/CodeItemsGenerator.h"

#include "../plugins/diagUml/DiagUml.h"

IMPLEMENT_DYNAMIC_CLASS(udCPPClassProjectGenerator, udProjectGenerator);

// constructor and destructor ///////////////////////////////////////////////////////////////////////////////

udCPPClassProjectGenerator::udCPPClassProjectGenerator()
{
}

udCPPClassProjectGenerator::~udCPPClassProjectGenerator()
{
}

void udCPPClassProjectGenerator::Initialize()
{
	udProjectGenerator::Initialize();
}

// protected virtual functions //////////////////////////////////////////////////////////////////////////////

void udCPPClassProjectGenerator::ProcessProject(udProject *src)
{
	udDiagramItem *pDiagram;
	udGenerator *pGenerator;
	bool fSuccess;
	
	udProgressDialog progressDlg( NULL );
	
	wxArrayString arrProcessedGenerators;
	
	// get settings of processed project
	udProjectSettings& Settings = src->GetSettings();
	
	wxFileName fnOutDir;
	wxString sPath = Settings.GetProperty(wxT("Output directory"))->ToString();
	wxString sPathDoc = Settings.GetProperty(wxT("Output directory"))->ToString();
	wxString sPathClass = Settings.GetProperty(wxT("Output directory"))->ToString();

	fnOutDir.SetPath( sPath );
	if( fnOutDir.IsRelative() )
	{
		sPath = udProject::Get()->GetProjectDirectory() + wxFileName::GetPathSeparator() + sPath;
		sPathDoc = udProject::Get()->GetProjectDirectory() + wxFileName::GetPathSeparator() 
				+ "doc" + wxFileName::GetPathSeparator();
		sPathClass = udProject::Get()->GetProjectDirectory() + wxFileName::GetPathSeparator() 
				+ "machines" + wxFileName::GetPathSeparator();
	}
	
	if( !wxDirExists( sPath ) )
	{
		wxMessageBox(wxT("Output directory '") + sPath + wxT("' doesn't exist. Please check the project settings."), wxT("CodeDesigner"), wxICON_ERROR | wxOK );
		return;
	}

	// Documentation directory
	if( !wxDirExists( sPathDoc ) ) wxMkdir( sPathDoc );

	if( !wxDirExists( sPathClass ) ) wxMkdir( sPathClass );

	wxFileName OutFile, HeaderFile, DocFile;
	
	if( m_pOutLang )
	{
		wxStringOutputStream *pOut;
		
		// get list of diagrams to be generated
		SerializableList lstDiagrams;
		src->GetDiagramsRecursively( CLASSINFO(udDiagramItem), lstDiagrams );
		
		udGenerator::ResetIDCounter();
		
		progressDlg.Clear();
		progressDlg.SetStepCount( lstDiagrams.GetCount() + 1 );
		
		progressDlg.Show();
		progressDlg.Raise();
		
		progressDlg.Step();
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// generate diagrams
		
		// Printed Common functions
		bool printCommon = false;

		// construct output file path
		OutFile = GetFullCodePath( Settings.GetProperty(wxT("Base file name"))->AsString(), m_pOutLang->GetExtension(udLanguage::FE_IMPL) );
				
		// construct header file path
		HeaderFile = GetFullCodePath( Settings.GetProperty(wxT("Base file name"))->AsString(), m_pOutLang->GetExtension(udLanguage::FE_DECL) );

		// construct documentation file path
		DocFile = GetFullCodePath( Settings.GetProperty(wxT("Base file name"))->AsString(), m_pOutLang->GetExtension(udLanguage::FE_DOC) );

		
		// **********************************************************************************************************************************************
		// Common functions
		// Insert ifndef 

		wxString commentarios = wxT(" @author ---");
		commentarios = commentarios + ENDL + wxT(" @copyright ---");
		commentarios = commentarios + ENDL + wxT(" @brief Common functions.");
		commentarios = commentarios + ENDL + wxT(" @warning <b>THIS IS A AUCTOMATIC GENERATION FILE. DON'T EDIT.</b> ");

		WriteToFile ( wxString::Format(wxT("/** ***************************************************************************")), HeaderFile );
		InsertEndFile ( wxString::Format(wxT("%s"), commentarios) << ENDL, HeaderFile );
		InsertEndFile ( wxString::Format(wxT("*************************************************************************** */")), HeaderFile );

		InsertEndFile( wxString::Format( wxT("#ifndef CODE_DESIGN_%s_H"), HeaderFile.GetName()) << ENDL << 
			wxString::Format( wxT("#define CODE_DESIGN_%s_H"), HeaderFile.GetName()) << ENDL, HeaderFile );

		InsertEndFile ( wxString::Format( wxT("#include <stdio.h>")), HeaderFile );
		InsertEndFile ( wxString::Format( wxT("#include <stdlib.h>")) << ENDL, HeaderFile	);
		InsertEndFile ( wxString::Format( wxT("#include \"FSMachine.h\"")) << ENDL, HeaderFile );

		// Insert class definition
		InsertEndFile ( wxString::Format( wxT("class %s : public FSMachine {"), HeaderFile.GetName() ) << ENDL,  HeaderFile );

		// **********************************************************************************************************************************************
			
		// **********************************************************************************************************************************************
		// Print definition file
		// clear previous code if exists

		WriteToFile ( wxString::Format(wxT("/** ***************************************************************************")), OutFile );
		InsertEndFile ( wxString::Format(wxT("%s"), commentarios) << ENDL, OutFile );
		InsertEndFile ( wxString::Format(wxT("*************************************************************************** */")), OutFile );
		InsertEndFile( wxString::Format( wxT("#include \"%s.h\""), HeaderFile.GetName() ) << ENDL, OutFile );

		// ************************************************************************************************

		// Doxygen Documentation File
		WriteToFile(  wxString::Format(wxT("/**")) << ENDL << wxString::Format( wxT("@defgroup %s FSMs "), DocFile.GetName() ) << ENDL, DocFile );
		InsertEndFile(  wxString::Format(wxT("<table>")) << ENDL, DocFile );
		
		// ************************************************************************************************
		// clean-up output stream
		delete pOut;
		
		SerializableList::compatibility_iterator node = lstDiagrams.GetLast();
		while( node )
		{
			
			fSuccess = true;
			pDiagram = (udDiagramItem*)node->GetData();
			
			if( !pDiagram->IsGenerated() )
			{
				node = node->GetPrevious();
				continue;
			}

			progressDlg.SetLabel(  wxString::Format( wxT("Generating '%s'..."), pDiagram->GetName().c_str() ) );
			Log( wxString::Format( wxT("Generating code for diagram '%s'..."), pDiagram->GetName().c_str() ) );
			
			// Para obtener el nombre de la clase: wxMessageBox(wxT("LOG: ") + OutFile.GetName() );
			wxString clasName = pDiagram->GetName();
			clasName.Replace(wxString(" "), wxString(""));
			//Documentación
			wxString pathToDocFile = sPathDoc + clasName + wxT(".dox");
			wxString pathToImgFile = sPathDoc + clasName + wxT(".jpg");

			wxFileName docDiagram = wxFileName(pathToDocFile);

			InsertEndFile(  wxString::Format(wxT("<tr> <td>@link %s %s </td></tr>"), clasName.c_str(), pDiagram->GetName().c_str()), DocFile );

			WriteToFile(  wxString::Format(wxT("/**")) << ENDL << wxString::Format( wxT("@page %s %s"), clasName.c_str(), pDiagram->GetName().c_str() ), docDiagram );
			InsertEndFile(  wxString::Format( wxT("@ingroup %s"),  DocFile.GetName().c_str()) << ENDL, docDiagram );
			InsertEndFile(  wxString::Format(wxT("%s"), pDiagram->GetDescription().c_str() ) << ENDL, docDiagram );
			InsertEndFile(  wxString::Format(wxT("@image html %s.jpg"), clasName.c_str() ) << ENDL << ENDL, docDiagram );

			// **************** CREATE IMG DIAGRAM **************************************************************
			if (! pDiagram->GetDiagramPage()) {
				pDiagram->ShowDiagramPage();
			}
			
			udDiagramCanvas *canvas = pDiagram->GetDiagramPage();

			canvas->SaveCanvasToImage( pathToImgFile, wxBITMAP_TYPE_JPEG, true, -1, false);

			// ****************** CREATE DIAGRAMS PAGES
			SerializableList lstCodeItems;
			ShapeList lstDiagItems;
			udProjectItem *pDiagItem;
			udFunctionItem *pCodeItem;

			IPluginManager::Get()->GetProject()->GetItems( CLASSINFO(udFunctionItem), lstCodeItems );
			pDiagram->GetDiagramManager().GetShapes(CLASSINFO(wxSFShapeBase), lstDiagItems);
			ShapeList::compatibility_iterator snode = lstDiagItems.GetFirst();
			wxString m = wxT("");
			while(snode)
			{
				pDiagItem = (udProjectItem*)snode->GetData()->GetUserData();
				if( pDiagItem ) {				
					for( SerializableList::iterator nodeAux = pDiagItem->GetChildrenList().begin(); nodeAux != pDiagItem->GetChildrenList().end(); ++ nodeAux ) {

						udFunctionLinkItem *pPar = wxDynamicCast( *nodeAux, udFunctionLinkItem );
						if (pPar) {
							wxString code = wxT("|") + pPar->GetName() + wxT("|");
							
							if (m.Find(code) ==  wxNOT_FOUND) {

								SerializableList::compatibility_iterator nodeItem = lstCodeItems.GetFirst();
								bool foundIt = false;
								while( nodeItem && ! foundIt)
								{
									pCodeItem = (udFunctionItem*)nodeItem->GetData();
									if (pCodeItem->GetName().IsSameAs(pPar->GetName())) {
										InsertEndFile(  wxString::Format(wxT("@par %s"), pCodeItem->GetName().c_str()), docDiagram );
										InsertEndFile(  wxString::Format(wxT("%s"), pCodeItem->GetDescription().c_str()), docDiagram );
										InsertEndFile(  wxString::Format(wxT("@code %s @endcode"), pCodeItem->GetCode().c_str()) << ENDL, docDiagram );
										InsertEndFile(  wxString::Format(wxT("---------------------------------------------------------------------------------------------------------------")) << ENDL << ENDL, docDiagram );

										foundIt = true;
									}

									nodeItem = nodeItem->GetNext();
								}
								m = m.Append(code);
							}							
						}
					}
				}
				snode = snode->GetNext();
			}
			
		

			//Fin documentación

			wxFileName ClassHeaderFile, ClassOutFile;

			// construct common output file path
			ClassOutFile = sPathClass + clasName + m_pOutLang->GetExtension(udLanguage::FE_IMPL);
			//ClassOutFile = GetFullCodePath( clasName, m_pOutLang->GetExtension(udLanguage::FE_IMPL) );
				
			// construct common header file path
			ClassHeaderFile = sPathClass + clasName + m_pOutLang->GetExtension(udLanguage::FE_DECL);
			//ClassHeaderFile = GetFullCodePath( clasName, m_pOutLang->GetExtension(udLanguage::FE_DECL) );


			// **********************************************************************************************************************************************
			// Common functions
			// Insert ifndef 
			WriteToFile( wxString::Format( wxT("#ifndef CODE_DESIGN_%s_H"), ClassHeaderFile.GetName()) << ENDL << 
				wxString::Format( wxT("#define CODE_DESIGN_%s_H"), ClassHeaderFile.GetName()) << ENDL, ClassHeaderFile );
			
			InsertEndFile ( wxString::Format(wxT("/** ***************************************************************************")), ClassHeaderFile );
			InsertEndFile ( wxString::Format(wxS("%s"), pDiagram->GetDescription().ToAscii()) << ENDL, ClassHeaderFile );
			InsertEndFile ( wxString::Format(wxT("*************************************************************************** */")), ClassHeaderFile );
			InsertEndFile ( wxString::Format( wxT("#include \"../%s.h\""), HeaderFile.GetName()) << ENDL, ClassHeaderFile );

			// Insert class definition
			InsertEndFile ( wxString::Format( wxT("class Class%s : public %s {"), ClassHeaderFile.GetName(), HeaderFile.GetName() ) << ENDL,  ClassHeaderFile );
			
			WriteToFile( wxString::Format(wxT("/** ***************************************************************************")), ClassOutFile );
			InsertEndFile( wxString::Format(wxT("%s"), pDiagram->GetDescription().ToAscii() ) << ENDL, ClassOutFile );
			InsertEndFile( wxString::Format(wxT("*************************************************************************** */")), ClassOutFile );
			InsertEndFile( wxString::Format( wxT("#include \"%s.h\""), ClassHeaderFile.GetName() ) << ENDL, ClassOutFile );

			Log( wxString::Format( wxT("Output files: %s, %s."), OutFile.GetFullPath().c_str(), HeaderFile.GetFullPath().c_str() ) );

			// Cmabiamos la clase que lo genera.
			//m_pOutLang->SetDescription(wxString::Format( wxT("%s|Class%s"), HeaderFile.GetName(), ClassHeaderFile.GetName()));
			m_pOutLang->SetFuncClass(HeaderFile.GetName());
			m_pOutLang->SetTypeClass(HeaderFile.GetName());

			pGenerator = udPROJECT::CreateGenerator( pDiagram );
			if( pGenerator )
			{
				pGenerator->SetActiveLanguage(m_pOutLang);
				
				
				////////////////////////////////////////////////////////////////////////////////////////////////////////
				// generate declaration

				if( ! printCommon )
				{
					// create output stream
					pOut = new wxStringOutputStream(NULL);				
					pGenerator->SetOutputStream(pOut);
					

					pGenerator->SetMode( udGenerator::genCOMMON_DECLARATION );
						
					// generate code
					fSuccess =  pGenerator->Generate( pDiagram, sfNORECURSIVE );
					if( fSuccess )
					{
						InsertEndFile ( wxString::Format( wxT("public:")) << ENDL,  HeaderFile );
						InsertEndFile( pOut->GetString(), HeaderFile );
						InsertEndFile ( wxString::Format( wxT("virtual STATE_T execute();")) << ENDL,  HeaderFile );
					}
					// clean-up output stream
					delete pOut;
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				// create definition file
				

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				if( ! printCommon ) {
					// create output stream
					pOut = new wxStringOutputStream(NULL);
					pGenerator->SetOutputStream( pOut );
				
					pGenerator->SetMode( udGenerator::genCOMMON_DEFINITION );
				
					// generate code
					fSuccess = pGenerator->Generate( pDiagram, sfNORECURSIVE );
					if( fSuccess )
					{
						InsertEndFile( pOut->GetString(), OutFile );
						InsertEndFile ( wxString::Format( wxT(" ")) << ENDL,  OutFile );
						InsertEndFile ( wxString::Format( wxT("%s::STATE_T %s::execute() {"),  OutFile.GetName(), OutFile.GetName()) << ENDL,  OutFile );
						InsertEndFile ( wxString::Format( wxT(" printf(\"ERROR: This method can't access.\\n STOP PROGRAM\");"), ClassHeaderFile.GetName()) << ENDL,  OutFile );
						InsertEndFile ( wxString::Format( wxT(" std::abort();")) << ENDL,  OutFile );
						InsertEndFile ( wxString::Format( wxT("};")) << ENDL,  OutFile );
					}
					
					// clean-up output stream
					delete pOut;
					printCommon = true;
				}

				// ////////////////////////////////////////////////////////////////////////////////////////////////////////
				// // generate definition

				m_pOutLang->SetFuncClass(wxT("Class") + ClassHeaderFile.GetName());
				pGenerator->SetActiveLanguage(m_pOutLang);

				// Print Common definitions
				if( fSuccess )
				{
					// create output stream
					pOut = new wxStringOutputStream(NULL);				
					pGenerator->SetOutputStream(pOut);

					pGenerator->SetMode( udGenerator::genDECLARATION );
											
					// generate code
					fSuccess =  pGenerator->Generate( pDiagram, sfNORECURSIVE );
					if( fSuccess )
					{
						InsertEndFile ( wxString::Format( wxT("private:")) << ENDL,  ClassHeaderFile );
						InsertEndFile( pOut->GetString(), ClassHeaderFile );

						InsertEndFile ( wxString::Format( wxT("public:")) << ENDL,  ClassHeaderFile );
						InsertEndFile ( wxString::Format( wxT("STATE_T execute();")) << ENDL,  ClassHeaderFile );
					}
				}

				if( fSuccess )
				{
					// create output stream
					pOut = new wxStringOutputStream(NULL);
					
					pGenerator->SetOutputStream( pOut );
					pGenerator->SetMode( udGenerator::genDEFINITION );

					// generate code
					fSuccess = pGenerator->Generate( pDiagram, sfNORECURSIVE );
					if( fSuccess )
					{
						
						InsertEndFile( pOut->GetString(), ClassOutFile );
						InsertEndFile ( wxString::Format( wxT(" ")) << ENDL,  ClassOutFile );
						InsertEndFile ( wxString::Format( wxT("%s::STATE_T Class%s::execute() {"),  HeaderFile.GetName(), ClassHeaderFile.GetName()) << ENDL,  ClassOutFile );
						InsertEndFile ( wxString::Format( wxT(" return this->%s();"), ClassHeaderFile.GetName()) << ENDL,  ClassOutFile );
						InsertEndFile ( wxString::Format( wxT("};")) << ENDL,  ClassOutFile );
					}
					
					// clean-up output stream
					delete pOut;
				}
				// clean-up code generator;
				delete pGenerator;
				
				if( !fSuccess ) wxMessageBox(wxT("Generation process finished with ERROR status. See the log window for more details."), wxT("CodeDesigner"), wxOK | wxICON_WARNING);
				
				progressDlg.Step();
				Log( wxT("Done.") );

				InsertEndFile(  wxString::Format(wxT("**************************************** */")) << ENDL, docDiagram );		
			}
			InsertEndFile( wxString::Format(wxT("};")) << ENDL, ClassHeaderFile );
			InsertEndFile( wxString::Format(wxT("#endif /* CODE_DESIGN_%s_H */"), ClassHeaderFile.GetName()) << ENDL, ClassHeaderFile );
			node = node->GetPrevious();
		}
		InsertEndFile(  wxString::Format(wxT("</table>")) << ENDL<< ENDL<< ENDL<< ENDL<< ENDL<< ENDL, DocFile );
		InsertEndFile(  wxString::Format(wxT("---------------------------------------------------------------------------------------------------------------")) << ENDL << ENDL, DocFile );
		InsertEndFile(  wxString::Format(wxT("**************************************** */")) << ENDL, DocFile );

		InsertEndFile( wxString::Format(wxT("};")) << ENDL, HeaderFile );
		InsertEndFile( wxString::Format(wxT("#endif /* CODE_DESIGN_%s_H */"), HeaderFile.GetName()) << ENDL, HeaderFile );

	}
}

void udCPPClassProjectGenerator::CleanUp()
{
	udProjectGenerator::CleanUp();
}

