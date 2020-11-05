#pragma once

#include <clang-c/Index.h>
#include <string>

namespace ts3d { namespace exchange { namespace parser {
    namespace csharp {
//        namespace cpp_layer {
//            std::string getInitializeFunctionSpelling( CXCursor struct_decl );
//            bool write( CXTranslationUnit unit, std::string const header_fn, std::string const cpp_fn );
//        }
//        
        namespace direct {
            bool writeStructs( CXTranslationUnit unit, std::string const fn );
            bool writeAPI( CXTranslationUnit unit, std::string const fn );
            bool writeEnums( CXTranslationUnit unit, std::string const fn );
//            bool writeStructSizeTests( CXTranslationUnit unit, std::string const fn );
            bool writeClasses( CXTranslationUnit unit, std::string const fn );
        }
    }
} } }
