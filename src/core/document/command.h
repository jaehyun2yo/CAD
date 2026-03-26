#pragma once

#include <memory>
#include <string>
#include <vector>

namespace yjcad {

/// Command pattern for Undo/Redo
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
};

/// Command history — manages undo/redo stack
class CommandHistory {
public:
    /// Execute a command and add to history
    void execute(std::unique_ptr<Command> cmd);

    /// Undo last command (returns false if nothing to undo)
    bool undo();

    /// Redo last undone command (returns false if nothing to redo)
    bool redo();

    /// Check if undo/redo is available
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    /// Get description of next undo/redo action
    std::string undoDescription() const;
    std::string redoDescription() const;

    /// Clear all history
    void clear();

    size_t undoCount() const { return m_undoStack.size(); }
    size_t redoCount() const { return m_redoStack.size(); }

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
};

} // namespace yjcad
