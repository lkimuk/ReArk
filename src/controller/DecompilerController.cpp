#include "controller/DecompilerController.h"

#include "core/ResourcePreviewProvider.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMimeDatabase>
#include <QProcess>
#include <QUrl>
#include <QVariantMap>
#include <QtConcurrent>

#include <algorithm>
#include <memory>

namespace {

constexpr int kMaxBackgroundPreloads = 2;
constexpr int kMaxBackgroundSourceBatchSize = 32;
constexpr int kMaxQueuedBackgroundPreloads = 512;
constexpr qsizetype kMaxBackgroundCachedBytes = 2 * 1024 * 1024;

qsizetype cachedResultSize(const HyleDecompiler::SourceResult& result)
{
    return result.content.size() * static_cast<qsizetype>(sizeof(QChar))
        + result.binaryContent.size()
        + result.diagnostics.size() * static_cast<qsizetype>(sizeof(QChar));
}

QString makeAppIconDataUrl(const QByteArray& bytes, const QString& iconPath, bool layered)
{
    if (bytes.isEmpty()) {
        return {};
    }

    QString mimeName = QStringLiteral("image/png");
    if (!layered) {
        QMimeDatabase mimeDatabase;
        auto mime = mimeDatabase.mimeTypeForFileNameAndData(iconPath, bytes);
        if (!mime.isValid() || !mime.name().startsWith(QLatin1String("image/"))) {
            mime = mimeDatabase.mimeTypeForData(bytes);
        }
        if (!mime.isValid() || !mime.name().startsWith(QLatin1String("image/"))) {
            return {};
        }
        mimeName = mime.name();
    }

    return QStringLiteral("data:%1;base64,%2")
        .arg(mimeName, QString::fromLatin1(bytes.toBase64()));
}

QString boundedAgentText(QString text, int maxChars)
{
    constexpr int kDefaultMaxChars = 12000;
    constexpr int kHardMaxChars = 60000;
    if (maxChars <= 0) {
        maxChars = kDefaultMaxChars;
    }
    maxChars = std::clamp(maxChars, 1000, kHardMaxChars);
    if (text.size() <= maxChars) {
        return text;
    }
    return text.left(maxChars)
        + QStringLiteral("\n\n[truncated: %1 of %2 characters shown]")
              .arg(maxChars)
              .arg(text.size());
}

int candidateNodeIndex(const QVariantMap& candidate)
{
    bool ok = false;
    const int nodeIndex = candidate.value(QStringLiteral("nodeIndex")).toInt(&ok);
    return ok ? nodeIndex : -1;
}

QString formatCandidateLine(const QVariantMap& candidate)
{
    const QString path = candidate.value(QStringLiteral("path")).toString();
    const QString kind = candidate.value(QStringLiteral("kind")).toString();
    const QString section = candidate.value(QStringLiteral("section")).toString();
    const QString mode = candidate.value(QStringLiteral("contentMode")).toString();
    const QString subtitle = candidate.value(QStringLiteral("subtitle")).toString();
    QString line = path;
    if (!kind.isEmpty()) {
        line += QStringLiteral(" | kind=%1").arg(kind);
    }
    if (!section.isEmpty()) {
        line += QStringLiteral(" | section=%1").arg(section);
    }
    if (!mode.isEmpty()) {
        line += QStringLiteral(" | mode=%1").arg(mode);
    }
    if (!subtitle.isEmpty() && subtitle != path) {
        line += QStringLiteral(" | %1").arg(subtitle);
    }
    return line;
}

} // namespace

DecompilerController::DecompilerController(ResourcePreviewProvider* previewProvider, QObject* parent)
    : QObject(parent)
    , treeModel_(this)
    , tabsModel_(this)
    , hexModel_(this)
    , previewProvider_(previewProvider)
{
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::selectedContentChanged);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::selectedNameChanged);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::diagnosticsChanged);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::refreshActiveHexDocument);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::activeDisassemblyChanged);
    connect(&treeModel_, &SourceTreeModel::selectedIndexChanged,
            this, &DecompilerController::selectedIndexChanged);
    connect(&treeModel_, &SourceTreeModel::fileActivated,
            this, &DecompilerController::openFileTab);
}

SourceTreeModel* DecompilerController::treeModel()
{
    return &treeModel_;
}

OpenFileTabsModel* DecompilerController::tabsModel()
{
    return &tabsModel_;
}

HexDocumentModel* DecompilerController::hexModel()
{
    return &hexModel_;
}

QString DecompilerController::selectedContent() const
{
    return tabsModel_.activeContent();
}

