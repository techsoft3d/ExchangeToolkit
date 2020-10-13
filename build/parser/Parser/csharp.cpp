#include <sstream>
#include <fstream>
#include "csharp.h"
#include "util.h"

using namespace ts3d::exchange::parser::csharp;

namespace {
    std::string processEnumDecl( CXCursor c ) {
        std::stringstream ss;
        auto const enum_decl_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType( c ) ) );
        std::cout << "Processing enum decl \"" << enum_decl_spelling << "\"" << std::endl;
        ss << "    public enum " << enum_decl_spelling << std::endl;
        ss << "    {" << std::endl;
        std::vector<std::string> enum_constants;
        clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            std::stringstream ss;
            if( clang_getCursorKind( c ) == CXCursor_EnumConstantDecl ) {
                auto const enum_constant_decl_spelling = ts3d::toString( clang_getCursorSpelling( c ) );
                auto const enum_constant_decl_type = clang_getCursorType( c );
                if( enum_constant_decl_type.kind != CXType_Int ) {
                    throw std::runtime_error("Encountered unexpected enum constant type.");
                }
                ss << enum_constant_decl_spelling << " = " << clang_getEnumConstantDeclValue( c );
                reinterpret_cast<std::vector<std::string>*>(client_data)->push_back( ss.str() );
            } else {
                throw std::runtime_error("Encountered unexpected cursor type inside enum decl." );
            }
            return CXChildVisit_Continue;
        }, &enum_constants);
        for( auto idx = 0u; idx < enum_constants.size(); ++idx ) {
            ss << "        " << enum_constants[idx];
            if( idx < enum_constants.size()-1 ) {
                ss << ",";
            }
            ss << std::endl;
        }
        ss << "}" << std::endl;
        return ss.str();
    }

    std::string getCSharpTypeSpelling( CXType t ) {
        auto const canonical_type = clang_getCanonicalType( t );
        switch( canonical_type.kind ) {
            case CXType_UShort:
                return "ushort";
                break;
            case CXType_Double:
                return "double";
                break;
            case CXType_Record:
            case CXType_Enum:
                if( clang_isConstQualifiedType( t ) ) {
                    auto foo = ts3d::toString( clang_getTypeSpelling( clang_Type_getModifiedType( t ) ) );
                    int j = 0;

                }
                return ts3d::toString( clang_getTypeSpelling( t ) );
                break;
            case CXType_SChar:
                if( ts3d::toString( clang_getTypeSpelling( t ) ) == "A3DBool" ) {
                    return "bool";
                } else if( ts3d::toString( clang_getTypeSpelling( t ) ) == "A3DInt8" ) {
                    return "byte";
                } else {
                    std::cout << ts3d::toString( clang_getTypeSpelling( t ) );
                    throw std::runtime_error("Unhandled struct field type." );
                }
                break;
            case CXType_UInt:
                return "uint";
                break;
            case CXType_Pointer:
            {
                auto const pointee_type = clang_getPointeeType( t );
                
            }
                return "IntPtr";
                break;
            case CXType_UChar:
                return "byte";
                break;
            case CXType_Int:
                return "int";
                break;
            case CXType_Float:
                return "float";
                break;
            case CXType_Char_S:
            case CXType_Char_U:
                return "byte";
                break;
            case CXType_Void:
                return "void";
                break;
            case CXType_ConstantArray:
            {
                std::stringstream ss;
                ss << getCSharpTypeSpelling( clang_getArrayElementType(t) ) <<  "[" << clang_getArraySize(t) << "]";
                return ss.str();
            }
                break;
            case CXType_IncompleteArray:
                return "IntPtr";
                break;
            default:
                throw std::runtime_error("Unhandled struct field type." );
                break;
        }
        return std::string();
    }
    
    std::string getCSharpTypeSpelling( CXCursor c ) {
        auto const field_decl_type = clang_getCursorType( c );
        try {
            return getCSharpTypeSpelling( field_decl_type );
        } catch( std::exception e ) {
            dump(c);
            throw e;
        }
        return std::string();
    }
    
    std::pair<CXType, long long> getOneDimensionalArray( CXType t ) {
        assert( t.kind == CXType_ConstantArray );
        auto const result = std::make_pair( clang_getCanonicalType( clang_getArrayElementType( t ) ), clang_getArraySize( t ) );
        
        if( result.first.kind == CXType_ConstantArray ) {
            auto element = getOneDimensionalArray( result.first );
            element.second *= result.second;
            return element;
        }
        
        return result;
    }
    
    std::string processFieldDecl( CXCursor c ) {
        std::stringstream ss;
        auto const field_decl_type = clang_getCursorType( c );
        auto const field_decl_type_spelling = ts3d::toString( clang_getTypeSpelling( field_decl_type ) );
        auto const canonical_type = clang_getCanonicalType( field_decl_type );
        auto const field_decl_spelling = ts3d::toString( clang_getCursorSpelling( c ) );
        if( CXType_ConstantArray == canonical_type.kind ) {
            auto const array_as_one_d = getOneDimensionalArray( field_decl_type );
            ss << "public fixed " << getCSharpTypeSpelling( array_as_one_d.first ) << " " << field_decl_spelling << "[" << array_as_one_d.second <<  "];" << std::endl;
        } else {
            if( field_decl_type_spelling == "A3DBool" ) {
                ss << "[MarshalAs(UnmanagedType.I1)] public bool " << field_decl_spelling << ";" << std::endl;
            } else if( field_decl_type_spelling == "A3DUTF8Char *" ) {
                ss << "[MarshalAs(UnmanagedType.LPStr)] public string " << field_decl_spelling << ";" << std::endl;
            } else {
                ss << "public " << getCSharpTypeSpelling( c ) << " " << field_decl_spelling << ";" << std::endl;
            }
        }

        return ss.str();
    }
    
    std::string processStructDecl( CXCursor c, std::string struct_decl_spelling ) {
        std::stringstream ss;
        if( struct_decl_spelling.empty() ) {
            struct_decl_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType( c ) ) );
        }
        std::cout << "Processing struct decl \"" << struct_decl_spelling << "\"" << std::endl;
        ss << "    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]" << std::endl;
        ss << "    public unsafe struct " << struct_decl_spelling << std::endl;
        ss << "    {" << std::endl;
        clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &ss = *reinterpret_cast<std::stringstream*>(client_data);
            if( clang_getCursorKind( c ) == CXCursor_FieldDecl ) {
                ss << "        " << processFieldDecl( c );
            } else if( clang_getCursorKind( c ) == CXCursor_UnionDecl) {
                ss << "        private fixed byte _union[" << clang_Type_getSizeOf( clang_getCursorType(c) ) << "];" << std::endl;
                clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data ) {
                    auto &ss = *reinterpret_cast<std::stringstream*>(client_data);
                    auto const field_spelling = ts3d::toString(clang_getCursorSpelling( c ) );
                    auto const field_type_spelling = ts3d::toString(clang_getTypeSpelling( clang_getCursorType(c)));
                    ss << "        public " << field_type_spelling << " " << field_spelling << std::endl;
                    ss << "        {" << std::endl;
                    ss << "            get" << std::endl;
                    ss << "            {" << std::endl;
                    ss << "                fixed( byte *p = _union )" << std::endl;
                    ss << "                {" << std::endl;
                    ss << "                    IntPtr ptr = new IntPtr((void*)p);" << std::endl;
                    ss << "                    return Marshal.PtrToStructure<" << field_type_spelling << ">(ptr);" << std::endl;
                    ss << "                }" << std::endl;
                    ss << "            }" << std::endl;
                    ss << "        }" << std::endl;
                    return CXChildVisit_Continue;
                }, &ss);
            } else {
                throw std::runtime_error("Encountered unexpected cursor type inside enum decl." );
            }
            return CXChildVisit_Continue;
        }, &ss);
        ss << "    }" << std::endl;
        return ss.str();

    }
    
    std::string getCursorTypeAsCSharpArgumentType( CXCursor c ) {
        assert( c.kind == CXCursor_ParmDecl );

        auto const cursor_type = clang_getCursorType( c );
        if( CXType_Pointer == cursor_type.kind ) {
            auto const pointee_type = clang_getPointeeType( cursor_type );
            if( CXType_Typedef == pointee_type.kind ) {
                auto const pointee_underlying_type = clang_getTypedefDeclUnderlyingType( clang_getTypeDeclaration( pointee_type ) );
                if( pointee_underlying_type.kind == CXType_Void ) {
                    return "IntPtr";
                } else if( CXType_Elaborated == pointee_underlying_type.kind ) {
                    auto const pointee_spelling = ts3d::toString( clang_getTypeSpelling( pointee_type ) );
                    if( !clang_isConstQualifiedType( pointee_type ) ) {
                        return "ref " + pointee_spelling;
                    } else {
                        auto const pointee_type_decl =  clang_getTypeDeclaration( pointee_underlying_type );
                        if( CXCursor_StructDecl == pointee_type_decl.kind ) {
                            auto const struct_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType( pointee_type_decl ) ) );
                            return "ref " + struct_spelling;
                        }
                    }
                } else if( CXType_Char_S == pointee_underlying_type.kind ) {
                    if( !clang_isConstQualifiedType( cursor_type ) ) {
                        return "[MarshalAs(UnmanagedType.LPStr)] string";
                    } else {
                        return "[MarshalAs(UnmanagedType.LPStr)] out string";
                    }
                } else {
                    return "out " + getCSharpTypeSpelling( pointee_type );
                }
            } else if( pointee_type.kind == CXType_Pointer ) {
                // pointer to a pointer
                auto const pointee_pointee_type = clang_getPointeeType( pointee_type );
                if( CXType_Typedef == pointee_pointee_type.kind ) {
                    auto const pointee_pointee_underlying_type = clang_getTypedefDeclUnderlyingType( clang_getTypeDeclaration( pointee_pointee_type ) );
                    if( CXType_Void == pointee_pointee_underlying_type.kind ) {
                        return "out IntPtr";
                    }
                }
            }
        } else if( CXType_Typedef == cursor_type.kind ) {
            CXCursor param_cursor = clang_getNullCursor();
            clang_visitChildren(c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
                *reinterpret_cast<CXCursor*>(client_data) = c;
                return CXChildVisit_Continue;
            }, &param_cursor);
            
            if( clang_Cursor_isNull( param_cursor ) ) {
                throw std::runtime_error("Parsing error." );
            }
            
            auto param_cursor_type = clang_getCursorType( param_cursor );
            return getCSharpTypeSpelling( param_cursor_type );
        }

        return getCSharpTypeSpelling( clang_getCursorType(c));
    }
    
    void writeCSharpStructInitializers( CXTranslationUnit unit, std::ostream &out_stream ) {
        // write the Initialize functions
        std::vector<CXCursor> structs_with_size;
        clang_visitChildren(clang_getTranslationUnitCursor( unit ), [](CXCursor c, CXCursor parent, CXClientData client_data) {
            if( clang_getCursorKind(c) == CXCursor_StructDecl && ts3d::hasField(c, "m_usStructSize") ) {
                reinterpret_cast<std::vector<CXCursor>*>(client_data)->push_back(c);
            }
            return CXChildVisit_Continue;
        }, &structs_with_size );
        
        for( auto const struct_decl_cursor : structs_with_size ) {
            auto const struct_spelling = ts3d::toString(clang_getTypeSpelling(clang_getCursorType(struct_decl_cursor)));
            out_stream << "        [DllImport(\"Exchange.layer\", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]" << std::endl;
            out_stream << "        private static extern void Initialize" << struct_spelling << "(ref " << struct_spelling << " s);" << std::endl;
            out_stream << "        public static void Initialize(out " << struct_spelling << " s)" << std::endl;
            out_stream << "        {" << std::endl;
            out_stream << "            s = new " << struct_spelling << "();" << std::endl;
            out_stream << "            s.m_usStructSize = (UInt16)Marshal.SizeOf(typeof(" << struct_spelling << "));" << std::endl;
            out_stream << "            " << cpp_layer::getInitializeFunctionSpelling(struct_decl_cursor) << "(ref s);" << std::endl;
            out_stream << "        }" << std::endl;
        }
    }
}

