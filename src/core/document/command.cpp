#include "command.h"

namespace yjcad {

void CommandHistory::execute(std::unique_ptr<Command> cmd) {
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear(); // new action invalidates redo stack
}

bool CommandHistory::undo() {
    if (m_undoStack.empty()) return false;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    return true;
}

bool CommandHistory::redo() {
    if (m_redoStack.empty()) return false;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    return true;
}

std::string CommandHistory::undoDescription() const {
    if (m_undoStack.empty()) return "";
    return m_undoStack.back()->description();
}

std::string CommandHistory::redoDescription() const {
    if (m_redoStack.empty()) return "";
    return m_redoStack.back()->description();
}

void CommandHistory::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

} // namespace yjcad
