#ifndef REARK_TRANSLATOR_H
#define REARK_TRANSLATOR_H

#include <QObject>
#include <QString>
#include <QTranslator>

class QQmlEngine;

class Translator : public QObject {
    Q_OBJECT

public:
    explicit Translator(QQmlEngine* engine, QObject* parent = nullptr);

    bool switchLanguage(const QString& locale);
    void resetLanguage();

signals:
    void languageChanged();

private:
    QQmlEngine* engine_ = nullptr;
    QTranslator translator_;
};

#endif // REARK_TRANSLATOR_H
