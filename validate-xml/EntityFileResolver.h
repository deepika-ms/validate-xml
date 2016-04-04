/// @file Xerces/EntityFileResolver.h
/// @brief Contains declaration for EntityFileResolver
#pragma once

//#include <QubeCppUtils/NamespaceMacros.h>

#include <xercesc/sax/EntityResolver.hpp>

#include <string>
#include <memory>

//QUBE_CPP_UTILS_NS_BEGIN
XERCES_CPP_NAMESPACE_USE
namespace Xerces
{
    /// A class to resolve Xerces entity file name to their file content.
    class EntityFileResolver : public XERCES_CPP_NAMESPACE::EntityResolver
    {
    public:
        /**
         * Constructs EntityResolver object without default search directory.
         */
        EntityFileResolver();

        /**
         * Constructs EntityResolver object with default search directory.
         * While resolving an entity file if content/path of the file doesn't
         * exist(i.e. non added) then it will check if the entity file exist
         * in the default search directory and if exist then it will be used.
         * @param defaultSearchDirectory
         *      The path to the directory to use as default search directory.
         */
        EntityFileResolver(const std::string& defaultSearchDirectory);

        /**
         * Destroys the EntityResolver object.
         */
        ~EntityFileResolver();

        /**
         * Adds reference to the file content for the specified entity file.
         * @param fileName
         *      The name of the entity file excluding its path.
         * @param fileContent
         *      The pointer to memory holding the file content. The memory should be alive
         *      throughout the life time of EntityResolver object referring to it.
         * @param fileContentLength
         *      The length of the file content.
         */
        void AddFileContent(const std::string& fileName, //
                            const uint8_t* fileContent, size_t fileContentLength);

        /**
         * Adds path of an entity file. The file content will be read from the path
         * when it tries to resolve the entity file identified by its name.
         * @param fileName
         *      The name of the entity file excluding its path.
         * @param filePath
         *      The full path(including name) of the entity file.
         */
        void AddFilePath(const std::string& fileName, const char* filePath);

        /**
         * Tells is there any file content/path information added or not.
         * @returns
         *      True if there are any file content/path information added, otherwise False.
         */
        bool IsEmpty() const;

        /**
         * Overrides Xerces EntityResolver::resolveEntity
         */
   XERCES_CPP_NAMESPACE::InputSource* resolveEntity(const XMLCh* const publicId,
                                                         const XMLCh* const systemId);

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}

//QUBE_CPP_UTILS_NS_END
