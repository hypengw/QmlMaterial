#pragma once

namespace qml_material::sg::mesh
{

/// Write 6 vertices forming two triangles (lt-rt-lb, lb-rb-rt) for one quad cell.
template<class V>
inline void quad(V* out, const V& lt, const V& rt, const V& lb, const V& rb) {
    out[0] = lt;
    out[1] = rt;
    out[2] = lb;
    out[3] = lb;
    out[4] = rb;
    out[5] = rt;
}

} // namespace qml_material::sg::mesh
