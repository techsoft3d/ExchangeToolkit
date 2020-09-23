#include <sstream>
#include "MeaningfulCursors.hpp"
#include "util.h"
#include "config.h"
#include "raii.h"

using namespace ts3d::exchange::parser;

raii &raii::instance( void ) {
    static raii _instance;
    return _instance;
}

raii::raii( void ) {
    auto &meaningfulCursors = MeaningfulCursors::instance();
    for( auto const &spelling : meaningfulCursors._stringToCursorMap ) {
        
        if( ts3d::exchange::parser::Category::Data != ts3d::exchange::parser::getCategory( spelling.first ) ) {
            continue;
        }
        
        // fundamentally, all constructors will call A3D_INITIALIZE_DATA
        // so the struct in question must have the field m_usStructSize.
        // if it does not, we must skip it.
        // currently only A3DCopyAndAdaptBrepModelErrorData meets this condition
        if( !ts3d::hasField( spelling.second, "m_usStructSize" ) ) {
            continue;
        }
        
        if( parser::Config::instance().shouldSkipWrapper( spelling.first ) ) {
            continue;
        }
        Wrapper e;
        auto const get_spelling = ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Get );
        auto const has_get = meaningfulCursors.hasCursor( get_spelling );
        auto const create_spelling = ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Create );
        auto const has_create = meaningfulCursors.hasCursor( create_spelling);
        auto const edit_spelling = ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Edit );
        auto const has_edit = meaningfulCursors.hasCursor( edit_spelling );
        e.data_type_spelling = spelling.first;
        e.void_type_spelling = std::string( std::begin( spelling.first ), std::end( spelling.first) - std::string("Data").length() );
        e.initialize_function_spelling = "inline void InitializeData( " + e.data_type_spelling + " &d ) { A3D_INITIALIZE_DATA( " + e.data_type_spelling + ", d ) }";
        e.wrapper_spelling = e.void_type_spelling + "Wrapper";
        if( !has_get && !has_create && !has_edit ) {
            e.alias_declaration_spelling = "using " + e.wrapper_spelling + " = Wrapper<" + e.data_type_spelling + ">;";
        } else if( has_get && !has_create && !has_edit ) {
            e.alias_declaration_spelling = "using " + e.wrapper_spelling + " = GettableWrapper<" + e.data_type_spelling + ", " + get_spelling + ">;";
        } else if( has_create && !has_get && !has_edit ) {
            e.alias_declaration_spelling = "using " + e.wrapper_spelling + " = CreateableWrapper<" + e.data_type_spelling + ", " + create_spelling + ">;";
        } else if( has_create && has_get && !has_edit ) {
            e.alias_declaration_spelling = "using " + e.wrapper_spelling + " = CreateableGettableWrapper<" + e.data_type_spelling + ", " + get_spelling + ", " + create_spelling + ">;";
        } else if( has_create && has_edit && has_get ) {
            e.alias_declaration_spelling = "using " + e.wrapper_spelling + " = CreateableGettableEditableWrapper<" + e.data_type_spelling + ", " + get_spelling + ", " + create_spelling + ", " + edit_spelling + ">;";
        } else {
            assert( false );
        }
        _wrappers.insert( std::lower_bound(std::begin( _wrappers ), std::end( _wrappers ), e ), std::move( e ) );
    }
}

namespace {
    std::string initFunctionsSpelling( std::vector<raii::Wrapper> const &entries ) {
        std::stringstream ss;
        ss << "namespace ts3d { namespace raii {" << std::endl;
        for( auto const &Wrapper : entries ) {
            ss << "    " << Wrapper.initialize_function_spelling << std::endl;
        }
        ss << "} } // namespace ts3d::raii" << std::endl;
        return ss.str();
    }

    std::string usingDeclsSpelling( std::vector<raii::Wrapper> const &entries )  {
        std::stringstream ss;
        ss << "namespace ts3d { namespace raii {" << std::endl;
        for( auto const &Wrapper : entries ) {
            ss << "    " << Wrapper.alias_declaration_spelling << std::endl;
        }
        ss << "} } // namespace ts3d::raii" << std::endl;
        return ss.str();
    }


