#include "ui/command_registry.h"

namespace yjcad {

void CommandRegistry::registerCommand(const QString& alias,
                                      const QString& name,
                                      CommandAction  action)
{
    const QString key = alias.toUpper().trimmed();
    m_commands.insert(key, CommandEntry{name, std::move(action)});
}

bool CommandRegistry::execute(const QString& input)
{
    const QString key = input.toUpper().trimmed();
    auto it = m_commands.find(key);
    if (it == m_commands.end())
        return false;

    if (it->action)
        it->action();
    return true;
}

QStringList CommandRegistry::suggestions(const QString& partial) const
{
    const QString prefix = partial.toUpper().trimmed();
    QStringList result;
    for (auto it = m_commands.cbegin(); it != m_commands.cend(); ++it) {
        if (it.key().startsWith(prefix))
            result << it.key();
    }
    return result;
}

QString CommandRegistry::nameOf(const QString& alias) const
{
    auto it = m_commands.find(alias.toUpper().trimmed());
    if (it == m_commands.end())
        return {};
    return it->name;
}

} // namespace yjcad
