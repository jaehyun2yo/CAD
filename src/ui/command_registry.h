#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <functional>

namespace yjcad {

/// Maps DreamCAD-compatible command aliases to executable actions.
/// All lookup is case-insensitive.
class CommandRegistry {
public:
    using CommandAction = std::function<void()>;

    /// Register a command with a short alias, a full name, and an action.
    /// \param alias  Short command alias (e.g., "L").
    /// \param name   Human-readable name (e.g., "Line").
    /// \param action Callable executed when the alias is matched.
    void registerCommand(const QString& alias,
                         const QString& name,
                         CommandAction  action);

    /// Execute the command that matches \p input (trimmed, upper-cased).
    /// Returns true if a matching command was found and executed.
    bool execute(const QString& input);

    /// Return all registered aliases that start with \p partial (for autocomplete).
    QStringList suggestions(const QString& partial) const;

    /// Return the full name for an alias, or empty string if not found.
    QString nameOf(const QString& alias) const;

private:
    struct CommandEntry {
        QString       name;
        CommandAction action;
    };

    /// Key: upper-cased alias
    QMap<QString, CommandEntry> m_commands;
};

} // namespace yjcad