QString DecompilerController::selectedName() const
{
    return tabsModel_.activePath();
}

QString DecompilerController::diagnostics() const
{
    return tabsModel_.activeDiagnostics();
}

QString DecompilerController::status() const
{
    return status_;
}

bool DecompilerController::busy() const
{
    return busy_;
}

double DecompilerController::loadingProgress() const
{
    return loadingProgress_;
}

QStringList DecompilerController::activityLog() const
{
    return activityLog_;
}

bool DecompilerController::hasPackage() const
{
    return hasPackage_;
}

QString DecompilerController::packagePath() const
{
    return hasPackage_ ? packagePath_ : QString();
}

QString DecompilerController::appIconUrl() const
{
    return hasPackage_ ? appIconUrl_ : QString();
}

QString DecompilerController::appIconDataUrl() const
{
    return hasPackage_ ? appIconDataUrl_ : QString();
}

QString DecompilerController::appIconPath() const
{
    return hasPackage_ ? appIconPath_ : QString();
}

bool DecompilerController::appIconLayered() const
{
    return hasPackage_ && appIconLayered_;
}

bool DecompilerController::activeSupportsDisassembly() const
{
    return treeModel_.nodeHasDisassembly(tabsModel_.activeNodeIndex());
}

bool DecompilerController::activeDisassemblyLoading() const
{
    return disassemblyLoadingNodes_.contains(tabsModel_.activeNodeIndex());
}

QString DecompilerController::activeDisassemblyContent() const
{
    const int nodeIndex = tabsModel_.activeNodeIndex();
    if (nodeIndex < 0) {
        return {};
    }
    if (disassemblyLoadingNodes_.contains(nodeIndex)
        && !treeModel_.nodeDisassemblyLoaded(nodeIndex)) {
        return tr("// Disassembling selected source file...");
    }
    return treeModel_.nodeDisassembly(nodeIndex);
}

int DecompilerController::selectedIndex() const
{
    return treeModel_.selectedIndex();
}

void DecompilerController::decompileFile(const QString& filePath)
{
    ++openRequestId_;
    if (packageContext_) {
        packageContext_->requestStop();
    }

    if (filePath.isEmpty()) {
        clear();
        return;
    }

    packageContext_.reset();
    if (previewProvider_ != nullptr) {
        previewProvider_->clear();
    }
    clearAppIcon();
    pendingPackagePath_ = filePath;
    packagePath_ = filePath;
    const bool packageAlreadyOpen = hasPackage_;
    setHasPackage(true);
    if (packageAlreadyOpen) {
        emit packageChanged();
    }
    resetLoadingState();
    tabsModel_.clear();
    hexModel_.clear();
    treeModel_.replaceFiles({});
    const quint64 requestId = openRequestId_;
    clearActivityLog();
    setLoadingProgress(0.08);
    setBusy(true);
    setStatus(tr("Opening %1").arg(QFileInfo(filePath).fileName()));
    appendActivity(tr("Opening package session."));

    auto context = std::make_shared<HyleDecompiler::SessionContext>();
    packageContext_ = context;
    auto* watcher = new QFutureWatcher<HyleDecompiler::OpenResult>(this);
    connect(watcher, &QFutureWatcher<HyleDecompiler::OpenResult>::finished, this, [this, watcher, requestId]() {
        applyOpenResult(requestId, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([filePath, context]() {
        return HyleDecompiler::openFile(filePath, context);
    }));
}

void DecompilerController::activateIndex(int index)
{
    treeModel_.activateIndex(index);
}

void DecompilerController::openActivePreviewFile() const
{
    if (tabsModel_.activeContentMode() != QStringLiteral("media")) {
        return;
    }

    const QUrl url(tabsModel_.activeContent());
    if (url.isValid() && url.isLocalFile()) {
        QDesktopServices::openUrl(url);
    }
}

void DecompilerController::revealPackageInFileExplorer()
{
    const QString path = packagePath_.isEmpty() ? pendingPackagePath_ : packagePath_;
    const QFileInfo fileInfo(path);
    if (path.isEmpty() || !fileInfo.exists()) {
        showStatusMessage(tr("Package file no longer exists: %1").arg(path));
        return;
    }

#ifdef Q_OS_WIN
    const QString nativePath = QDir::toNativeSeparators(fileInfo.absoluteFilePath());
    const bool opened = QProcess::startDetached(
        QStringLiteral("explorer.exe"),
        { QStringLiteral("/select,%1").arg(nativePath) });
#elif defined(Q_OS_MACOS)
    const bool opened = QProcess::startDetached(
        QStringLiteral("open"),
        { QStringLiteral("-R"), fileInfo.absoluteFilePath() });
#else
    const bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif

    if (!opened) {
        showStatusMessage(tr("Failed to reveal package file: %1").arg(fileInfo.absoluteFilePath()));
    }
}

QVariantList DecompilerController::quickOpenCandidates(const QString& query) const
{
    return treeModel_.navigationCandidates(query, 80);
}

QVariantList DecompilerController::searchCandidates(const QString& query) const
{
    return treeModel_.loadedContentSearchResults(query, 80);
}

QVariantList DecompilerController::entryPointCandidates() const
{
    return treeModel_.entryPointCandidates();
}

void DecompilerController::navigateToNode(int nodeIndex)
{
    treeModel_.activateNode(nodeIndex);
}

void DecompilerController::loadActiveDisassembly()
{
    startDisassemblyLoad(tabsModel_.activeNodeIndex());
}

void DecompilerController::showStatusMessage(const QString& message)
{
    setStatus(message);
}

QString DecompilerController::formatJson(const QString& content) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || document.isNull()) {
        return content;
    }

    return QString::fromUtf8(document.toJson(QJsonDocument::Indented));
}

