//
//  wrapper.cpp
//  Parser
//
//  Created by Brad Flubacher on 5/8/20.
//  Copyright © 2020 Brad Flubacher. All rights reserved.
//
#include <sstream>
#include "MeaningfulCursors.h"
#include "config.h"
#include "util.h"
#include "raii.h"
#include "wrapper.h"

using namespace ts3d::exchange::parser;

wrapper &wrapper::instance( void ) {
    static wrapper _instance;
    
    return _instance;
}

namespace {
    class FieldMetaData {
    public:
        using FieldMetaDataPtr = std::shared_ptr<FieldMetaData>;
        
        FieldMetaData( CXCursor field_cursor ) : _cursor( field_cursor ) {
        }
        
        FieldMetaData( void ) : _cursor( clang_getNullCursor() ) {
            
        }
        
        CXCursor getCursor( void ) const {
            return _cursor;
        }
        
        void setCursor( CXCursor c ) {
            _cursor = c;
        }
        
        virtual ts3d::CursorSet getAllCursors( void ) const {
            ts3d::CursorSet result;
            if( !clang_Cursor_isNull(_cursor ) ) {
                result.insert( _cursor );
            }
            if( _array_size ) {
                auto const array_cursors = _array_size->getAllCursors();
                result.insert(std::begin(array_cursors), std::end(array_cursors));
            }
            return result;
        }

        virtual std::string spelling( void ) const {
            return ts3d::toString( clang_getCursorSpelling( _cursor ) );

        }

        std::string type_spelling( void ) const {
            return ts3d::toString( clang_getTypeSpelling( clang_getCursorType( _cursor ) ) );
        }
        
        void setArrayFieldSizeCursor( CXCursor array_size_field_cursor ) {
            if( clang_Cursor_isNull(array_size_field_cursor ) ) {
                _array_size.reset();
            } else {
                _array_size = std::make_shared<FieldMetaData>(array_size_field_cursor);
            }
        }
        
        void setArrayFieldSizeMetaData( FieldMetaDataPtr array_size_field_meta_data ) {
            _array_size = array_size_field_meta_data;
        }
        
        bool isArray( void ) const {
            return nullptr != _array_size && !clang_Cursor_isNull(_cursor);
        }
        
        FieldMetaDataPtr getArraySizeFieldMetaData( void ) const {
            return _array_size;
        }
        
    protected:
        CXCursor _cursor;
        FieldMetaDataPtr _array_size;
    };
    
    class ConstantFieldMetaData : public FieldMetaData {
    public:
        ConstantFieldMetaData( std::string constant_spelling ) :
        FieldMetaData( clang_getNullCursor() ),
        _constant_spelling( constant_spelling ) {
            
        }
        
        virtual std::string spelling( void ) const override {
            return _constant_spelling;
        }
        
    private:
        std::string _constant_spelling;
    };
    
    class BinaryFieldMetaData : public FieldMetaData {
    public:
        BinaryFieldMetaData( FieldMetaDataPtr lhs, FieldMetaDataPtr rhs ) : _lhs( lhs ), _rhs( rhs ){
        }
        
        FieldMetaDataPtr getLHS( void ) const { return _lhs; }
        FieldMetaDataPtr getRHS( void ) const { return _rhs; }
        
        virtual ts3d::CursorSet getAllCursors( void ) const override {
            ts3d::CursorSet result;
            if( _lhs ) {
                auto lhs_cursors = _lhs->getAllCursors();
                result.insert( std::begin( lhs_cursors ), std::end( lhs_cursors ) );
            }
            if( _rhs ) {
                auto rhs_cursors = _rhs->getAllCursors();
                result.insert( std::begin( rhs_cursors ), std::end( rhs_cursors ) );
            }
            return result;
        }

        virtual std::string spelling( void ) const override = 0;
        
    protected:
        FieldMetaDataPtr _lhs;
        FieldMetaDataPtr _rhs;
    };
    
    class MultiplicationFieldMetaData : public BinaryFieldMetaData {
    public:
        MultiplicationFieldMetaData( FieldMetaDataPtr lhs, FieldMetaDataPtr rhs ) : BinaryFieldMetaData( lhs, rhs ) {
        }
        virtual std::string spelling( void ) const override {
            if( nullptr == _lhs || nullptr == _rhs ) {
                return std::string();
            }
            return _lhs->spelling() + "*" + _rhs->spelling();
        }
    };
    
    CXCursor determineSizeField( ts3d::CursorSet all_ui_field_cursors, CXCursor array_cursor ) {
        auto const field_spelling = ts3d::toString( clang_getCursorSpelling( array_cursor ) );
        auto const field_name_without_m_ = ts3d::removeHungarianNotation( field_spelling );
        auto best_similarity_metric = 0.;
        CXCursor best_similarity_field_cursor;
        for( auto const ui_field_cursor : all_ui_field_cursors ) {
            auto const potential_size_field = ts3d::toString( clang_getCursorSpelling( ui_field_cursor ) );
            auto const metric = ts3d::getSimilarityMetric(potential_size_field, field_spelling );
            if( metric > best_similarity_metric ) {
                best_similarity_metric = metric;
                best_similarity_field_cursor = ui_field_cursor;
            }
        }

        if( best_similarity_metric < .5 ) {
            return clang_getNullCursor();
        }

        return best_similarity_field_cursor;
    }

