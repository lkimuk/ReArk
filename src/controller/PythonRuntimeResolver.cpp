#include "controller/PythonRuntimeResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {

constexpr int kPythonProbeTimeoutMs = 3000;

struct PythonCandidate {
    QString path;
    QString source;
    bool requireExistingFile = true;
};

QString envString(const char* name)
{
    return QString::fromUtf8(qgetenv(name)).trimmed();
}

QString normalizedPath(QString path)
{
    path = path.trimmed();
    if (path.isEmpty()) {
        return {};
    }

    const QFileInfo info(path);
    return info.exists() ? info.absoluteFilePath() : path;
}

QStringList bundledPythonCandidates()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    return {
        appDir.absoluteFilePath(QStringLiteral("python/python.exe")),
        appDir.absoluteFilePath(QStringLiteral("runtime/python/python.exe")),
        appDir.absoluteFilePath(QStringLiteral("python.exe"))
    };
}

QString findSystemPython()
{
    const QStringList names {
#ifdef Q_OS_WIN
        QStringLiteral("python.exe"),
        QStringLiteral("python3.exe"),
        QStringLiteral("py.exe")
#else
        QStringLiteral("python3"),
        QStringLiteral("python")
#endif
    };

    for (const QString& name : names) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty()) {
            return path;
        }
    }
    return {};
}

QVector<PythonCandidate> candidatesFor(const QString& configuredPath)
{
    QVector<PythonCandidate> candidates;
    const QString configured = configuredPath.trimmed();
    if (!configured.isEmpty()) {
        candidates.push_back({
            .path = configured,
            .source = QStringLiteral("configured"),
            .requireExistingFile = true
        });
    }

    for (const QString& path : bundledPythonCandidates()) {
        candidates.push_back({
            .path = path,
            .source = QStringLiteral("bundled"),
            .requireExistingFile = true
        });
    }

    const QString envPath = envString("REARK_PYTHON_PATH");
    if (!envPath.isEmpty()) {
        candidates.push_back({
            .path = envPath,
            .source = QStringLiteral("environment"),
            .requireExistingFile = true
        });
    }

    const QString systemPython = findSystemPython();
    if (!systemPython.isEmpty()) {
        candidates.push_back({
            .path = systemPython,
            .source = QStringLiteral("system"),
            .requireExistingFile = true
        });
    }
    return candidates;
}

PythonRuntimeProbe probeCandidate(const PythonCandidate& candidate)
{
    PythonRuntimeProbe result;
    result.configuredPath = candidate.path;
    result.resolvedPath = normalizedPath(candidate.path);
    result.source = candidate.source;

    if (result.resolvedPath.trimmed().isEmpty()) {
        result.status = PythonRuntimeProbe::Status::NotConfigured;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }

    const QFileInfo info(result.resolvedPath);
    if (candidate.requireExistingFile && (!info.exists() || !info.isFile())) {
        result.status = PythonRuntimeProbe::Status::NotFound;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }
    if (candidate.requireExistingFile && !info.isExecutable()) {
        result.status = PythonRuntimeProbe::Status::LaunchFailed;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }

    static constexpr auto kProbeCode =
        "import json, sys\n"
        "print(json.dumps({"
        "'version': sys.version.split()[0], "
        "'executable': sys.executable"
        "}, ensure_ascii=False))\n";

    QProcess process;
    process.setProgram(result.resolvedPath);
    process.setArguments({ QStringLiteral("-c"), QString::fromLatin1(kProbeCode) });
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setProcessEnvironment(QProcessEnvironment());
    process.start();

    if (!process.waitForStarted(kPythonProbeTimeoutMs)) {
        result.status = PythonRuntimeProbe::Status::LaunchFailed;
        result.detail = process.errorString();
        if (result.detail.isEmpty()) {
            result.detail = PythonRuntimeResolver::userMessage(result);
        }
        return result;
    }

    if (!process.waitForFinished(kPythonProbeTimeoutMs)) {
        process.kill();
        process.waitForFinished(500);
        result.status = PythonRuntimeProbe::Status::TimedOut;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }

    const QByteArray stdoutBytes = process.readAllStandardOutput();
    const QByteArray stderrBytes = process.readAllStandardError();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.status = PythonRuntimeProbe::Status::InvalidPython;
        result.detail = QString::fromUtf8(stderrBytes).trimmed();
        if (result.detail.isEmpty()) {
            result.detail = PythonRuntimeResolver::userMessage(result);
        }
        return result;
    }

    const QJsonDocument document = QJsonDocument::fromJson(stdoutBytes);
    if (!document.isObject()) {
        result.status = PythonRuntimeProbe::Status::InvalidPython;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }

    const QJsonObject object = document.object();
    result.version = object.value(QStringLiteral("version")).toString();
    result.executable = object.value(QStringLiteral("executable")).toString();
    if (result.version.isEmpty() || result.executable.isEmpty()) {
        result.status = PythonRuntimeProbe::Status::InvalidPython;
        result.detail = PythonRuntimeResolver::userMessage(result);
        return result;
    }

    result.status = PythonRuntimeProbe::Status::Ok;
    result.resolvedPath = normalizedPath(result.executable);
    result.detail = PythonRuntimeResolver::userMessage(result);
    return result;
}

} // namespace