void DecompilerController::copyTextToClipboard(const QString& text) const
{
    if (auto* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
}

QString DecompilerController::agentPackageSummary() const
{
    QString summary;
    summary += QStringLiteral("Package: %1\n").arg(hasPackage_ ? packagePath_ : QStringLiteral("<none>"));
    summary += QStringLiteral("Status: %1\n").arg(status_);
    summary += QStringLiteral("Busy: %1\n").arg(busy_ ? QStringLiteral("true") : QStringLiteral("false"));
    summary += QStringLiteral("Active tab: %1\n").arg(tabsModel_.activePath());
    summary += QStringLiteral("Active kind: %1\n").arg(tabsModel_.activeKind());
    summary += QStringLiteral("Active content mode: %1\n").arg(tabsModel_.activeContentMode());
    summary += QStringLiteral("Diagnostics: %1\n").arg(diagnostics());

    const QVariantList entryPoints = treeModel_.entryPointCandidates();
    if (!entryPoints.isEmpty()) {
        summary += QStringLiteral("\nImportant files:\n");
        for (const QVariant& item : entryPoints) {
            summary += QStringLiteral("- %1\n").arg(formatCandidateLine(item.toMap()));
        }
    }

    return boundedAgentText(summary, 16000);
}

QString DecompilerController::agentListFiles(const QString& query, int limit) const
{
    const QVariantList candidates = treeModel_.navigationCandidates(query, limit <= 0 ? 40 : limit);
    if (candidates.isEmpty()) {
        return tr("No files matched: %1").arg(query);
    }

    QString result;
    for (const QVariant& item : candidates) {
        result += QStringLiteral("- %1\n").arg(formatCandidateLine(item.toMap()));
    }
    return boundedAgentText(result, 24000);
}

QString DecompilerController::agentSearchLoadedContent(const QString& query, int limit) const
{
    const QVariantList candidates = treeModel_.loadedContentSearchResults(query, limit <= 0 ? 40 : limit);
    if (candidates.isEmpty()) {
        return tr("No loaded content matched: %1").arg(query);
    }

    QString result;
    for (const QVariant& item : candidates) {
        result += QStringLiteral("- %1\n").arg(formatCandidateLine(item.toMap()));
    }
    return boundedAgentText(result, 24000);
}

QString DecompilerController::agentReadSource(const QString& pathOrQuery, int maxChars) const
{
    const QVariantList candidates = treeModel_.navigationCandidates(pathOrQuery, 10);
    if (candidates.isEmpty()) {
        return tr("No source file matched: %1").arg(pathOrQuery);
    }

    const QVariantMap candidate = candidates.first().toMap();
    const int nodeIndex = candidateNodeIndex(candidate);
    if (nodeIndex < 0) {
        return tr("Matched file is no longer available: %1").arg(pathOrQuery);
    }
    if (treeModel_.nodeNeedsLoad(nodeIndex)) {
        return tr("Matched file is not loaded yet: %1").arg(treeModel_.nodePath(nodeIndex));
    }

    QString text;
    text += QStringLiteral("# file: %1\n").arg(treeModel_.nodePath(nodeIndex));
    text += QStringLiteral("# kind: %1\n\n").arg(treeModel_.nodeKind(nodeIndex));
    text += treeModel_.nodeContent(nodeIndex);
    return boundedAgentText(text, maxChars);
}

QString DecompilerController::agentReadDisassembly(const QString& pathOrQuery, int maxChars) const
{
    const QVariantList candidates = treeModel_.navigationCandidates(pathOrQuery, 10);
    if (candidates.isEmpty()) {
        return tr("No source file matched: %1").arg(pathOrQuery);
    }

    const QVariantMap candidate = candidates.first().toMap();
    const int nodeIndex = candidateNodeIndex(candidate);
    if (nodeIndex < 0) {
        return tr("Matched file is no longer available: %1").arg(pathOrQuery);
    }
    if (!treeModel_.nodeHasDisassembly(nodeIndex)) {
        return tr("Matched file does not have source-file disassembly: %1").arg(treeModel_.nodePath(nodeIndex));
    }
    if (!treeModel_.nodeDisassemblyLoaded(nodeIndex)) {
        return tr("Disassembly is not loaded yet for: %1").arg(treeModel_.nodePath(nodeIndex));
    }

    QString text;
    text += QStringLiteral("# disassembly: %1\n\n").arg(treeModel_.nodePath(nodeIndex));
    text += treeModel_.nodeDisassembly(nodeIndex);
    return boundedAgentText(text, maxChars);
}

QString DecompilerController::agentEntryPoints() const
{
    const QVariantList candidates = treeModel_.entryPointCandidates();
    if (candidates.isEmpty()) {
        return tr("No entry point candidates are available.");
    }

    QString result;
    for (const QVariant& item : candidates) {
        result += QStringLiteral("- %1\n").arg(formatCandidateLine(item.toMap()));
    }
    return boundedAgentText(result, 16000);
}

QString DecompilerController::agentSignatureSummary(int maxChars) const
{
    const QVariantList candidates = treeModel_.navigationCandidates(QStringLiteral("signature"), 10);
    for (const QVariant& item : candidates) {
        const QVariantMap candidate = item.toMap();
        const int nodeIndex = candidateNodeIndex(candidate);
        if (nodeIndex < 0) {
            continue;
        }
        const QString section = candidate.value(QStringLiteral("section")).toString();
        const QString name = candidate.value(QStringLiteral("name")).toString();
        if (section != QStringLiteral("signature") && !name.contains(QStringLiteral("signature"), Qt::CaseInsensitive)) {
            continue;
        }
        if (treeModel_.nodeNeedsLoad(nodeIndex)) {
            return tr("Signature view is not loaded yet: %1").arg(treeModel_.nodePath(nodeIndex));
        }
        return boundedAgentText(treeModel_.nodeContent(nodeIndex), maxChars);
    }

    return tr("No package signature view is available.");
}

DecompilerController::AgentSnapshot DecompilerController::agentSnapshot(
    int maxFiles,
    int maxContentChars,
    int maxDisassemblyChars) const
{
    AgentSnapshot snapshot;
    snapshot.packageSummary = agentPackageSummary();
    snapshot.fileList = agentListFiles({}, maxFiles);
    snapshot.entryPoints = agentEntryPoints();
    snapshot.signatureSummary = agentSignatureSummary(maxContentChars);
    snapshot.packagePath = packagePath_;
    snapshot.packageContext = packageContext_;

    const QVariantList candidates = treeModel_.navigationCandidates({}, maxFiles <= 0 ? 500 : maxFiles);
    snapshot.files.reserve(candidates.size());
    for (const QVariant& item : candidates) {
        const QVariantMap candidate = item.toMap();
        const int nodeIndex = candidateNodeIndex(candidate);
        if (nodeIndex < 0) {
            continue;
        }

        AgentFileSnapshot file;
        file.name = candidate.value(QStringLiteral("name")).toString();
        file.path = treeModel_.nodePath(nodeIndex);
        file.kind = treeModel_.nodeKind(nodeIndex);
        file.section = candidate.value(QStringLiteral("section")).toString();
        file.contentMode = treeModel_.nodeContentMode(nodeIndex);
        file.hyleId = treeModel_.nodeHyleId(nodeIndex);
        file.loaded = !treeModel_.nodeNeedsLoad(nodeIndex);
        if (file.loaded) {
            file.content = boundedAgentText(treeModel_.nodeContent(nodeIndex), maxContentChars);
        }
        file.hasDisassembly = treeModel_.nodeHasDisassembly(nodeIndex);
        file.disassemblyLoaded = file.hasDisassembly && treeModel_.nodeDisassemblyLoaded(nodeIndex);
        if (file.disassemblyLoaded) {
            file.disassembly = boundedAgentText(treeModel_.nodeDisassembly(nodeIndex), maxDisassemblyChars);
        }
        snapshot.files.push_back(std::move(file));
    }

    return snapshot;
}

void DecompilerController::clear()
{
    ++openRequestId_;
    if (packageContext_) {
        packageContext_->requestStop();
    }
    packageContext_.reset();
    pendingPackagePath_.clear();
    if (previewProvider_ != nullptr) {
        previewProvider_->clear();
    }
    packagePath_.clear();
    clearAppIcon();
    setHasPackage(false);
    resetLoadingState();
    clearActivityLog();
    setLoadingProgress(0.0);
    tabsModel_.clear();
    hexModel_.clear();
    treeModel_.replaceFiles({});
    setStatus(tr("Ready"));
    setBusy(false);
}

void DecompilerController::setSelectedIndex(int index)
{
    treeModel_.setSelectedIndex(index);
}

void DecompilerController::setStatus(const QString& status)
{
    if (status_ == status) {
        return;
    }
    status_ = status;
    emit statusChanged();
}

void DecompilerController::setBusy(bool busy)
{
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    emit busyChanged();
}

void DecompilerController::setLoadingProgress(double progress)
{
    progress = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(loadingProgress_, progress)) {
        return;
    }
    loadingProgress_ = progress;
    emit loadingProgressChanged();
}

