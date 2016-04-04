// XMLValidation.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "NameSpaceUris.h"
#include "Schemas.h"
#include "EntityFileResolver.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/XMLGrammarDescription.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>

#include <xsec/utils/XSECPlatformUtils.hpp>
#include <xsec/framework/XSECProvider.hpp>
#include <xsec/canon/XSECC14n20010315.hpp>
#include <xsec/dsig/DSIGSignature.hpp>
#include <xsec/framework/XSECException.hpp>
#include <xsec/enc/XSECCryptoException.hpp>
#include <xsec/utils/XSECDOMUtils.hpp>
#include <xsec/enc/XSECKeyInfoResolverDefault.hpp>

#include <sys/stat.h>
#include <errno.h>

#include <iostream>
#include <list>
#include <string>
#include <memory>

XERCES_CPP_NAMESPACE_USE

using namespace std;

#define MAX_FILE_PATH_LENGTH 256
const int schemaError = 1;
const int signatureError = 2;
const int valid = 0;

#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>

#include <map>

//using namespace QUBE_CPP_UTILS_NS::Xerces;
XERCES_CPP_NAMESPACE_USE;
using namespace std;

/// @cond PRIVATE
struct EntityFileInfo
{
    EntityFileInfo() : Path(), ContentLength(0) {}

    ~EntityFileInfo() {}

    string Path;
    const uint8_t* Content;
    size_t ContentLength;
};

struct Xerces::EntityFileResolver::Impl
{
    map<string, EntityFileInfo> EntityFileInfos;
    string DefaultSearchDirectory;
};
/// @endcond

Xerces::EntityFileResolver::EntityFileResolver() : _impl(new Impl())
{
}

Xerces::EntityFileResolver::EntityFileResolver(const std::string& defaultSearchDirectory)
    : _impl(new Impl())
{
    _impl->DefaultSearchDirectory = defaultSearchDirectory;
}

Xerces::EntityFileResolver::~EntityFileResolver()
{
}

void Xerces::EntityFileResolver::AddFileContent(const std::string& fileName, 
                                        const uint8_t* fileContent, size_t fileContentLength)
{
    if (_impl->EntityFileInfos.count(fileName) != 0)
    {
        cerr<<"content already exists for the given file"<<endl;
    }

    _impl->EntityFileInfos[fileName].Content = fileContent;
    _impl->EntityFileInfos[fileName].ContentLength = fileContentLength;
}

bool Xerces::EntityFileResolver::IsEmpty() const
{
    return _impl->EntityFileInfos.empty();
}



class StrX
{
    char*   fLocalForm;
public :
    StrX(const XMLCh* const toTranscode)
    {
        fLocalForm = XMLString::transcode(toTranscode);
    }
    ~StrX()
    {
        XMLString::release(&fLocalForm);
    }
    const char* localForm() const { return fLocalForm; }
};

std::ostream&
operator<<(std::ostream& target, const StrX& toDump)
{
    target << toDump.localForm();
    return target;
}

// error handler interface

class DOMTreeErrorReporter : public ErrorHandler
{
public:
    void warning(const SAXParseException& toCatch) {}
    void resetErrors() {}
    void error(const SAXParseException& toCatch)
    {
        StrX fileNameWithPath(toCatch.getSystemId());
        const char* filePath = strrchr(fileNameWithPath.localForm(), '/') + 1;
        cerr << "Error: Schema validation failed for \"" << filePath
             << "\": " << StrX(toCatch.getMessage()) << endl;
    }
    void fatalError(const SAXParseException& toCatch) {
        cerr << "Fatal Error at file \"" << StrX(toCatch.getSystemId())
             << "\", line " << toCatch.getLineNumber()
             << ", column " << toCatch.getColumnNumber() << endl
             << "Message: " << StrX(toCatch.getMessage()) << endl;
    }
};
static bool IsFileExists(const char* filePath)
{
    struct stat fileStat;
    if (stat(filePath, &fileStat) == -1 && errno == ENOENT)
        return false;
    return true;
}
//---------------------------------------------------------------------------
void printUsage()
{
	cout<<"Usage: XMLValidation <XML File Path>"<<endl;
	cout<<"Exits with code"<<endl;
	cout<<"0 - Valid XML"<<endl<<"1 - Schema Check Error"<<endl<<"2 - Signature Validation Error"<<endl;
}

