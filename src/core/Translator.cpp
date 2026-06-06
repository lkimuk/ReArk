#include "core/Translator.h"

#include <QCoreApplication>
#include <QQmlEngine>

Translator::Translator(QQmlEngine* engine, QObject* parent)
    : QObject(parent)
    , engine_(engine)
{
}

bool Translator::switchLanguage(const QString& locale)
{
    QCoreApplication::removeTranslator(&translator_);

    if (!translator_.load(QStringLiteral(":/i18n/reark_%1").arg(locale))) {
        if (engine_) {
            engine_->retranslate();
        }
        emit languageChanged();
        return false;
    }

    QCoreApplication::installTranslator(&translator_);
    if (engine_) {
        engine_->retranslate();
    }
    emit languageChanged();
    return true;
}

void Translator::resetLanguage()
{
    QCoreApplication::removeTranslator(&translator_);
    if (engine_) {
        engine_->retranslate();
    }
    emit languageChanged();
}