    FieldMetaData::FieldMetaDataPtr getFieldMetaDataFromSpelling( std::string const &field_spelling, ts3d::CursorSet const &field_cursors ) {
        auto const field_spelling_without_spaces = std::regex_replace( field_spelling, std::regex("[[:space:]]"), "" );
        std::smatch matches;
        if( std::regex_match( field_spelling, matches, std::regex("(.*)\\*(.*)")) ) {
            auto const lhs = getFieldMetaDataFromSpelling( matches[1].str(), field_cursors );
            auto const rhs = getFieldMetaDataFromSpelling( matches[2].str(), field_cursors );
            if( nullptr != lhs && nullptr != rhs ) {
                return std::make_shared<MultiplicationFieldMetaData>(lhs, rhs);
            } else {
                throw std::runtime_error("Unable to parse field spelling from configuration file:" + field_spelling );
            }
        } else {
            if( std::regex_match( field_spelling_without_spaces, std::regex("[0-9\\.]+") ) ) {
                // field size specification is a contant
                return std::make_shared<ConstantFieldMetaData>(field_spelling_without_spaces);
            } else {
                for( auto field_cursor : field_cursors ) {
                    auto const this_field_spelling = ts3d::toString( clang_getCursorSpelling( field_cursor ) );
                    if( this_field_spelling == field_spelling_without_spaces ) {
                        return std::make_shared<FieldMetaData>(field_cursor);
                    }
                }
            }
        }
        return nullptr;
    }
    
    std::vector<FieldMetaData::FieldMetaDataPtr> extractArrayFieldMetaData( ts3d::CursorSet const &all_field_cursors, std::unordered_map<Config::ArrayFieldSpelling, Config::ArraySizeFieldSpelling> const &array_and_size_fields ) {
        std::cout << "[START] Extracting array field meta data." << std::endl;
        std::vector<FieldMetaData::FieldMetaDataPtr> array_fields;
        auto mutable_all_fields_cursors = all_field_cursors;

        std::cout << "\tApplying overrides from config file." << std::endl;
        for( auto const &array_size_spec : array_and_size_fields ) {
            auto field = getFieldMetaDataFromSpelling( array_size_spec.first, all_field_cursors );
            if( nullptr == field ) {
                throw std::runtime_error("Invalid array field specification: " + array_size_spec.first);
            }
            
            if( array_size_spec.second.empty() ) {
                std::cout << "\t\t" << field->spelling() << "[]" << std::endl;
            } else {
                field->setArrayFieldSizeMetaData( getFieldMetaDataFromSpelling( array_size_spec.second, all_field_cursors ) );
            
                if( !field->isArray() ) {
                    throw std::runtime_error("Invalid array size field specification: " + array_size_spec.second);
                }
            
                std::cout << "\t\t" << field->spelling() << "[" << field->getArraySizeFieldMetaData()->spelling() << "]" << std::endl;
            }
            
            for( auto const used_cursor : field->getAllCursors() ) {
                mutable_all_fields_cursors.erase( used_cursor );
            }

            array_fields.emplace_back( std::move( field ) );
        }
        
        std::cout << "\tFinding possible array size fields." << std::endl;
        ts3d::CursorSet potential_array_size_field_cursors;
        for( auto field_cursor : all_field_cursors ) {
            auto const field_spelling = ts3d::toString( clang_getCursorSpelling( field_cursor ) );
            auto const cursor_type = clang_getCursorType( field_cursor );
            if( cursor_type.kind == CXType_Typedef && clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration( cursor_type ) ).kind == CXType_UInt ) {
                if( field_spelling != "m_usStructSize" ) {
                    potential_array_size_field_cursors.insert( field_cursor );
                }
                mutable_all_fields_cursors.erase( field_cursor );
            }
        }
        std::cout << "\tFound " << potential_array_size_field_cursors.size() << " potential array size fields." << std::endl;
        std::cout << "\tAttempting to match array fields with array size fields." << std::endl;
        for( auto field_cursor : mutable_all_fields_cursors ) {
            auto const cursor_type = clang_getCursorType( field_cursor );
            if( cursor_type.kind != CXType_Pointer ) {
                continue;
            }
            
            auto field = std::make_shared<FieldMetaData>( field_cursor );
            
            auto const pointee_type = clang_getPointeeType( cursor_type );
            if( pointee_type.kind == CXType_Pointer ) {
                // double pointer
                auto const pointee_pointee_type = clang_getPointeeType( pointee_type );
                if( pointee_pointee_type.kind == CXType_Typedef &&
                   (clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration( pointee_pointee_type ) ).kind == CXType_Void ||
                    clang_getTypedefDeclUnderlyingType( clang_getTypeDeclaration( pointee_pointee_type ) ).kind == CXType_Char_S )) {
                    // double pointer to void type
                    // must determine size field
                    auto const size_field_cursor = determineSizeField( potential_array_size_field_cursors, field_cursor );
                    if( clang_Cursor_isNull( size_field_cursor ) ) {
                        // couldn't figure out a size field!
                        determineSizeField( potential_array_size_field_cursors, field_cursor );
                        throw std::runtime_error("Unable to determine size field for array pointer: " + field->spelling() );
                    }
                    potential_array_size_field_cursors.erase( size_field_cursor );
                    field->setArrayFieldSizeCursor( size_field_cursor );
                    array_fields.push_back( field );
                } else {
                    throw std::runtime_error("Not sure what to do with a n-dimensional non-void-type field.");
                }
            } else {
                // single pointer
                if( pointee_type.kind == CXType_Typedef && clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration( pointee_type ) ).kind == CXType_Void ) {
                    // single pointer to void type
                    auto const pointee_spelling = ts3d::toString( clang_getTypeSpelling( pointee_type ) );
                    auto const data_type_spelling = pointee_spelling + "Data";
                    if( ts3d::MeaningfulCursors::instance().hasCursor( data_type_spelling ) ) {
                        // In this block, we know it is a single pointer to a void type
                        // that has a corresponding "Data" struct.
                        
                        // no need for size field
                        
                        // wrapper accessor
                    }
                } else {
                    // single pointer to non-void type
                    // is this just a string?
                    if( pointee_type.kind == CXType_Typedef && clang_getTypedefDeclUnderlyingType( clang_getTypeDeclaration( pointee_type ) ).kind == CXType_Char_S ) {
                        // it is! it IS just a string...
                    } else {
                        auto const size_field_cursor = determineSizeField( potential_array_size_field_cursors, field_cursor );
                        if( clang_Cursor_isNull( size_field_cursor ) ) {
                            // couldn't figure out a size field!
                            throw std::runtime_error("Unable to determine size field for array pointer: " + field->spelling() );
                        }
                        potential_array_size_field_cursors.erase( size_field_cursor );
                    
                        // Plain array accessor
                        field->setArrayFieldSizeCursor(size_field_cursor);
                        array_fields.push_back( field );
                    }
                }
            }
        }
        std::cout << "\tMatched " << array_fields.size() << " array fields with size fields." << std::endl;

        std::cout << "[FINISH] Extracting array field meta data." << std::endl;
        return array_fields;
    }
}

