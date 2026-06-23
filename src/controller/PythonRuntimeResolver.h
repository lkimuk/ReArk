#ifndef REARK_PYTHON_RUNTIME_RESOLVER_H
#define REARK_PYTHON_RUNTIME_RESOLVER_H

#include <QString>
#include <QVariantMap>

#include <filesystem>

struct PythonRuntimeProbe {
    enum class Status {
        Ok,
        NotConfigured,
        NotFound,
        NotExecutable,
        PermissionDenied,
        LaunchFailed,
        TimedOut,
        InvalidPython,
        UnsupportedVersion
    };

    Status status = Status::NotFound;
    QString configuredPath;
    QString resolvedPath;
    QString source;
    QString version;
    QString executable;
    QString detail;
};

class PythonRuntimeResolver {
public:
    [[nodiscard]] static PythonRuntimeProbe resolve(const QString& configuredPath);
    [[nodiscard]] static QVariantMap toVariantMap(const PythonRuntimeProbe& probe);
    [[nodiscard]] static QString statusLabel(const PythonRuntimeProbe& probe);
    [[nodiscard]] static QString userMessage(const PythonRuntimeProbe& probe);
    [[nodiscard]] static std::filesystem::path toFilesystemPath(const QString& value);
};

#endif // REARK_PYTHON_RUNTIME_RESOLVER_H
