#include "controller/DecompilerController.h"

#include "core/ResourcePreviewProvider.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>
#include <memory>

namespace {

constexpr int kMaxBackgroundPreloads = 2;
constexpr int kMaxQueuedBackgroundPreloads = 512;
constexpr qsizetype kMaxBackgroundCachedBytes = 2 * 1024 * 1024;

qsizetype cachedResultSize(const HyleDecompiler::SourceResult& result)
{
    return result.content.size() * static_cast<qsizetype>(sizeof(QChar))
        + result.binaryContent.size()
        + result.diagnostics.size() * static_cast<qsizetype>(sizeof(QChar));
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
    packagePath_ = filePath;
    resetLoadingState();
    const quint64 requestId = openRequestId_;
    setBusy(true);
    setStatus(tr("Decompiling %1").arg(QFileInfo(filePath).fileName()));

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

void DecompilerController::clear()
{
    ++openRequestId_;
    if (packageContext_) {
        packageContext_->requestStop();
    }
    packageContext_.reset();
    if (previewProvider_ != nullptr) {
        previewProvider_->clear();
    }
    packagePath_.clear();
    resetLoadingState();
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
        packagePath_.clear();
        resetLoadingState();
        tabsModel_.clear();
        hexModel_.clear();
        treeModel_.replaceFiles({});
        setStatus(result.error);
        setBusy(false);
        return;
    }

    packageContext_ = std::move(result.context);
    tabsModel_.clear();
    treeModel_.replaceFiles(std::move(result.files));
    if (foregroundLoadingNodes_.empty()) {
        setBusy(false);
        setStatus(result.status);
    }
    rebuildBackgroundPreloadQueue(treeModel_.selectedNode());
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
    }

    if (wasBackground && !wasForeground && result.error.isEmpty()
        && cachedResultSize(result) > kMaxBackgroundCachedBytes) {
        backgroundSkippedNodes_.insert(result.nodeIndex);
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
    }
    startNextBackgroundPreloads();
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
        setStatus(section == QStringLiteral("resource") || section == QStringLiteral("signature") || section == QStringLiteral("summary")
            ? tr("Loading %1").arg(name)
            : tr("Decompiling %1").arg(name));
        if (alreadyForeground || alreadyBackground) {
            return;
        }
    } else if (alreadyForeground || alreadyBackground) {
        return;
    }

    if (!foreground) {
        backgroundLoadingNodes_.insert(nodeIndex);
        ++activeBackgroundPreloads_;
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

void DecompilerController::resetLoadingState()
{
    foregroundLoadingNodes_.clear();
    backgroundLoadingNodes_.clear();
    backgroundSkippedNodes_.clear();
    backgroundPreloadQueue_.clear();
    activeBackgroundPreloads_ = 0;
}

void DecompilerController::rebuildBackgroundPreloadQueue(int centerNode)
{
    backgroundPreloadQueue_.clear();
    for (int nodeIndex : treeModel_.prioritizedPreloadNodeIndices(centerNode, kMaxQueuedBackgroundPreloads)) {
        if (foregroundLoadingNodes_.contains(nodeIndex)
            || backgroundLoadingNodes_.contains(nodeIndex)
            || backgroundSkippedNodes_.contains(nodeIndex)) {
            continue;
        }
        backgroundPreloadQueue_.push_back(nodeIndex);
    }
    startNextBackgroundPreloads();
}

void DecompilerController::startNextBackgroundPreloads()
{
    while (activeBackgroundPreloads_ < kMaxBackgroundPreloads && !backgroundPreloadQueue_.empty()) {
        const int nodeIndex = backgroundPreloadQueue_.front();
        backgroundPreloadQueue_.pop_front();
        if (!treeModel_.nodeNeedsLoad(nodeIndex)
            || foregroundLoadingNodes_.contains(nodeIndex)
            || backgroundLoadingNodes_.contains(nodeIndex)
            || backgroundSkippedNodes_.contains(nodeIndex)) {
            continue;
        }
        startNodeLoad(nodeIndex, false);
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
