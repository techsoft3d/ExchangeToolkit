//
//  ExchangeHPSBridge.hpp
//  ExchangeToolkit
//
//  Created by Brad Flubacher on 4/18/19.
//  Copyright © 2019 Brad Flubacher. All rights reserved.
//

#pragma once

#include <A3DSDKIncludes.h>

namespace ts3d {
    QHash<A3DEntity*, HPS::SegmentKey> createSegmentForPartDefinition( A3DAsmPartDefinition *part );
    HPS::MaterialKit getMaterialKit( A3DGraphStyleData const &style_data );

}