bool ts3d::exchange::parser::csharp::writeStructs( CXTranslationUnit unit, std::string const fn ) {
    std::ofstream out_stream( fn );
    if( !out_stream.is_open() ) {
        std::cerr << "Unable to open output file: " << fn << std::endl;
        return false;
    }
    
    out_stream << "using System;" << std::endl;
    out_stream << "using System.Runtime.InteropServices;" << std::endl;
    out_stream << std::endl;
    out_stream << "namespace TS3D.Exchange.Direct" << std::endl;
    out_stream << "{" << std::endl;

    clang_visitChildren( clang_getTranslationUnitCursor( unit ), [](CXCursor c, CXCursor parent, CXClientData client_data) {
        auto &out_stream = *(reinterpret_cast<std::ofstream*>(client_data));
        if( CXCursor_TypedefDecl == clang_getCursorKind( c ) && CXCursor_StructDecl == clang_getCursorKind( clang_getTypeDeclaration( clang_getTypedefDeclUnderlyingType(c) ) ) ) {
            auto const struct_code = processStructDecl( clang_getTypeDeclaration( clang_getTypedefDeclUnderlyingType(c) ), ts3d::toString(clang_getCursorSpelling(c)));
            if( !struct_code.empty()) {
                out_stream << struct_code << std::endl;
            }
        }
        return CXChildVisit_Continue;
    }, &out_stream);

    out_stream << "}" << std::endl;
    out_stream << std::endl;
    return true;
}

