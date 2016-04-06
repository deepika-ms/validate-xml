
#pragma once
#include <xercesc/sax/EntityResolver.hpp>

#include <string>
#include <memory>

XERCES_CPP_NAMESPACE_USE
namespace Xerces
{
    /// A class to resolve Xerces entity file name to their file content.
    class EntityFileResolver : public XERCES_CPP_NAMESPACE::EntityResolver
    {
    public:

        EntityFileResolver();
		EntityFileResolver(const std::string& defaultSearchDirectory);
        ~EntityFileResolver();
        void AddFileContent(const std::string& fileName, //
                            const uint8_t* fileContent, size_t fileContentLength);

        void AddFilePath(const std::string& fileName, const char* filePath);

        bool IsEmpty() const;

   XERCES_CPP_NAMESPACE::InputSource* resolveEntity(const XMLCh* const publicId,
                                                         const XMLCh* const systemId);

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}