    std::string templateSpellings( void ) {
        std::stringstream ss;
        ss << "namespace ts3d { namespace raii {" << std::endl;
        ss << "    template <typename DataType>" << std::endl;
        ss << "    struct Wrapper {" << std::endl;
        ss << "        Wrapper( void ) {" << std::endl;
        ss << "            InitializeData(_d);" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        Wrapper( Wrapper && other ) {" << std::endl;
        ss << "            _d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        Wrapper( Wrapper const &other ) = delete;" << std::endl;
        ss << std::endl;
        ss << "        Wrapper &operator=( Wrapper const &other ) = delete;" << std::endl;
        ss << std::endl;
        ss << "        Wrapper &operator=( Wrapper &&other ) {" << std::endl;
        ss << "            _d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "            return *this;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        ~Wrapper( void ) = default;" << std::endl;
        ss << std::endl;
        ss << "        DataType const *operator->( void ) const {" << std::endl;
        ss << "            return &_d;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        DataType *operator->( void ) {" << std::endl;
        ss << "            return &_d;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        DataType _d;" << std::endl;
        ss << "    };" << std::endl;
        ss << std::endl;
        ss << "    template <typename T> using GetterFcnRef = A3DStatus(*&)(void const*, T*);" << std::endl;
        ss << "    template <typename DataType, GetterFcnRef<DataType> Getter>" << std::endl;
        ss << "    struct GettableWrapper : Wrapper<DataType> {" << std::endl;
        ss << "        GettableWrapper( void *p = nullptr ) : Wrapper<DataType>() {" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        GettableWrapper( GettableWrapper && other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        ~GettableWrapper( void ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        GettableWrapper &operator=( GettableWrapper &&other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "            return *this;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        void reset( void *p = nullptr ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << "    };" << std::endl;
        ss << std::endl;
        ss << "    template <typename T> using CreatorFcnRef = A3DStatus(*&)(T const *, void **);" << std::endl;
        ss << "    template<typename DataType, CreatorFcnRef<DataType> Creator>" << std::endl;
        ss << "    struct CreateableWrapper : Wrapper<DataType> {" << std::endl;
        ss << "        void *create( void ) const {" << std::endl;
        ss << "            void *p = nullptr;" << std::endl;
        ss << "            Creator( &this->_d, &p );" << std::endl;
        ss << "            return p;" << std::endl;
        ss << "        }" << std::endl;
        ss << "    };" << std::endl;
        ss << std::endl;
        ss << "    template <typename DataType, GetterFcnRef<DataType> Getter, CreatorFcnRef<DataType> Creator>" << std::endl;
        ss << "    struct CreateableGettableWrapper : Wrapper<DataType> {" << std::endl;
        ss << "        CreateableGettableWrapper( void *p = nullptr ) : Wrapper<DataType>() {" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        CreateableGettableWrapper( CreateableGettableWrapper && other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        ~CreateableGettableWrapper( void ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        CreateableGettableWrapper &operator=( CreateableGettableWrapper &&other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "            return *this;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        void reset( void *p = nullptr ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        void *create( void ) const {" << std::endl;
        ss << "            void *p = nullptr;" << std::endl;
        ss << "            Creator( &this->_d, &p );" << std::endl;
        ss << "            return p;" << std::endl;
        ss << "        }" << std::endl;
        ss << "    };" << std::endl;
        ss << std::endl;
        ss << "    template <typename T> using EditorFcnRef = A3DStatus(*&)(T const *, void*);" << std::endl;
        ss << "    template <typename DataType, GetterFcnRef<DataType> Getter, CreatorFcnRef<DataType> Creator, EditorFcnRef<DataType> Editor>" << std::endl;
        ss << "    struct CreateableGettableEditableWrapper : Wrapper<DataType> {" << std::endl;
        ss << "        CreateableGettableEditableWrapper( void *p = nullptr ) : Wrapper<DataType>() {" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        CreateableGettableEditableWrapper( CreateableGettableEditableWrapper && other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        ~CreateableGettableEditableWrapper( void ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        CreateableGettableEditableWrapper &operator=( CreateableGettableEditableWrapper &&other ) {" << std::endl;
        ss << "            this->_d = other._d;" << std::endl;
        ss << "            InitializeData( other._d );" << std::endl;
        ss << "            return *this;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        void reset( void *p = nullptr ) {" << std::endl;
        ss << "            Getter( nullptr, &this->_d );" << std::endl;
        ss << "            if( p ) {" << std::endl;
        ss << "                Getter( p, &this->_d );" << std::endl;
        ss << "            }" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        void *create( void ) const {" << std::endl;
        ss << "            void *p = nullptr;" << std::endl;
        ss << "            Creator( &this->_d, &p );" << std::endl;
        ss << "            return p;" << std::endl;
        ss << "        }" << std::endl;
        ss << std::endl;
        ss << "        bool edit( void *p ) {" << std::endl;
        ss << "            return A3D_SUCCESS == Editor( &this->_d, p );" << std::endl;
        ss << "        }" << std::endl;
        ss << "    };" << std::endl;
        ss << "} } // namespace ts3d::raii" << std::endl;
        return ss.str();
    }
}
        
std::string raii::codeSpelling( void ) const {
    std::stringstream ss;
    ss << initFunctionsSpelling( _wrappers ) << std::endl;
    ss << templateSpellings() << std::endl;
    ss << usingDeclsSpelling( _wrappers ) << std::endl;
    return ss.str();
}