void DecompilerController::clearActivityLog()
{
    if (activityLog_.isEmpty()) {
        return;
    }
    activityLog_.clear();
    emit activityLogChanged();
}

void DecompilerController::appendActivity(const QString& activity)
{
    if (activity.isEmpty()) {
        return;
    }
    activityLog_.append(activity);
    constexpr qsizetype kMaxActivityLogItems = 8;
    while (activityLog_.size() > kMaxActivityLogItems) {
        activityLog_.removeFirst();
    }
    emit activityLogChanged();
}

void DecompilerController::setHasPackage(bool hasPackage)
{
    if (hasPackage_ == hasPackage) {
        return;
    }

    hasPackage_ = hasPackage;
    emit packageChanged();
}

void DecompilerController::clearAppIcon()
{
    if (appIconUrl_.isEmpty() && appIconPath_.isEmpty() && !appIconLayered_) {
        return;
    }

    appIconUrl_.clear();
    appIconDataUrl_.clear();
    appIconPath_.clear();
    appIconLayered_ = false;
    emit appIconChanged();
}

void DecompilerController::applyOpenResult(quint64 requestId, HyleDecompiler::OpenResult result)
{
    if (requestId != openRequestId_) {
        if (result.context) {
            result.context->requestStop();
        }
        return;
    }

    if (!result.error.isEmpty()) {
        if (result.context) {
            result.context->requestStop();
        }
        packageContext_.reset();
        if (previewProvider_ != nullptr) {
            previewProvider_->clear();
        }
        pendingPackagePath_.clear();
        packagePath_.clear();
        clearAppIcon();
        setHasPackage(false);
        resetLoadingState();
        setLoadingProgress(0.0);
        tabsModel_.clear();
        hexModel_.clear();
        treeModel_.replaceFiles({});
        appendActivity(result.error);
        setStatus(result.error);
        setBusy(false);
        return;
    }

    packageContext_ = std::move(result.context);
    packagePath_ = pendingPackagePath_;
    pendingPackagePath_.clear();
    clearAppIcon();
    appIconPath_ = std::move(result.appIconPath);
    appIconLayered_ = result.appIconLayered;
    appIconDataUrl_ = makeAppIconDataUrl(result.appIconBytes, appIconPath_, appIconLayered_);
    if (!appIconDataUrl_.isEmpty() && previewProvider_ != nullptr) {
        appIconUrl_ = previewProvider_->storeImage(result.appIconBytes);
    }
    if (!appIconUrl_.isEmpty() || !appIconPath_.isEmpty() || appIconLayered_) {
        emit appIconChanged();
    }
    tabsModel_.clear();
    setLoadingProgress(0.22);
    appendActivity(tr("Building file tree."));
    treeModel_.replaceFiles(std::move(result.files));
    setHasPackage(true);
    emit packageOpened(packagePath_, appIconDataUrl_);
    appendActivity(result.status);
    setStatus(result.status);
    rebuildBackgroundPreloadQueue(treeModel_.selectedNode());
    updateBackgroundPreloadProgress();
}

