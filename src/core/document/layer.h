#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace yjcad {

/// Layer — groups entities for visibility and organization
class Layer {
public:
    explicit Layer(const std::string& name, int color = 7);

    const std::string& name() const { return m_name; }
    int color() const { return m_color; }
    bool isVisible() const { return m_visible; }
    bool isLocked() const { return m_locked; }
    bool isFrozen() const { return m_frozen; }

    const std::string& lineType() const { return m_lineType; }
    double lineWeight() const { return m_lineWeight; }

    void setColor(int color) { m_color = color; }
    void setVisible(bool v) { m_visible = v; }
    void setLocked(bool l) { m_locked = l; }
    void setFrozen(bool f) { m_frozen = f; }
    void setLineType(const std::string& lt) { m_lineType = lt; }
    void setLineWeight(double w) { m_lineWeight = w; }

private:
    std::string m_name;
    int m_color;
    bool m_visible = true;
    bool m_locked = false;
    bool m_frozen = false;
    std::string m_lineType;
    double m_lineWeight = 0.0;
};

/// Layer manager — owns all layers
class LayerManager {
public:
    LayerManager();

    /// Add a layer (returns false if already exists)
    bool addLayer(const std::string& name, int color = 7);

    /// Remove a layer (cannot remove "0")
    bool removeLayer(const std::string& name);

    /// Get layer by name (nullptr if not found)
    Layer* layer(const std::string& name);
    const Layer* layer(const std::string& name) const;

    /// Get all layer names
    std::vector<std::string> layerNames() const;

    /// Get current active layer name
    const std::string& currentLayer() const { return m_currentLayer; }

    /// Set current active layer (must exist)
    bool setCurrentLayer(const std::string& name);

    size_t count() const { return m_layers.size(); }

private:
    std::unordered_map<std::string, Layer> m_layers;
    std::string m_currentLayer = "0";
};

} // namespace yjcad
