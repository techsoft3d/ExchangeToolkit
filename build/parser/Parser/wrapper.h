#pragma once


namespace ts3d { namespace exchange { namespace parser {
    class wrapper {
    public:
        static wrapper &instance( void );
        
        std::string codeSpelling( CXCursor c ) const ;
    private:
    };
} } }
