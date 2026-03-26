#pragma once

#include "core/document/document.h"
#include "core/geometry/operations.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

namespace yjcad {

/// Python plugin API — exposes Document, Entity, Geometry to Python
/// Full open API (엔지니어링 리뷰 결정: 3C)
class PluginAPI {
public:
    /// Register all YJ-CAD types with pybind11
    static void registerModule(pybind11::module_& m);
};

} // namespace yjcad
