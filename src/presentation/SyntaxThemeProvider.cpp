#include "presentation/SyntaxThemeProvider.h"

SyntaxThemeProvider::SyntaxThemeProvider(QObject* parent)
    : QObject(parent)
    , themes_ {
          QStringLiteral("GitHub Dark"),
          QStringLiteral("GitHub Light"),
          QStringLiteral("Monokai"),
          QStringLiteral("Dracula")
      }
{
}

QStringList SyntaxThemeProvider::themes() const
{
    return themes_;
}