bool ts3d::exchange::parser::csharp::writeEnums( CXTranslationUnit unit, std::string const fn ) {
    std::ofstream out_stream( fn );
    if( !out_stream.is_open() ) {
        std::cerr << "Unable to open output file: " << fn << std::endl;
        return false;
    }
    
    out_stream << "namespace TS3D.Exchange.Direct" << std::endl;
    out_stream << "{" << std::endl;
    
    clang_visitChildren( clang_getTranslationUnitCursor( unit ), [](CXCursor c, CXCursor parent, CXClientData client_data) {
        auto &out_stream = *(reinterpret_cast<std::ofstream*>(client_data));
        if( CXCursor_EnumDecl == clang_getCursorKind(c)) {
            auto const enum_code = processEnumDecl(c);
            if( !enum_code.empty() ) {
                out_stream << enum_code << std::endl;
            }
        }
        return CXChildVisit_Continue;
    }, &out_stream );
    
    out_stream << "}" << std::endl;
    return true;
}

bool ts3d::exchange::parser::csharp::writeAPI( CXTranslationUnit unit, std::string const fn ) {
    std::ofstream out_stream( fn );
    if( !out_stream.is_open() ) {
        std::cerr << "Unable to open output file: " << fn << std::endl;
        return false;
    }
    
    out_stream << "using System;" << std::endl;
    out_stream << "using System.Runtime.InteropServices;" << std::endl;
    out_stream << std::endl;
    out_stream << "namespace TS3D.Exchange.Direct" << std::endl;
    out_stream << "{" << std::endl;
    out_stream << "    public class API" << std::endl;
    out_stream << "    {" << std::endl;

    writeCSharpStructInitializers( unit, out_stream );

    clang_visitChildren( clang_getTranslationUnitCursor( unit ), [](CXCursor c, CXCursor parent, CXClientData client_data) {
        auto const cursor_location = clang_getCursorLocation( c );
        CXFile source_file;
        clang_getSpellingLocation( cursor_location, &source_file, nullptr, nullptr, nullptr);
        auto const source_file_spelling = ts3d::toString( clang_getFileName( source_file ) );
        if( std::regex_match( source_file_spelling, std::regex( ".*A3DSDKDraw\\.h$") ) ) {
            return CXChildVisit_Continue;
        }
        auto &out_stream = *(reinterpret_cast<std::ofstream*>(client_data));
        if( clang_getCursorKind(c) == CXCursor_TypedefDecl ) {
            auto underlying_type = clang_getTypedefDeclUnderlyingType( c );
            if( underlying_type.kind == CXType_Pointer && clang_getPointeeType( underlying_type ).kind == CXType_FunctionProto ) {
                auto const fcn_name = ts3d::toString( clang_getCursorSpelling( c ) );
                std::cout << "Processing function pointer decl " << fcn_name << std::endl;
                if( fcn_name == "PFA3DMkpRTFGetField" ) {
                    int j = 0;
                    dump(c);
                }
                std::vector<CXCursor> fcn_cursors;
                clang_visitChildren(c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
                    auto &fcn_cursors = *reinterpret_cast<std::vector<CXCursor>*>(client_data);
                    fcn_cursors.push_back( c );
                    return CXChildVisit_Continue;
                }, &fcn_cursors );
                if( fcn_cursors.size() ) {
                    auto ret_val_cursor = fcn_cursors[0];
                    out_stream << "        public delegate " << getCSharpTypeSpelling( ret_val_cursor ) << " " << fcn_name << "(";
                    for( auto idx = 1u; idx < fcn_cursors.size(); ++idx ) {
                        auto &c = fcn_cursors[idx];
                        auto arg_spelling = ts3d::toString( clang_getCursorSpelling( c ) );
                        if( arg_spelling.empty() ) {
                            std::stringstream arg_stream;
                            arg_stream << "arg" << idx;
                            arg_spelling = arg_stream.str();
                        }
                        out_stream << getCursorTypeAsCSharpArgumentType(c) << " " << arg_spelling;
                        if(idx < fcn_cursors.size()-1) {
                            out_stream << ", ";
                        }
                    }
                    out_stream << ");" << std::endl;
                }
            }
        } else if( clang_getCursorKind(c) == CXCursor_VarDecl ) {
            auto const var_name = ts3d::toString( clang_getCursorSpelling( c ) );
            auto const type_cursor = clang_getTypeDeclaration( clang_getCursorType( c ) );
            auto underlying_type = clang_getTypedefDeclUnderlyingType( type_cursor );
            if( underlying_type.kind == CXType_Pointer && clang_getPointeeType( underlying_type ).kind == CXType_FunctionProto ) {
                auto const type_name = ts3d::toString( clang_getTypeSpelling( clang_getCursorType( c ) ) );
                out_stream << "        public static " << type_name << " " << var_name << " = Marshal.GetDelegateForFunctionPointer<" << type_name << ">(Library.A3DGetProcAddress(\"" << var_name << "\", 1));" << std::endl;
            }
        }

        return CXChildVisit_Continue;
    }, &out_stream );
    
    out_stream << "    }" << std::endl;
    out_stream << "}" << std::endl;
    return true;
}

