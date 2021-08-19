#pragma once

#include "util.h"

namespace ts3d {
    class MeaningfulCursors {
    public:
        enum class Category {
            Data = 0,
            Get = 1,
            Edit = 2,
            Create = 3,
            Type = 4,
            MAX_VALUE = 5
        };
           
        static MeaningfulCursors &instance( void );
        
        void populate( CXCursor cursor );
        
        
        ts3d::StringSet getAllCursorSpellings( ts3d::exchange::parser::Category const &category ) const;

        CXCursor getCursor( std::string const &cursor_spelling );
        
        
        StringToCursor _stringToCursorMap;
        StringSet _missingTypeEnums;
        
        bool hasCursor( std::string const &spelling );

    private:
            MeaningfulCursors( void );
    };
    
}
