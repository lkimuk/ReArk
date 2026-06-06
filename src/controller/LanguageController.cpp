#include "controller/LanguageController.h"

#include "core/Translator.h"

LanguageController::LanguageController(QQmlEngine* engine, QObject* parent)
    : QObject(parent)
    , translator_(new Translator(engine, this))
{
}

QString LanguageController::currentLanguage() const
{
    return currentLanguage_;
}

bool LanguageController::switchLanguage(const QString& locale)
{
    if (locale == QStringLiteral("en_US")) {
        resetLanguage();
        return true;
    }

    const bool switched = translator_->switchLanguage(locale);
    if (switched) {
        currentLanguage_ = locale;
        emit languageChanged();
    }
    return switched;
}

void LanguageController::resetLanguage()
{
    if (currentLanguage_ != QStringLiteral("en_US")) {
        currentLanguage_ = QStringLiteral("en_US");
    }
    translator_->resetLanguage();
    emit languageChanged();
}