std::string cpp_layer::getInitializeFunctionSpelling( CXCursor struct_decl_cursor ) {
    std::stringstream ss;
    auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(struct_decl_cursor)));
    ss << "Initialize" << struct_type_spelling;
    return ss.str();
}


bool cpp_layer::write( CXCursor c, std::string const header_fn, std::string const cpp_fn ) {
    std::vector<CXCursor> structs_with_size;
    clang_visitChildren(c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
        if( clang_getCursorKind(c) == CXCursor_StructDecl && ts3d::hasField(c, "m_usStructSize") ) {
            reinterpret_cast<std::vector<CXCursor>*>(client_data)->push_back(c);
        }
        return CXChildVisit_Continue;
    }, &structs_with_size );
    
    std::ofstream h_ss( header_fn ), cpp_ss( cpp_fn );
    if( ! h_ss.is_open() ) {
        std::cerr << "Unable to open output file: " << header_fn << std::endl;
        return false;
    }
    
    if( ! cpp_ss.is_open() ) {
        std::cerr << "Unable to open output file: " << cpp_fn << std::endl;
        return false;
    }
    
    h_ss << "#ifdef __WIN32" << std::endl;
    h_ss << "#define DECLSPEC __declspec(dllexport)" << std::endl;
    h_ss << "#else" << std::endl;
    h_ss << "#define DECLSPEC" << std::endl;
    h_ss << "#endif" << std::endl;
    h_ss << "#include \"A3DSDKIncludes.h\"" << std::endl;
    h_ss << std::endl;
    h_ss << "extern \"C\" {" << std::endl;
    h_ss << "    DECLSPEC void GetVersionNumbers( int *major_version, int *minor_version );" << std::endl;
    h_ss << "    DECLSPEC void SetExchangeLibraryFolder( char const *folder );" << std::endl;
    h_ss << "    DECLSPEC void *GetAPILookupFunction( void );" << std::endl;
    
    cpp_ss << "#include \"" << header_fn << "\"" << std::endl;
    cpp_ss << "#include <stdexcept>" << std::endl;
    cpp_ss << "#include <sstream>" << std::endl;
    cpp_ss << "#define INITIALIZE_A3D_API" << std::endl;
    cpp_ss << "#define A3DAPI_GETPROCADDRESS" << std::endl;
    cpp_ss << "#include \"A3DSDKIncludes.h\"" << std::endl;
    cpp_ss << std::endl;
    cpp_ss << "static std::string exchange_folder;" << std::endl;
    cpp_ss << std::endl;
    cpp_ss << "void SetExchangeLibraryFolder( char const *folder ) {" << std::endl;
    cpp_ss << "    exchange_folder = folder;" << std::endl;
    cpp_ss << "}" << std::endl;
    cpp_ss << std::endl;
    cpp_ss << "void *GetAPILookupFunction( void ) {" << std::endl;
    cpp_ss << "    static auto load_status = A3DSDKLoadLibrary( exchange_folder.c_str() );" << std::endl;
    cpp_ss << "    if( nullptr == A3DGetProcAddress && A3D_SUCCESS != load_status ) {" << std::endl;
    cpp_ss << "        std::stringstream ss;" << std::endl;
    cpp_ss << "        ss << \"Unable to load the Exchange libraries, status = \" << load_status;" << std::endl;
    cpp_ss << "        throw std::runtime_error( ss.str() );" << std::endl;
    cpp_ss << "    }" << std::endl;
    cpp_ss << "    return reinterpret_cast<void*>(A3DGetProcAddress);" << std::endl;
    cpp_ss << "}" << std::endl;
    cpp_ss << std::endl;
    cpp_ss << "void GetVersionNumbers( int *major_version, int *minor_version ) {" << std::endl;
    cpp_ss << "    if(major_version) {" << std::endl;
    cpp_ss << "        *major_version = A3D_DLL_MAJORVERSION;" << std::endl;
    cpp_ss << "    }" << std::endl;
    cpp_ss << "    if(minor_version) {" << std::endl;
    cpp_ss << "        *minor_version = A3D_DLL_MINORVERSION;" << std::endl;
    cpp_ss << "    }" << std::endl;
    cpp_ss << "}" << std::endl;
    cpp_ss << std::endl;

    for( auto c : structs_with_size ) {
        auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(c)));
        h_ss << "    DECLSPEC void " << getInitializeFunctionSpelling(c) << "( " << struct_type_spelling << " *ptr );" << std::endl;
        cpp_ss << "void " << getInitializeFunctionSpelling(c) << "( " << struct_type_spelling << " *ptr ) {" << std::endl;
        cpp_ss << "    if( ptr && ptr->m_usStructSize == sizeof( " << struct_type_spelling << " ) ) {" << std::endl;
        cpp_ss << "        A3D_INITIALIZE_DATA( " << struct_type_spelling << ", (*ptr) );" << std::endl;
        cpp_ss << "    }" << std::endl;
        cpp_ss << "}" << std::endl;
        cpp_ss << std::endl;
    }
    h_ss << "}" << std::endl;
    
    return true;
}
