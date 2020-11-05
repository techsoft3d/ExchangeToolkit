#include <sstream>
#include <fstream>
#include "csharp.h"
#include "MeaningfulCursors.h"
#include "util.h"
//#include "clang/Frontend/ASTUnit.h"

using namespace ts3d::exchange::parser::csharp;

namespace {
    std::vector<CXCursor> getStructsWithSize( CXTranslationUnit unit ) {
        auto const c = clang_getTranslationUnitCursor(unit);
        std::vector<CXCursor> structs_with_size;
        clang_visitChildren(c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            if( clang_getCursorKind(c) == CXCursor_StructDecl && ts3d::hasField(c, "m_usStructSize") ) {
                reinterpret_cast<std::vector<CXCursor>*>(client_data)->push_back(c);
            }
            return CXChildVisit_Continue;
        }, &structs_with_size );
        return structs_with_size;
    }

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
            {
                // easiest fix here for deprecated typedef
                auto type_spelling = ts3d::toString( clang_getTypeSpelling( t ) );
                if( type_spelling == "A3DRTFFieldData"  ) {
                    type_spelling = "A3DMkpRTFFieldData";
                }
                return type_spelling;
            }
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
    
    std::string getClass( CXCursor c, std::string const struct_spelling, std::string const class_spelling ) {
        auto const get_spelling = ts3d::exchange::parser::getExchangeToken( struct_spelling, ts3d::exchange::parser::Category::Get );
        auto const has_get = ts3d::MeaningfulCursors::instance().hasCursor( get_spelling );

        std::stringstream ss;
        ss << "    public class " << class_spelling << std::endl;
        ss << "    {" << std::endl;
        ss << "        public " << class_spelling << "()" << std::endl;
        ss << "        {" << std::endl;
        ss << "            API.Initialize(out _d);" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        if(has_get) {
            ss << "        public " << class_spelling << "(IntPtr p)" << std::endl;
            ss << "        {" << std::endl;
            ss << "            API.Initialize(out _d);" << std::endl;
            ss << "            if( p != IntPtr.Zero )" << std::endl;
            ss << "            {" << std::endl;
            ss << "                API." << get_spelling << "( p, ref _d );" << std::endl;
            ss << "            }" << std::endl;
            ss << "        }" << std::endl;
            ss << std::endl;
        }
        ss << "        ~" << class_spelling << "()" << std::endl;
        ss << "        {" << std::endl;
        if(has_get) {
            ss << "            API." << get_spelling << "( IntPtr.Zero, ref _d );" << std::endl;
        }
        ss << "        }" << std::endl;
        clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &ss = *reinterpret_cast<std::stringstream*>(client_data);
            if( clang_getCursorKind( c ) == CXCursor_FieldDecl ) {
                auto const field_decl_type = clang_getCursorType( c );
                auto const field_decl_type_spelling = ts3d::toString( clang_getTypeSpelling( field_decl_type ) );
                auto const canonical_type = clang_getCanonicalType( field_decl_type );
                auto const field_decl_spelling = ts3d::toString( clang_getCursorSpelling( c ) );
                if( CXType_ConstantArray == canonical_type.kind ) {
                    auto const array_as_one_d = getOneDimensionalArray( field_decl_type );
                    auto const field_type_spelling = getCSharpTypeSpelling( array_as_one_d.first );
                    ss << "        public " << field_type_spelling << "[] " << field_decl_spelling << " {" << std::endl;
                    ss << "            get {" << std::endl;
                    ss << "                var result = new " << field_type_spelling << "[" << array_as_one_d.second << "];" << std::endl;
                    ss << "                for( var idx = 0; idx < " << array_as_one_d.second << "; ++idx ) unsafe {" << std::endl;
                    ss << "                    result[idx] = _d." << field_decl_spelling << "[idx];" << std::endl;
                    ss << "                }" << std::endl;
                    ss << "                return result;" << std::endl;
                    ss << "            }" << std::endl;
                    ss << "        }" << std::endl;
                } else {
                    if( field_decl_type_spelling == "A3DBool" ) {
                        ss << "        public bool " << field_decl_spelling << " => _d." << field_decl_spelling << ";" << std::endl;
                    } else if( field_decl_type_spelling == "A3DUTF8Char *" ) {
                        ss << "        public string " << field_decl_spelling << " => _d." << field_decl_spelling << ";" << std::endl;
                    } else {
                        ss << "        public " << getCSharpTypeSpelling( c ) << " " << field_decl_spelling << "=>_d." << field_decl_spelling << ";" << std::endl;
                    }
                }
            } else if( clang_getCursorKind( c ) == CXCursor_UnionDecl) {
                clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data ) {
                    auto &ss = *reinterpret_cast<std::stringstream*>(client_data);
                    auto const field_spelling = ts3d::toString(clang_getCursorSpelling( c ) );
                    auto const field_type_spelling = ts3d::toString(clang_getTypeSpelling( clang_getCursorType(c)));
                    ss << "        public " << field_type_spelling << " " << field_spelling << " => _d." << field_spelling << ";" << std::endl;
                    return CXChildVisit_Continue;
                }, &ss);
            } else {
                throw std::runtime_error("Encountered unexpected cursor type inside enum decl." );
            }
            return CXChildVisit_Continue;
        }, &ss);
        ss << std::endl;
        ss << "        private " << struct_spelling << " _d;" << std::endl;
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
    
    
    void writeIntermediateFile( CXTranslationUnit unit, std::string const fn ) {
        std::ofstream out( fn );
        out << "#include \"A3DSDKIncludes.h\"" << std::endl;
        auto const constants = {
            "A3D_DLL_MAJORVERSION",
            "A3D_DLL_MINORVERSION",
            "A3D_DEFAULT_LAYER",
            "A3D_DEFAULT_TRANSPARENCY",
            "A3D_DEFAULT_LINE_WIDTH",
            "A3D_DEFAULT_COLOR_INDEX",
            "A3D_DEFAULT_PATTERN_INDEX",
            "A3D_DEFAULT_STYLE_INDEX",
            "A3D_DEFAULT_LINEPATTERN_INDEX",
            "A3D_DEFAULT_MATERIAL_INDEX",
            "A3D_DEFAULT_PICTURE_INDEX",
            "A3D_DEFAULT_TEXTURE_DEFINITION_INDEX",
            "A3D_DEFAULT_TEXTURE_APPLICATION_INDEX",
            "A3D_DEFAULT_NO_UNIT",
            "A3D_LOOP_UNKNOWN_OUTER_INDEX"
        };
        for( auto constant : constants ) {
            auto csharp_constant = std::regex_replace( constant, std::regex("A3D_"), "" );
            out << "static auto " << csharp_constant << " = " << constant << ";" << std::endl;
        }
        
        for( auto struct_cursor : getStructsWithSize( unit ) ) {
            auto const struct_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(struct_cursor) ) );
            auto const initialize_fcn_spelling = "A3D_INITIALIZE_" + struct_spelling;
            out << "void Initialize(" << struct_spelling << " &d ) { " << initialize_fcn_spelling << "(d); } " << std::endl;
        }
    }
    
    void processIntermediateVarDecl( CXCursor var_decl, std::ostream &out_stream ) {
        auto const var_spelling = ts3d::toString( clang_getCursorSpelling( var_decl ) );
        auto const csharp_type_spelling = getCSharpTypeSpelling( clang_getCursorType( var_decl ) );
        CXCursorKind rhs_kind = CXCursor_FirstInvalid;
        clang_visitChildren( var_decl, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto const k = clang_getCursorKind(c);
            if( k == CXCursor_IntegerLiteral || k == CXCursor_FloatingLiteral ||k == CXCursor_StringLiteral ) {
                *reinterpret_cast<CXCursorKind*>(client_data) = k;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Recurse;
        }, &rhs_kind );
        
        auto const evaluation = clang_Cursor_Evaluate( var_decl );
        std::stringstream value_stream;
        switch( rhs_kind ) {
            case CXCursor_IntegerLiteral:
            {
                auto value = clang_EvalResult_getAsInt( evaluation );
                if( -1 == value || std::numeric_limits<int16_t>::max() ) {
                    if( csharp_type_spelling == "uint" || csharp_type_spelling == "ushort" ) {
                        value_stream << csharp_type_spelling << ".MaxValue";
                        break;
                    }
                }
                value_stream << value;
            }
                break;
            case CXCursor_FloatingLiteral:
                value_stream << clang_EvalResult_getAsDouble( evaluation );
                break;
            case CXCursor_StringLiteral:
                value_stream << clang_EvalResult_getAsStr( evaluation );
                break;
            default:
                throw std::runtime_error("Unable to obtain rhs value." );
                break;
        }
        
        out_stream  << "    public static " << csharp_type_spelling << " " << var_spelling << " = " << value_stream.str() << ";" << std::endl;
    }
    
    std::vector<CXCursor> getRefExprs( CXCursor c ) {
        std::vector<CXCursor> member_ref_exprs;
        auto const k = clang_getCursorKind( c );
        if( CXCursor_MemberRefExpr == k || CXCursor_DeclRefExpr == k ) {
            member_ref_exprs.push_back( c );
        }
        clang_visitChildren( c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &member_ref_exprs = *reinterpret_cast<std::vector<CXCursor>*>(client_data);
            auto const k = clang_getCursorKind( c );
            if( CXCursor_MemberRefExpr == k || CXCursor_DeclRefExpr == k ) {
                member_ref_exprs.push_back( c );
            }
            return CXChildVisit_Recurse;
        }, &member_ref_exprs );
        return member_ref_exprs;
    }
    
    std::string processIntermediateBinaryOperatorLHS( CXTranslationUnit unit, CXCursor lhs ) {
        std::vector<CXCursor> lhs_refs;
        clang_visitChildren( lhs, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &lhs_refs = *reinterpret_cast<std::vector<CXCursor>*>(client_data);
            if( CXCursor_MemberRefExpr == clang_getCursorKind( c ) ||
               CXCursor_DeclRefExpr == clang_getCursorKind( c ) ) {
                lhs_refs.insert( lhs_refs.begin(), c );
            }
            return CXChildVisit_Recurse;
        }, &lhs_refs );
        lhs_refs.push_back( lhs );
        
        std::string member_spelling;
        for( auto const c : lhs_refs ) {
            if( !member_spelling.empty() ) {
                member_spelling += ".";
            }
            member_spelling += ts3d::toString( clang_getCursorSpelling(c) );
        }
        
        return member_spelling;
    }
    
    std::string processIntermediateBinaryOperatorRHS( CXTranslationUnit unit, CXCursor rhs ) {
        std::string rhs_spelling;
        auto const rhs_kind = clang_getCursorKind( rhs );
        if( rhs_kind == CXCursor_FloatingLiteral ||
           rhs_kind == CXCursor_IntegerLiteral ) {
            auto const extent = clang_getCursorLocation(rhs);
            auto token = clang_getToken(unit, extent);
            if( nullptr == token ) {
                throw std::runtime_error("Unable to obtain rhs value." );
            }
            rhs_spelling = ts3d::toString( clang_getTokenSpelling(unit, *token));
        } else if(rhs_kind == CXCursor_DeclRefExpr ){
            auto const cursor_type = clang_getCursorType( rhs );
            auto const type_spelling = ts3d::toString(clang_getTypeSpelling(cursor_type));
            auto const value_spalling = ts3d::toString(clang_getCursorSpelling(rhs));
            rhs_spelling = type_spelling + "." + value_spalling;
        } else if( rhs_kind == CXCursor_UnexposedExpr ) {
            auto const extent = clang_getCursorLocation(rhs);
            if( auto token = clang_getToken(unit, extent) ) {
                rhs_spelling = ts3d::toString( clang_getTokenSpelling(unit, *token));
            }
        } else {
            dump(rhs);
        }
        return rhs_spelling;
    }
    
    std::pair<std::string, std::string> processIntermediateBinaryOperator( CXTranslationUnit unit, CXCursor binary_op, std::set<std::string> &members_to_initialize ) {
        auto lhs_rhs = std::make_pair(clang_getNullCursor(), clang_getNullCursor());
        clang_visitChildren( binary_op, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &lhs_rhs = *reinterpret_cast<std::pair<CXCursor,CXCursor>*>(client_data);
            if( clang_Cursor_isNull( lhs_rhs.first ) ) {
                lhs_rhs.first = c;
            } else {
                lhs_rhs.second = c;
            }
            return CXChildVisit_Continue;
        }, &lhs_rhs );
        
        auto const lhs = lhs_rhs.first;
        if( lhs.kind != CXCursor_MemberRefExpr ) {
            throw std::runtime_error("Unhandled lhs of binary operator." );
        }
        
        std::string lhs_spelling;
        auto const lhs_ref_exprs = getRefExprs( lhs );
        if( lhs_ref_exprs.size() > 2 ) {
            // something like d.m_sValidationPropertiesThreshold.m_dGEOMPercentVolume
            // we want to transform these occurrences to a single API.Initialize( out d.m_sValidationPropertiesThreshold );
            
            std::stringstream ss;
            ss << ts3d::toString( clang_getCursorSpelling( lhs_ref_exprs.back() ) ) ;
            ss << ".";
            ss << ts3d::toString( clang_getCursorSpelling( lhs_ref_exprs[lhs_ref_exprs.size()-2] ) ) ;
            if( ! std::regex_match( ss.str(), std::regex(".*\\.m_sOrthotropic3D" ) ) ) {
                // skip the weird union member situation for now
                members_to_initialize.insert( ss.str() );
            }
            return std::pair<std::string, std::string>();
        } else {
            lhs_spelling = processIntermediateBinaryOperatorLHS(unit, lhs);
        }
        
        if( std::regex_match( lhs_spelling, std::regex( ".*\\.m_usStructSize" ) ) ) {
            return std::pair<std::string, std::string>();
        }
        
        std::string rhs_spelling;
        auto const rhs = lhs_rhs.second;
        if( clang_Cursor_isNull(rhs)) {
            throw std::runtime_error("RHS is null!");
        }
        
        rhs_spelling = processIntermediateBinaryOperatorRHS( unit, rhs );
        if( rhs_spelling.empty() ) {
            if( std::regex_match( lhs_spelling, std::regex( ".*m_bExportNormalsWithTessellation" ) ) ) {
                rhs_spelling = "true";
            } else if( std::regex_match(lhs_spelling, std::regex(".*m_usUnit" ) ) ) {
                rhs_spelling = "DEFAULT_NO_UNIT";
            } else if( std::regex_match(lhs_spelling, std::regex(".*m_bIncludeHiddenRIs" ) ) ) {
                rhs_spelling = "false";
            } else {
                int j = 0;
            }
        }
        
        return std::make_pair(lhs_spelling, rhs_spelling);
    }
    
    
    void processIntermediateCompoundStmt( CXTranslationUnit unit, CXCursor compound_stmt, std::ostream &out_stream ) {
        std::vector<CXCursor> binary_operators; // assignments
        clang_visitChildren( compound_stmt, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            if( c.kind == CXCursor_BinaryOperator ) {
                reinterpret_cast<std::vector<CXCursor>*>(client_data)->push_back(c);
            }
            return CXChildVisit_Recurse;
        }, &binary_operators );
        
        std::set<std::string> members_to_initialize;
        std::vector<std::string> assignments;
        for( auto const binary_op : binary_operators ) {
            auto const lhs_rhs_spelling = processIntermediateBinaryOperator( unit, binary_op, members_to_initialize );
            if( !lhs_rhs_spelling.first.empty() && !lhs_rhs_spelling.second.empty() ) {
                std::stringstream ss;
                ss << lhs_rhs_spelling.first << " = " << lhs_rhs_spelling.second << ";";
                assignments.push_back( ss.str() );
            }
        }
        
        for( auto const &member_to_initialize : members_to_initialize ) {
            out_stream << "        API.Initialize( out " << member_to_initialize << " );" << std::endl;
        }
        
        for( auto const &assignment : assignments ) {
            out_stream << "        " << assignment << std::endl;
        }
        
    }
    
    void processIntermediateFunctionDecl( CXTranslationUnit unit, CXCursor function_decl, std::ostream &out_stream ) {
        auto const function_spelling = ts3d::toString( clang_getCursorSpelling( function_decl ) );
        auto parm_decl_and_compound_stmt = std::make_pair( clang_getNullCursor(), clang_getNullCursor() );
        clang_visitChildren( function_decl, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto &parm_decl_and_compound_stmt = *reinterpret_cast<std::pair<CXCursor, CXCursor>*>(client_data);
            if( c.kind == CXCursor_ParmDecl ) {
                parm_decl_and_compound_stmt.first = c;
            } else if( c.kind == CXCursor_CompoundStmt ) {
                parm_decl_and_compound_stmt.second = c;
            }
            return CXChildVisit_Continue;
        }, &parm_decl_and_compound_stmt );
        
        auto const parm_decl = parm_decl_and_compound_stmt.first;
        if( clang_Cursor_isNull( parm_decl ) ) {
            throw std::runtime_error("Unable to find function parameter.");
        }
        
        auto const parm_spelling = ts3d::toString( clang_getCursorSpelling( parm_decl ) );
        CXCursor parm_typeref = clang_getNullCursor();
        clang_visitChildren( parm_decl, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            if( c.kind == CXCursor_TypeRef ) {
                *reinterpret_cast<CXCursor*>(client_data) = c;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }, &parm_typeref );
        if( clang_Cursor_isNull(parm_typeref ) ){
            throw std::runtime_error("Unable to find function parameter type.");
        }
        auto const parm_type_spelling = ts3d::toString( clang_getCursorSpelling( parm_typeref ) );
        
        out_stream << "    public static void " << function_spelling << "(out " << parm_type_spelling << " " << parm_spelling << " ) { " << std::endl;
        out_stream << "        " << parm_spelling << " = new " << parm_type_spelling << "();" << std::endl;
        out_stream << "        " << parm_spelling << ".m_usStructSize = (UInt16)Marshal.SizeOf(typeof(" << parm_type_spelling << "));" << std::endl;
        
        auto const compound_stmt = parm_decl_and_compound_stmt.second;
        if( clang_Cursor_isNull( compound_stmt ) ) {
            throw std::runtime_error("Unable to find initializer function.");
        }

        processIntermediateCompoundStmt( unit, compound_stmt, out_stream );
        
        out_stream << "    }" << std::endl;
    }
    
    void processIntermediateCursor( CXTranslationUnit unit, CXCursor c, std::ostream &out_stream ) {
        switch( clang_getCursorKind(c)) {
            case CXCursor_VarDecl:
                processIntermediateVarDecl( c, out_stream );
                break;
            case CXCursor_FunctionDecl:
                processIntermediateFunctionDecl( unit, c, out_stream );
                break;
            default:
                break;
        }
    }
    
    void processIntermediateTranslationUnit( CXTranslationUnit unit, CXFile file, std::ostream &out_stream ) {
        clang_visitChildren( clang_getTranslationUnitCursor(unit), [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto const loc = clang_getCursorLocation( c );
            if( clang_Location_isFromMainFile( loc ) ) {
                auto &out_stream = *reinterpret_cast<std::ostream*>(client_data);
                processIntermediateCursor( clang_Cursor_getTranslationUnit(c), c, out_stream );
            }
            return CXChildVisit_Continue;
        }, &out_stream);
    }
    
    void writeCSharpStructInitializers( CXTranslationUnit unit, std::ostream &out_stream ) {
        std::string const fn = "/tmp/foo.cpp";
        writeIntermediateFile( unit, fn);
                
        auto index = clang_createIndex(0, 0);
        char const *cmd_line_option = "-I/opt/local/ts3d/HOOPS_Exchange_2020_SP2_U2/include";
        auto intermediate_unit = clang_parseTranslationUnit( index, fn.c_str(), &cmd_line_option, 1, nullptr, 0, CXTranslationUnit_DetailedPreprocessingRecord);
        auto file = clang_getFile( intermediate_unit, fn.c_str() );
        processIntermediateTranslationUnit( intermediate_unit, file, out_stream );
        clang_disposeTranslationUnit( intermediate_unit );
        clang_disposeIndex( index );
    }
}