PythonRuntimeProbe PythonRuntimeResolver::resolve(const QString& configuredPath)
{
    PythonRuntimeProbe lastFailure;
    lastFailure.configuredPath = configuredPath.trimmed();

    const QVector<PythonCandidate> candidates = candidatesFor(configuredPath);
    for (const PythonCandidate& candidate : candidates) {
        PythonRuntimeProbe probe = probeCandidate(candidate);
        if (candidate.source == QStringLiteral("configured")) {
            return probe;
        }
        if (probe.status == PythonRuntimeProbe::Status::Ok) {
            return probe;
        }
        if (lastFailure.status == PythonRuntimeProbe::Status::NotFound) {
            lastFailure = probe;
        }
    }

    if (lastFailure.configuredPath.isEmpty()) {
        lastFailure.status = PythonRuntimeProbe::Status::NotFound;
        lastFailure.detail = userMessage(lastFailure);
    }
    return lastFailure;
}

QVariantMap PythonRuntimeResolver::toVariantMap(const PythonRuntimeProbe& probe)
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), probe.status == PythonRuntimeProbe::Status::Ok);
    result.insert(QStringLiteral("status"), statusLabel(probe));
    result.insert(QStringLiteral("configuredPath"), probe.configuredPath);
    result.insert(QStringLiteral("resolvedPath"), probe.resolvedPath);
    result.insert(QStringLiteral("source"), probe.source);
    result.insert(QStringLiteral("version"), probe.version);
    result.insert(QStringLiteral("executable"), probe.executable);
    result.insert(QStringLiteral("detail"), probe.detail.isEmpty() ? userMessage(probe) : probe.detail);
    return result;
}

QString PythonRuntimeResolver::statusLabel(const PythonRuntimeProbe& probe)
{
    switch (probe.status) {
    case PythonRuntimeProbe::Status::Ok:
        return QCoreApplication::translate("PythonRuntimeResolver", "Ready");
    case PythonRuntimeProbe::Status::NotConfigured:
        return QCoreApplication::translate("PythonRuntimeResolver", "Not configured");
    case PythonRuntimeProbe::Status::NotFound:
        return QCoreApplication::translate("PythonRuntimeResolver", "Not found");
    case PythonRuntimeProbe::Status::LaunchFailed:
        return QCoreApplication::translate("PythonRuntimeResolver", "Launch failed");
    case PythonRuntimeProbe::Status::TimedOut:
        return QCoreApplication::translate("PythonRuntimeResolver", "Startup timed out");
    case PythonRuntimeProbe::Status::InvalidPython:
        return QCoreApplication::translate("PythonRuntimeResolver", "Invalid Python");
    }
    return QCoreApplication::translate("PythonRuntimeResolver", "Unavailable");
}

QString PythonRuntimeResolver::userMessage(const PythonRuntimeProbe& probe)
{
    switch (probe.status) {
    case PythonRuntimeProbe::Status::Ok:
        return QCoreApplication::translate("PythonRuntimeResolver", "Python %1 is available.").arg(probe.version);
    case PythonRuntimeProbe::Status::NotConfigured:
        return QCoreApplication::translate("PythonRuntimeResolver", "Set a Python interpreter path or install the bundled runtime.");
    case PythonRuntimeProbe::Status::NotFound:
        return probe.configuredPath.isEmpty()
            ? QCoreApplication::translate("PythonRuntimeResolver", "No Python interpreter was found. Local analysis scripts are disabled.")
            : QCoreApplication::translate("PythonRuntimeResolver", "Python interpreter was not found at the configured path.");
    case PythonRuntimeProbe::Status::LaunchFailed:
        return QCoreApplication::translate("PythonRuntimeResolver", "Python interpreter could not be launched.");
    case PythonRuntimeProbe::Status::TimedOut:
        return QCoreApplication::translate("PythonRuntimeResolver", "Python interpreter did not start in time.");
    case PythonRuntimeProbe::Status::InvalidPython:
        return QCoreApplication::translate("PythonRuntimeResolver", "The selected executable did not behave like Python.");
    }
    return QCoreApplication::translate("PythonRuntimeResolver", "Python runtime is unavailable.");
}

std::filesystem::path PythonRuntimeResolver::toFilesystemPath(const QString& value)
{
    return std::filesystem::path(value.toStdWString());
}
