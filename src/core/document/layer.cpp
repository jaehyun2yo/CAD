#include "layer.h"

namespace yjcad {

Layer::Layer(const std::string& name, int color)
    : m_name(name), m_color(color) {}

LayerManager::LayerManager() {
    // Default layer "0" always exists (DXF convention)
    m_layers.emplace("0", Layer("0", 7));
}

bool LayerManager::addLayer(const std::string& name, int color) {
    if (m_layers.count(name)) return false;
    m_layers.emplace(name, Layer(name, color));
    return true;
}

bool LayerManager::removeLayer(const std::string& name) {
    if (name == "0") return false; // cannot remove default layer
    if (name == m_currentLayer) return false; // cannot remove active layer
    return m_layers.erase(name) > 0;
}

Layer* LayerManager::layer(const std::string& name) {
    auto it = m_layers.find(name);
    return it != m_layers.end() ? &it->second : nullptr;
}

const Layer* LayerManager::layer(const std::string& name) const {
    auto it = m_layers.find(name);
    return it != m_layers.end() ? &it->second : nullptr;
}

std::vector<std::string> LayerManager::layerNames() const {
    std::vector<std::string> names;
    names.reserve(m_layers.size());
    for (const auto& [name, _] : m_layers) {
        names.push_back(name);
    }
    return names;
}

bool LayerManager::setCurrentLayer(const std::string& name) {
    if (!m_layers.count(name)) return false;
    m_currentLayer = name;
    return true;
}

} // namespace yjcad
