#include "plugin_api.h"

namespace py = pybind11;

namespace yjcad {

void PluginAPI::registerModule(py::module_& m) {
    // ── Point2D ──
    py::class_<Point2D>(m, "Point2D")
        .def(py::init<>())
        .def(py::init<double, double>())
        .def_readwrite("x", &Point2D::x)
        .def_readwrite("y", &Point2D::y)
        .def("distance_to", &Point2D::distanceTo)
        .def("__repr__", [](const Point2D& p) {
            return "Point2D(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
        });

    // ── BoundingBox ──
    py::class_<BoundingBox>(m, "BoundingBox")
        .def_readwrite("min", &BoundingBox::min)
        .def_readwrite("max", &BoundingBox::max)
        .def("contains", &BoundingBox::contains)
        .def("intersects", &BoundingBox::intersects);

    // ── LineClassification enum ──
    py::enum_<LineClassification>(m, "LineClassification")
        .value("Unknown", LineClassification::Unknown)
        .value("Oshi", LineClassification::Oshi)
        .value("Knife", LineClassification::Knife)
        .value("Guide", LineClassification::Guide)
        .value("Bridge", LineClassification::Bridge)
        .value("Outline", LineClassification::Outline);

    // ── EntityType enum ──
    py::enum_<EntityType>(m, "EntityType")
        .value("Line", EntityType::Line)
        .value("Circle", EntityType::Circle)
        .value("Arc", EntityType::Arc)
        .value("Polyline", EntityType::Polyline)
        .value("Text", EntityType::Text);

    // ── Entity base ──
    py::class_<Entity>(m, "Entity")
        .def("id", &Entity::id)
        .def("type", &Entity::type)
        .def_property("layer_name", &Entity::layerName, &Entity::setLayerName)
        .def_property("color", &Entity::color, &Entity::setColor)
        .def_property("classification", &Entity::classification, &Entity::setClassification)
        .def_property("visible", &Entity::isVisible, &Entity::setVisible)
        .def_property("selected", &Entity::isSelected, &Entity::setSelected)
        .def("bounding_box", &Entity::boundingBox)
        .def("translate", &Entity::translate)
        .def("rotate", &Entity::rotate)
        .def("scale", &Entity::scale);

    // ── LineEntity ──
    py::class_<LineEntity, Entity>(m, "LineEntity")
        .def(py::init<EntityId, const Point2D&, const Point2D&>())
        .def_property("start", &LineEntity::start, &LineEntity::setStart)
        .def_property("end", &LineEntity::end, &LineEntity::setEnd)
        .def("length", &LineEntity::length);

    // ── CircleEntity ──
    py::class_<CircleEntity, Entity>(m, "CircleEntity")
        .def(py::init<EntityId, const Point2D&, double>())
        .def_property("center", &CircleEntity::center, &CircleEntity::setCenter)
        .def_property("radius", &CircleEntity::radius, &CircleEntity::setRadius);

    // ── ArcEntity ──
    py::class_<ArcEntity, Entity>(m, "ArcEntity")
        .def(py::init<EntityId, const Point2D&, double, double, double>())
        .def("center", &ArcEntity::center)
        .def("radius", &ArcEntity::radius)
        .def("start_angle", &ArcEntity::startAngle)
        .def("end_angle", &ArcEntity::endAngle);

    // ── PolylineEntity ──
    py::class_<PolylineEntity, Entity>(m, "PolylineEntity")
        .def(py::init<EntityId, const std::vector<Point2D>&, bool>(),
             py::arg("id"), py::arg("vertices"), py::arg("closed") = false)
        .def("vertices", &PolylineEntity::vertices)
        .def_property("closed", &PolylineEntity::isClosed, &PolylineEntity::setClosed);

    // ── TextEntity ──
    py::class_<TextEntity, Entity>(m, "TextEntity")
        .def(py::init<EntityId, const Point2D&, const std::string&, double>())
        .def_property("position", &TextEntity::position, &TextEntity::setPosition)
        .def_property("text", &TextEntity::text, &TextEntity::setText)
        .def_property("height", &TextEntity::height, &TextEntity::setHeight);

    // ── Layer ──
    py::class_<Layer>(m, "Layer")
        .def("name", &Layer::name)
        .def_property("color", &Layer::color, &Layer::setColor)
        .def_property("visible", &Layer::isVisible, &Layer::setVisible)
        .def_property("locked", &Layer::isLocked, &Layer::setLocked);

    // ── LayerManager ──
    py::class_<LayerManager>(m, "LayerManager")
        .def("add_layer", &LayerManager::addLayer,
             py::arg("name"), py::arg("color") = 7)
        .def("remove_layer", &LayerManager::removeLayer)
        .def("layer", py::overload_cast<const std::string&>(&LayerManager::layer),
             py::return_value_policy::reference)
        .def("layer_names", &LayerManager::layerNames)
        .def_property("current_layer", &LayerManager::currentLayer, &LayerManager::setCurrentLayer)
        .def("count", &LayerManager::count);

    // ── Selection ──
    py::class_<Selection>(m, "Selection")
        .def("select", &Selection::select)
        .def("deselect", &Selection::deselect)
        .def("toggle", &Selection::toggle)
        .def("clear", &Selection::clear)
        .def("is_selected", &Selection::isSelected)
        .def("count", &Selection::count)
        .def("selected_ids", [](const Selection& s) {
            return std::vector<EntityId>(s.selectedIds().begin(), s.selectedIds().end());
        });

    // ── Document ──
    py::class_<Document>(m, "Document")
        .def(py::init<>())
        .def("next_id", &Document::nextId)
        .def("entity_count", &Document::entityCount)
        .def("entity_ids", &Document::entityIds)
        .def("entity", py::overload_cast<EntityId>(&Document::entity),
             py::return_value_policy::reference)
        .def("bounds", &Document::bounds)
        .def("entities_in_box", &Document::entitiesInBox)
        .def_property_readonly("layers", py::overload_cast<>(&Document::layers),
                               py::return_value_policy::reference)
        .def_property_readonly("selection", py::overload_cast<>(&Document::selection),
                               py::return_value_policy::reference)
        .def_property("modified", &Document::isModified, &Document::setModified)
        .def_property("file_path",
                      [](const Document& d) { return d.filePath(); },
                      &Document::setFilePath);

    // ── Geometry Operations ──
    py::class_<GeometryOps>(m, "GeometryOps")
        .def_static("insert_bridges", &GeometryOps::insertBridges)
        .def_static("arc_to_polyline", &GeometryOps::arcToPolyline)
        .def_static("circle_to_polyline", &GeometryOps::circleToPolyline)
        .def_static("point_to_segment_distance", &GeometryOps::pointToSegmentDistance);
}

// ── Module definition ──
PYBIND11_MODULE(yjcad_core, m) {
    m.doc() = "YJ-CAD Core Python bindings — 도무송 목형 전용 CAD";
    PluginAPI::registerModule(m);
}

} // namespace yjcad
