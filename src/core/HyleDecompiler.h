#ifndef REARK_HYLE_DECOMPILER_H
#define REARK_HYLE_DECOMPILER_H

#include "model/SourceTreeModel.h"

#include <hyle/common/async.h>
#include <hyle/hap/decompile/abc_decompiler.h>

#include <QByteArray>
#include <QString>
#include <QTemporaryDir>

#include <memory>
#include <stop_token>
#include <string>
#include <vector>

namespace HyleDecompiler {

using Session = hyle::hap::decompiled_package_session;

struct PackageSession {
    QString path;
    QString displayName;
    Session session;
};

struct SessionContext {
    hyle::async::thread_pool_executor executor;
    std::stop_source stopSource;
    std::unique_ptr<QTemporaryDir> appTempDir;
    std::vector<PackageSession> packages;

    SessionContext();
    [[nodiscard]] hyle::async::scheduler scheduler() const noexcept;
    [[nodiscard]] std::stop_token stopToken() const noexcept;
    void requestStop() noexcept;
};

struct OpenResult {
    QString error;
    QString status;
    std::vector<DecompiledSourceFile> files;
    std::shared_ptr<SessionContext> context;
    QByteArray appIconBytes;
    QString appIconPath;
    bool appIconLayered = false;
};

struct SourceResult {
    int nodeIndex = -1;
    QString name;
    QString content;
    QByteArray binaryContent;
    QString diagnostics;
    QString kind;
    QString contentMode = QStringLiteral("text");
    QString error;
};

struct SourceRequest {
    int nodeIndex = -1;
    std::size_t hyleId = 0;
    QString name;
    QString path;
    std::size_t packageId = 0;
};

struct SourceBatchResult {
    std::vector<SourceResult> files;
};

struct DisassemblyResult {
    int nodeIndex = -1;
    QString name;
    QString content;
    QString error;
};

[[nodiscard]] OpenResult openFile(
    const QString& filePath,
    const std::shared_ptr<SessionContext>& context);
[[nodiscard]] SourceResult decompileSourceFile(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name,
    std::stop_token stopToken = {},
    std::size_t packageId = 0);
[[nodiscard]] SourceBatchResult decompileSourceFiles(
    const std::shared_ptr<SessionContext>& context,
    std::vector<SourceRequest> requests);
[[nodiscard]] bool isSourceFileCached(
    const std::shared_ptr<SessionContext>& context,
    std::size_t hyleId);
[[nodiscard]] bool isSourceFileCached(
    const std::shared_ptr<SessionContext>& context,
    std::size_t hyleId,
    std::size_t packageId);
[[nodiscard]] DisassemblyResult disassembleSourceFileText(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t sourceFileId,
    const QString& name,
    std::stop_token stopToken = {},
    std::size_t packageId = 0);
[[nodiscard]] SourceResult readResourceContent(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name,
    std::stop_token stopToken = {},
    std::size_t packageId = 0);
[[nodiscard]] SourceResult readSignatureContent(
    const QString& filePath,
    int nodeIndex,
    const QString& name);
[[nodiscard]] SourceResult readSignatureContent(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    const QString& name,
    std::size_t packageId = 0);
[[nodiscard]] SourceResult readSummaryContent(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    const QString& name,
    std::stop_token stopToken = {},
    std::size_t packageId = 0);

} // namespace HyleDecompiler

#endif // REARK_HYLE_DECOMPILER_H