void DecompilerController::applySourceResult(quint64 requestId, HyleDecompiler::SourceResult result)
{
    if (requestId != openRequestId_) {
        return;
    }
    const bool wasForeground = foregroundLoadingNodes_.erase(result.nodeIndex) > 0;
    const bool wasBackground = backgroundLoadingNodes_.erase(result.nodeIndex) > 0;
    if (wasBackground) {
        activeBackgroundPreloads_ = std::max(0, activeBackgroundPreloads_ - 1);
        ++backgroundPreloadCompleted_;
    }

    if (wasBackground && !wasForeground && result.error.isEmpty()
        && cachedResultSize(result) > kMaxBackgroundCachedBytes) {
        backgroundSkippedNodes_.insert(result.nodeIndex);
        appendActivity(tr("Skipped large background item: %1").arg(result.name));
        updateBackgroundPreloadProgress();
        startNextBackgroundPreloads();
        return;
    }

    if (!result.error.isEmpty()) {
        auto document = std::make_shared<DocumentContent>();
        document->text = result.error;
        document->contentMode = QStringLiteral("text");
        treeModel_.setNodeContent(result.nodeIndex, document);
        tabsModel_.updateNode(result.nodeIndex, std::move(document));
    } else {
        auto document = std::make_shared<DocumentContent>();
        document->text = std::move(result.content);
        document->binary = std::move(result.binaryContent);
        document->diagnostics = std::move(result.diagnostics);
        document->kind = std::move(result.kind);
        document->contentMode = std::move(result.contentMode);
        if (document->contentMode == QStringLiteral("image") && previewProvider_ != nullptr) {
            document->text = previewProvider_->storeImage(document->binary);
        } else if (document->contentMode == QStringLiteral("media") && previewProvider_ != nullptr) {
            document->text = previewProvider_->storeMediaFile(result.name, document->binary);
        }
        treeModel_.setNodeContent(result.nodeIndex, document);
        tabsModel_.updateNode(result.nodeIndex, std::move(document));
    }

    if (wasForeground) {
        setStatus(result.error.isEmpty()
            ? tr("Loaded %1").arg(result.name)
            : result.error);
        setBusy(!foregroundLoadingNodes_.empty());
    } else if (wasBackground) {
        setStatus(result.error.isEmpty()
            ? tr("Cached %1").arg(result.name)
            : result.error);
        appendActivity(result.error.isEmpty()
            ? tr("Cached %1").arg(result.name)
            : result.error);
        updateBackgroundPreloadProgress();
    }
    startNextBackgroundPreloads();
}

