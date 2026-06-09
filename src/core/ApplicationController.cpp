#include "core/ApplicationController.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

ApplicationController::ApplicationController(QObject* parent)
    : QObject(parent)
{
}

bool ApplicationController::openNewWindow()
{
    const QString executablePath = QCoreApplication::applicationFilePath();
    const QString workingDirectory = QFileInfo(executablePath).absolutePath();
    return QProcess::startDetached(executablePath, {}, workingDirectory);
}