std::string wrapper::codeSpelling( CXCursor a3d_data_cursor ) const {
    auto const struct_spelling = toString( clang_getTypeSpelling( clang_getCursorType( a3d_data_cursor ) ) );
    std::cout << "Processing: " << struct_spelling << std::endl;
    auto const array_field_size_overrides = Config::instance().getArrayAndSizeFieldOverrides( struct_spelling );

    std::vector<FieldMetaData> all_field_metadata;
    auto field_cursors_to_process = ts3d::getAllFieldCursors( a3d_data_cursor );
    
    // take care of array fields
    auto fields = extractArrayFieldMetaData( field_cursors_to_process, array_field_size_overrides );
    
    // remove all the cursors that are used by array fields and their size field(s)
    for( auto const &field : fields ) {
        auto const cursors = field->getAllCursors();
        for( auto const cursor : cursors ) {
            field_cursors_to_process.erase( cursor );
        }
    }
    
    // the remaining cursors will be treated as atomic field values, not arrays
    for( auto remaining_cursor : field_cursors_to_process ) {
        auto const field = std::make_shared<FieldMetaData>( remaining_cursor );
        // skip inclusion of the struct size field. This is an arbitrary decision to clean up the interface exposed
        if( field->spelling() != "m_usStructSize" ) {
            fields.emplace_back( field );
        }
    }
    
    std::stringstream ss;
    ss << "class " << struct_spelling << " {" << std::endl;
    ss << "public:" << std::endl;
    ss << "    " << struct_spelling << "( A3DEntity *ntt ); " << std::endl;
    auto const base_spelling = parser::getBaseExchangeToken( struct_spelling );
    auto const raii_spelling = raii::instance().getSpelling( a3d_data_cursor );
    for( auto const &field : fields ) {
        auto const field_name_without_m_ = removeHungarianNotation(field->spelling());
        auto const getter_spelling = "Get" + field_name_without_m_;
        if( field->isArray() ) {
            auto const field_type_spelling_less_one_pointer = field->type_spelling().substr( 0, field->type_spelling().rfind("*") );
            ss << "    std::vector<" << field_type_spelling_less_one_pointer << "> " + getter_spelling + "( void ) const { return ts3d::toVector( get()->" + field->spelling() + ", get()->" + field->getArraySizeFieldMetaData()->spelling() + " ); }" << std::endl;
        } else {
            ss << "    " << field->type_spelling() << " " + getter_spelling + "( void ) const { return get()->" + field->spelling() + "; }" << std::endl;
        }
    }
    ss << "};" << std::endl;
    
    return ss.str();
}