void DecompilerController::applySourceBatchResult(quint64 requestId, HyleDecompiler::SourceBatchResult result)
{
    if (requestId != openRequestId_) {
        return;
    }

    for (auto& file : result.files) {
        applySourceResult(requestId, std::move(file));
    }
}

void DecompilerController::applyDisassemblyResult(quint64 requestId, HyleDecompiler::DisassemblyResult result)
{
    if (requestId != openRequestId_) {
        return;
    }

    disassemblyLoadingNodes_.erase(result.nodeIndex);
    treeModel_.setNodeDisassembly(
        result.nodeIndex,
        result.error.isEmpty() ? result.content : result.error);

    if (tabsModel_.activeNodeIndex() == result.nodeIndex) {
        setStatus(result.error.isEmpty()
            ? tr("Disassembled %1").arg(result.name)
            : result.error);
        emit activeDisassemblyChanged();
    }
}

void DecompilerController::openFileTab(int nodeIndex)
{
    tabsModel_.openOrActivate(
        nodeIndex,
        treeModel_.nodeName(nodeIndex),
        treeModel_.nodePath(nodeIndex),
        treeModel_.nodeKind(nodeIndex),
        treeModel_.nodeDocument(nodeIndex),
        treeModel_.nodeContentMode(nodeIndex),
        treeModel_.nodeNeedsLoad(nodeIndex));

    startNodeLoad(nodeIndex, true);
    rebuildBackgroundPreloadQueue(nodeIndex);
}

