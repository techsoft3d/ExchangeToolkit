#pragma once

#include <set>
#include <A3DSDKIncludes.h>

namespace ts3d {
    bool attachEdgeAttributes( A3DTopoEdge *edge, std::set<A3DTopoFace*> const &owning_faces, A3DVector3dData const &scale );
    bool attachFaceAttributes( A3DTopoFace *face, A3DVector3dData const &scale );
}