int IsFilePathValid(const char* filename)
{
	if(IsFileExists(filename))
	{
		string fn = filename;
		if(fn.substr(fn.find_last_of(".") + 1) == "xml")
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

int IsSigValid(DOMDocument *theDOM, DOMNode *sigNode)
{
	XSECProvider prov;
	XSECKeyInfoResolverDefault theKeyInfoResolver;
	DSIGSignature * sig = prov.newSignatureFromDOM(theDOM, sigNode);

	sig->setKeyInfoResolver(&theKeyInfoResolver);
	bool result;
	try
	{
		sig->load();
		result = sig->verifySignatureOnly();
	}
	catch (XSECException &e)
	{
		char * msg = XMLString::transcode(e.getMsg());
		cerr << "An error occured during signature verification\n   Message: "
		<< msg << endl;
		XSEC_RELEASE_XMLCH(msg);
		return 0;
	}
	catch (XSECCryptoException &e)
	{
		cerr << "An error occured during signature verification\n   Message: "
		<< e.getMsg() << endl;
		return 0;
	}
	catch (...) 
	{
		cerr << "Unknown Exception type occured.  Cleaning up and exiting\n" << endl;
		return 0;
	}
	if (result)
	{
		cout << "Valid XML" << endl;
		return 1;
	}
	else
	{
		cout << "Invalid XML" << endl;
		char * e = XMLString::transcode(sig->getErrMsgs());
		cout << e << endl;
		XSEC_RELEASE_XMLCH(e);
		return 0;
	}
	
}
	

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		printUsage();
		exit (1);
	}
	else
	{
		if(!IsFilePathValid(argv[1]))
		{
		cout<<"Invalid File";
		exit(1);
		}
	}
    try
    {
       XMLPlatformUtils::Initialize();
	   XSECPlatformUtils::Initialise();
    }
    catch(const XMLException& e)
    {
        StrX tmp_e(e.getMessage());
        cerr << "Initialization error: " << tmp_e.localForm() << endl;
        return schemaError;
    }

    unique_ptr<XercesDOMParser> parser(new XercesDOMParser);
    unique_ptr<DOMTreeErrorReporter> errReporter(new DOMTreeErrorReporter());
    parser->setErrorHandler(errReporter.get());
    parser->setDoNamespaces(true);
    parser->useCachedGrammarInParse(true);
    parser->setValidationScheme(XercesDOMParser::Val_Auto);
    parser->setDoSchema(true);
    parser->setValidationConstraintFatal(true);
    parser->setCreateEntityReferenceNodes(true);

	static Xerces::EntityFileResolver entityFileResolver;

    if (entityFileResolver.IsEmpty())
    {
        entityFileResolver.AddFileContent("INTEROP_CPL_SCHEMA_NAME",
                                          PROTO_ASDCP_CPL_20040511_XSD,
                                          PROTO_ASDCP_CPL_20040511_XSD_SIZE);

        entityFileResolver.AddFileContent("XML_SCHEMA_NAME", XML_XSD, XML_XSD_SIZE);

        entityFileResolver.AddFileContent("SMPTE_CPL_SCHEMA_NAME", CPL_429_7_2006_XSD,
                                          CPL_429_7_2006_XSD_SIZE);

        entityFileResolver.AddFileContent("XML_DSIG_SCHEMA_NAME", XMLDSIG_CORE_SCHEMA_XSD,
                                          XMLDSIG_CORE_SCHEMA_XSD_SIZE);

        entityFileResolver.AddFileContent("SMPTE_CPL_STEREO_SCHEMA_NAME",
                                          MAIN_STEREO_PICTURE_CPL_429_10_2008_XSD,
                                          MAIN_STEREO_PICTURE_CPL_429_10_2008_XSD_SIZE);

        entityFileResolver.AddFileContent("INTEROP_CPL_STEREO_SCHEMA_NAME",
                                          MAIN_STEREO_PICTURE_CPL_XSD,
                                          MAIN_STEREO_PICTURE_CPL_XSD_SIZE);

        entityFileResolver.AddFileContent("INTEROP_CPL_CC_SCHEMA_NAME",
                                          PROTO_ASDCP_CC_CPL_20070926_XSD,
                                          PROTO_ASDCP_CC_CPL_20070926_XSD_SIZE);

        entityFileResolver.AddFileContent("SMPTE_TIMED_TEXT_SCHEMA_NAME", TT_429_12_2008_XSD,
                                          TT_429_12_2008_XSD_SIZE);

        entityFileResolver.AddFileContent("CPL_DOLBY_ATMOS_AUX_DATA_SCHEMA_NAME",
                                          DOLBY_AD_2012_XSD,
                                          DOLBY_AD_2012_XSD_SIZE);
    }

    parser->setEntityResolver(&entityFileResolver);

    try
    {
        parser->parse(argv[1]);
    }
    catch (const OutOfMemoryException&)
    {
        cerr << "Out of memory exception." << endl;
		return schemaError;
    }
    catch (const XMLException& e)
    {
        cerr << "An error occurred during parsing" << endl
             << "   Message: " << StrX(e.getMessage()) << endl;
		return schemaError;
    }
    catch (const DOMException& e)
    {
        const unsigned int maxChars = 2047;
        XMLCh errText[maxChars + 1];
        cerr << endl
             << "A DOM error occurred during parsing: '"
             << std::string(argv[1])
             << "'" << endl
             << "DOM Exception code:  "
             << e.code << endl;
        if (DOMImplementation::loadDOMExceptionMsg(e.code, errText, maxChars))
        {
            cerr << "Message is: " << StrX(errText) << endl;
        }
		return schemaError;
    }
    catch (...)
    {
        cerr << "An unclassified error occurred during parsing." << endl;
		return schemaError;
    }

	DOMNode *doc;	
	doc = parser->getDocument();
	DOMDocument *theDOM = parser->getDocument();
	// Find the signature node
	DOMNode *sigNode = findDSIGNode(doc, "Signature");

	if (sigNode == 0)
	{
		DOMElement *FirstNode = theDOM->getDocumentElement();
		const char* root = XMLString::transcode(FirstNode->getNodeName());
		if(strcmp(root,"DCinemaSecurityMessage"))
		{
			cout<<"Valid XML"<<endl;
			return valid;
		}
		else
			cerr << "Could not find <Signature> node in " << argv[1] << endl;
		return signatureError;
	}
	else
	{
		if(IsSigValid(theDOM, sigNode))
		{
			return valid;
		}
		else
			return signatureError;
	}
	
	XSECPlatformUtils::Terminate();
    XMLPlatformUtils::Terminate();

    return valid;
}