void DecompilerController::startNodeLoad(int nodeIndex, bool foreground)
{
    if (!treeModel_.nodeNeedsLoad(nodeIndex)) {
        return;
    }

    const QString name = treeModel_.nodeName(nodeIndex);
    const QString section = treeModel_.nodeSection(nodeIndex);
    const bool alreadyForeground = foregroundLoadingNodes_.contains(nodeIndex);
    const bool alreadyBackground = backgroundLoadingNodes_.contains(nodeIndex);

    if (foreground) {
        foregroundLoadingNodes_.insert(nodeIndex);
        setBusy(true);
        tabsModel_.setNodeLoading(nodeIndex, true);
        const bool cachedSource = section == QStringLiteral("source")
            && HyleDecompiler::isSourceFileCached(packageContext_, treeModel_.nodeHyleId(nodeIndex));
        setStatus(section == QStringLiteral("resource") || section == QStringLiteral("signature") || section == QStringLiteral("summary")
            ? tr("Loading %1").arg(name)
            : cachedSource ? tr("Opening cached %1").arg(name) : tr("Decompiling %1").arg(name));
        if (alreadyForeground || alreadyBackground) {
            return;
        }
    } else if (alreadyForeground || alreadyBackground) {
        return;
    }

    if (!foreground) {
        backgroundLoadingNodes_.insert(nodeIndex);
        ++activeBackgroundPreloads_;
        setBusy(true);
        setStatus(section == QStringLiteral("resource") || section == QStringLiteral("signature") || section == QStringLiteral("summary")
            ? tr("Caching %1").arg(name)
            : tr("Pre-decompiling %1").arg(name));
        appendActivity(section == QStringLiteral("resource") || section == QStringLiteral("signature") || section == QStringLiteral("summary")
            ? tr("Caching %1").arg(name)
            : tr("Pre-decompiling %1").arg(name));
    }

    const quint64 requestId = openRequestId_;
    const auto hyleId = treeModel_.nodeHyleId(nodeIndex);
    const auto context = packageContext_;
    const QString packagePath = packagePath_;

    auto* watcher = new QFutureWatcher<HyleDecompiler::SourceResult>(this);
    connect(watcher, &QFutureWatcher<HyleDecompiler::SourceResult>::finished, this, [this, watcher, requestId]() {
        applySourceResult(requestId, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([context, packagePath, nodeIndex, hyleId, name, section]() {
        if (section == QStringLiteral("resource")) {
            return HyleDecompiler::readResourceContent(context, nodeIndex, hyleId, name);
        }
        if (section == QStringLiteral("signature")) {
            return HyleDecompiler::readSignatureContent(packagePath, nodeIndex, name);
        }
        if (section == QStringLiteral("summary")) {
            return HyleDecompiler::readSummaryContent(context, nodeIndex, name);
        }
        return HyleDecompiler::decompileSourceFile(context, nodeIndex, hyleId, name);
    }));
}

void DecompilerController::startDisassemblyLoad(int nodeIndex)
{
    if (!treeModel_.nodeHasDisassembly(nodeIndex)
        || treeModel_.nodeDisassemblyLoaded(nodeIndex)
        || disassemblyLoadingNodes_.contains(nodeIndex)) {
        return;
    }

    disassemblyLoadingNodes_.insert(nodeIndex);
    emit activeDisassemblyChanged();

    const quint64 requestId = openRequestId_;
    const auto sourceFileId = treeModel_.nodeHyleId(nodeIndex);
    const auto context = packageContext_;
    const QString name = treeModel_.nodeName(nodeIndex);

    setStatus(tr("Disassembling %1").arg(name));

    auto* watcher = new QFutureWatcher<HyleDecompiler::DisassemblyResult>(this);
    connect(watcher, &QFutureWatcher<HyleDecompiler::DisassemblyResult>::finished, this, [this, watcher, requestId]() {
        applyDisassemblyResult(requestId, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([context, nodeIndex, sourceFileId, name]() {
        return HyleDecompiler::disassembleSourceFileText(context, nodeIndex, sourceFileId, name);
    }));
}

void DecompilerController::resetLoadingState()
{
    foregroundLoadingNodes_.clear();
    backgroundLoadingNodes_.clear();
    backgroundSkippedNodes_.clear();
    disassemblyLoadingNodes_.clear();
    backgroundPreloadQueue_.clear();
    activeBackgroundPreloads_ = 0;
    backgroundPreloadTotal_ = 0;
    backgroundPreloadCompleted_ = 0;
}

void DecompilerController::rebuildBackgroundPreloadQueue(int centerNode)
{
    backgroundPreloadQueue_.clear();
    backgroundPreloadCompleted_ = 0;
    for (int nodeIndex : treeModel_.prioritizedPreloadNodeIndices(centerNode, kMaxQueuedBackgroundPreloads)) {
        if (foregroundLoadingNodes_.contains(nodeIndex)
            || backgroundLoadingNodes_.contains(nodeIndex)
            || backgroundSkippedNodes_.contains(nodeIndex)) {
            continue;
        }
        backgroundPreloadQueue_.push_back(nodeIndex);
    }
    backgroundPreloadTotal_ = static_cast<int>(backgroundPreloadQueue_.size());
    if (backgroundPreloadTotal_ > 0) {
        appendActivity(tr("Preparing content cache for %1 item(s).").arg(backgroundPreloadTotal_));
    }
    startNextBackgroundPreloads();
}

void DecompilerController::startNextBackgroundPreloads()
{
    while (activeBackgroundPreloads_ < kMaxBackgroundPreloads && !backgroundPreloadQueue_.empty()) {
        std::vector<int> sourceBatch;

        while (!backgroundPreloadQueue_.empty()
               && sourceBatch.size() < static_cast<std::size_t>(kMaxBackgroundSourceBatchSize)) {
            const int nodeIndex = backgroundPreloadQueue_.front();
            backgroundPreloadQueue_.pop_front();
            if (!treeModel_.nodeNeedsLoad(nodeIndex)
                || foregroundLoadingNodes_.contains(nodeIndex)
                || backgroundLoadingNodes_.contains(nodeIndex)
                || backgroundSkippedNodes_.contains(nodeIndex)) {
                continue;
            }
            if (treeModel_.nodeSection(nodeIndex) == QStringLiteral("source")) {
                sourceBatch.push_back(nodeIndex);
                continue;
            }

            if (!sourceBatch.empty()) {
                backgroundPreloadQueue_.push_front(nodeIndex);
                break;
            }

            startNodeLoad(nodeIndex, false);
            break;
        }

        if (!sourceBatch.empty()) {
            startSourceBatchLoad(std::move(sourceBatch));
        }
    }
    updateBackgroundPreloadProgress();
}

void DecompilerController::startSourceBatchLoad(std::vector<int> nodeIndices)
{
    std::vector<HyleDecompiler::SourceRequest> requests;
    requests.reserve(nodeIndices.size());

    for (int nodeIndex : nodeIndices) {
        if (!treeModel_.nodeNeedsLoad(nodeIndex)
            || foregroundLoadingNodes_.contains(nodeIndex)
            || backgroundLoadingNodes_.contains(nodeIndex)
            || backgroundSkippedNodes_.contains(nodeIndex)
            || treeModel_.nodeSection(nodeIndex) != QStringLiteral("source")) {
            continue;
        }

        backgroundLoadingNodes_.insert(nodeIndex);
        ++activeBackgroundPreloads_;
        requests.push_back({
            nodeIndex,
            treeModel_.nodeHyleId(nodeIndex),
            treeModel_.nodeName(nodeIndex),
            treeModel_.nodePath(nodeIndex)
        });
    }

    if (requests.empty()) {
        return;
    }

    setBusy(true);
    setStatus(tr("Pre-decompiling %1 source file(s)").arg(static_cast<int>(requests.size())));
    appendActivity(tr("Pre-decompiling %1 source file(s)").arg(static_cast<int>(requests.size())));

    const quint64 requestId = openRequestId_;
    const auto context = packageContext_;

    auto* watcher = new QFutureWatcher<HyleDecompiler::SourceBatchResult>(this);
    connect(watcher, &QFutureWatcher<HyleDecompiler::SourceBatchResult>::finished, this, [this, watcher, requestId]() {
        applySourceBatchResult(requestId, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([context, requests = std::move(requests)]() mutable {
        return HyleDecompiler::decompileSourceFiles(context, std::move(requests));
    }));
}

void DecompilerController::updateBackgroundPreloadProgress()
{
    if (!foregroundLoadingNodes_.empty()) {
        return;
    }

    if (backgroundPreloadTotal_ <= 0) {
        setLoadingProgress(1.0);
        setBusy(false);
        return;
    }

    const int inFlight = activeBackgroundPreloads_;
    const int queued = static_cast<int>(backgroundPreloadQueue_.size());
    const int completed = std::max(0, backgroundPreloadTotal_ - queued - inFlight);
    backgroundPreloadCompleted_ = std::max(backgroundPreloadCompleted_, completed);
    const double ratio = static_cast<double>(backgroundPreloadCompleted_) / static_cast<double>(backgroundPreloadTotal_);
    setLoadingProgress(0.25 + ratio * 0.75);

    if (queued == 0 && inFlight == 0) {
        setLoadingProgress(1.0);
        setStatus(tr("Ready"));
        appendActivity(tr("Content cache is ready."));
        setBusy(false);
    } else {
        setBusy(true);
    }
}

void DecompilerController::refreshActiveHexDocument()
{
    const QByteArray binary = tabsModel_.activeBinaryContent();
    if (binary.isEmpty()) {
        hexModel_.clear();
        return;
    }

    hexModel_.setDocument(
        tabsModel_.activePath(),
        tabsModel_.activeKind(),
        binary);
}
