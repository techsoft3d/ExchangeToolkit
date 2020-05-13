#include "MeaningfulCursors.hpp"
#include "util.h"
#include "config.h"
#include "raii.h"

using namespace ts3d::exchange::parser;
std::vector<raii::Entry> raii::get( CXCursor c ) {
    auto &meaningfulCursors =MeaningfulCursors::instance();
    std::vector<raii::Entry> result;
    for( auto const &spelling : meaningfulCursors._stringToCursorMap ) {

        if( ts3d::exchange::parser::Category::Data != ts3d::exchange::parser::getCategory( spelling.first ) ) {
            continue;
        }

        if( std::end( ts3d::exchange::parser::wrappersToSkip ) == ts3d::exchange::parser::wrappersToSkip.find( spelling.first ) ) {
            Entry e;
            e.has_get = meaningfulCursors.hasCursor( ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Get ) );
            e.has_create = meaningfulCursors.hasCursor( ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Create ) );
            e.has_edit = meaningfulCursors.hasCursor( ts3d::exchange::parser::getExchangeToken( spelling.first, ts3d::exchange::parser::Category::Edit ) );
            e.void_type_spelling  = "A3D" + ts3d::exchange::parser::getBaseExchangeToken( spelling.first );
            result.emplace_back( e );
        }
    }
    return result;
}

std::ostream &raii::Entry::operator<<(std::ostream &os ) const {
    if( has_get && has_create && has_edit ) {
        os << "A3D_BETA_CREATABLE_EDITABLE_HELPER( " << void_type_spelling << " )" << std::endl;
    } else if( has_get && has_create ) {
        os << "A3D_BETA_CREATABLE_HELPER( " << void_type_spelling << " )" << std::endl;
    } else if( has_get ) {
        os << "A3D_BETA_READONLY_HELPER( " << void_type_spelling << " )" << std::endl;
    }
    return os;
}