bool ts3d::exchange::parser::csharp::direct::writeStructs( CXTranslationUnit unit, std::string const fn ) {
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

bool ts3d::exchange::parser::csharp::direct::writeEnums( CXTranslationUnit unit, std::string const fn ) {
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

bool ts3d::exchange::parser::csharp::direct::writeAPI( CXTranslationUnit unit, std::string const fn ) {
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
                std::vector<CXCursor> fcn_cursors;
                clang_visitChildren(c, [](CXCursor c, CXCursor parent, CXClientData client_data) {
                    auto &fcn_cursors = *reinterpret_cast<std::vector<CXCursor>*>(client_data);
                    fcn_cursors.push_back( c );
                    return CXChildVisit_Continue;
                }, &fcn_cursors );
                if( fcn_cursors.size() ) {
                    auto ret_val_cursor = fcn_cursors[0];
                    if( fcn_name == "PFA3DMiscGetErrorMsg" ) {
                        int j = 0;
                    }
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

//std::string cpp_layer::getInitializeFunctionSpelling( CXCursor struct_decl_cursor ) {
//    std::stringstream ss;
//    auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(struct_decl_cursor)));
//    ss << "Initialize" << struct_type_spelling;
//    return ss.str();
//}



//bool cpp_layer::write( CXTranslationUnit unit, std::string const header_fn, std::string const cpp_fn ) {
//
//    std::ofstream h_ss( header_fn ), cpp_ss( cpp_fn );
//    if( ! h_ss.is_open() ) {
//        std::cerr << "Unable to open output file: " << header_fn << std::endl;
//        return false;
//    }
//
//    if( ! cpp_ss.is_open() ) {
//        std::cerr << "Unable to open output file: " << cpp_fn << std::endl;
//        return false;
//    }
//
//    h_ss << "#ifdef __WIN32" << std::endl;
//    h_ss << "#define DECLSPEC __declspec(dllexport)" << std::endl;
//    h_ss << "#else" << std::endl;
//    h_ss << "#define DECLSPEC" << std::endl;
//    h_ss << "#endif" << std::endl;
//    h_ss << std::endl;
//    h_ss << "extern \"C\" {" << std::endl;
//    h_ss << "    DECLSPEC void GetVersionNumbers( int *major_version, int *minor_version );" << std::endl;
//    h_ss << "    DECLSPEC void SetExchangeLibraryFolder( char const *folder );" << std::endl;
//    h_ss << "    DECLSPEC void *GetAPILookupFunction( void );" << std::endl;
//
//    cpp_ss << "#include <stdexcept>" << std::endl;
//    cpp_ss << "#include <sstream>" << std::endl;
//    cpp_ss << "#define INITIALIZE_A3D_API" << std::endl;
//    cpp_ss << "#define A3DAPI_GETPROCADDRESS" << std::endl;
//    cpp_ss << "#include \"A3DSDKIncludes.h\"" << std::endl;
//    cpp_ss << "#include \"" << header_fn << "\"" << std::endl;
//    cpp_ss << std::endl;
//    cpp_ss << "static std::string exchange_folder;" << std::endl;
//    cpp_ss << std::endl;
//    cpp_ss << "void SetExchangeLibraryFolder( char const *folder ) {" << std::endl;
//    cpp_ss << "    exchange_folder = folder;" << std::endl;
//    cpp_ss << "}" << std::endl;
//    cpp_ss << std::endl;
//    cpp_ss << "void *GetAPILookupFunction( void ) {" << std::endl;
//    cpp_ss << "    static auto load_status = A3DSDKLoadLibrary( exchange_folder.c_str() );" << std::endl;
//    cpp_ss << "    if( nullptr == A3DGetProcAddress && A3D_SUCCESS != load_status ) {" << std::endl;
//    cpp_ss << "        std::stringstream ss;" << std::endl;
//    cpp_ss << "        ss << \"Unable to load the Exchange libraries, status = \" << load_status;" << std::endl;
//    cpp_ss << "        throw std::runtime_error( ss.str() );" << std::endl;
//    cpp_ss << "    }" << std::endl;
//    cpp_ss << "    return reinterpret_cast<void*>(A3DGetProcAddress);" << std::endl;
//    cpp_ss << "}" << std::endl;
//    cpp_ss << std::endl;
//    cpp_ss << "void GetVersionNumbers( int *major_version, int *minor_version ) {" << std::endl;
//    cpp_ss << "    if(major_version) {" << std::endl;
//    cpp_ss << "        *major_version = A3D_DLL_MAJORVERSION;" << std::endl;
//    cpp_ss << "    }" << std::endl;
//    cpp_ss << "    if(minor_version) {" << std::endl;
//    cpp_ss << "        *minor_version = A3D_DLL_MINORVERSION;" << std::endl;
//    cpp_ss << "    }" << std::endl;
//    cpp_ss << "}" << std::endl;
//    cpp_ss << std::endl;
//
//    for( auto c : getStructsWithSize(unit) ) {
//        auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(c)));
//        h_ss << "    DECLSPEC bool " << getInitializeFunctionSpelling(c) << "( " << struct_type_spelling << " *ptr );" << std::endl;
//        cpp_ss << "bool " << getInitializeFunctionSpelling(c) << "( " << struct_type_spelling << " *ptr ) {" << std::endl;
//        cpp_ss << "    if( ptr && ptr->m_usStructSize == sizeof( " << struct_type_spelling << " ) ) {" << std::endl;
//        cpp_ss << "        A3D_INITIALIZE_DATA( " << struct_type_spelling << ", (*ptr) );" << std::endl;
//        cpp_ss << "        return true;" << std::endl;
//        cpp_ss << "    }" << std::endl;
//        cpp_ss << "    return false;" << std::endl;
//        cpp_ss << "}" << std::endl;
//        cpp_ss << std::endl;
//    }
//    h_ss << "}" << std::endl;
//
//    return true;
//}

//bool ts3d::exchange::parser::csharp::direct::writeStructSizeTests( CXTranslationUnit unit, std::string const fn ) {
//    
//    foo(unit);
//    auto const structs_with_size = getStructsWithSize( unit );
//    for( auto c : structs_with_size ) {
//        auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(c)));
//    }
//    std::ofstream ss( fn );
//    if( !ss.is_open() ) {
//        std::cerr << "Unable to open output file: " << fn << std::endl;
//        return false;
//    }
//    
//    ss << "using System;" << std::endl;
//    ss << std::endl;
//    ss << "namespace TS3D.Exchange.Direct" << std::endl;
//    ss << "{" << std::endl;
//    ss << "    public class Test" << std::endl;
//    ss << "    {" << std::endl;
//    ss << "        public static bool PerformStructSizeTests()" << std::endl;
//    ss << "        {" << std::endl;
//    ss << "            try {" << std::endl;
//    for( auto const c : structs_with_size ) {
//        auto const struct_type_spelling = ts3d::toString( clang_getTypeSpelling( clang_getCursorType(c)));
//
//        ss << "                {" << std::endl;
//        ss << "                    " << struct_type_spelling << " d;" << std::endl;
//        ss << "                    if( !API.Initialize( out d ) ) {" << std::endl;
//        ss << "                        throw new InvalidOperationException( \"Struct sizes do not match for type " << struct_type_spelling << "\" );" << std::endl;
//        ss << "                    }" << std::endl;
//        ss << "                }" << std::endl;
//    }
//    ss << "            } catch( InvalidOperationException e ) {" << std::endl;
//    ss << "                return false;" << std::endl;
//    ss << "            }" << std::endl;
//    ss << "            return true;" << std::endl;
//    ss << "        }" << std::endl;
//    ss << "    }" << std::endl;
//    ss << "}" << std::endl;
//
//
//    return true;
//}

bool ts3d::exchange::parser::csharp::direct::writeClasses( CXTranslationUnit unit, std::string const fn ) {
    auto const structs_with_size = getStructsWithSize( unit );
    std::ofstream ss( fn );
    if( !ss.is_open() ) {
        std::cerr << "Unable to open output file: " << fn << std::endl;
        return false;
    }
    
    ss << "using System;" << std::endl;
    ss << std::endl;
    ss << "namespace TS3D.Exchange.Direct" << std::endl;
    ss << "{" << std::endl;
    
    for( auto c : structs_with_size ) {
        auto const struct_decl_cursor = c;
        auto const struct_spelling = ts3d::toString(clang_getTypeSpelling(clang_getCursorType(struct_decl_cursor)));

        auto class_spelling = std::regex_replace( struct_spelling, std::regex("Data$"), "Wrapper" );
        if(class_spelling == struct_spelling ) {
            // A3DAsmProductOccurrenceDataSLW
            class_spelling = struct_spelling + "Wrapper";
        }
        ss << getClass(struct_decl_cursor, struct_spelling, class_spelling );
    }
    
    ss << "}" << std::endl;
    ss << std::endl;
    return true;

}
