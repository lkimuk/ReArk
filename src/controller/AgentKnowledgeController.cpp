#include "controller/AgentKnowledgeController.h"

#include "controller/AgentSettings.h"

#ifdef REARK_HAS_WUWE
#include <wuwe/agent/knowledge/knowledge_document_loader.hpp>
#include <wuwe/agent/knowledge/knowledge_pipeline.hpp>
#include <wuwe/agent/memory/openai_embedding_model.hpp>
#endif

#include <QDateTime>
#include <QFileInfo>
#include <QMetaObject>
#include <QPointer>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <thread>
#include <utility>

namespace {

QString pathFromUrl(const QUrl& url)
{
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return url.toString();
}

std::string toStdString(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString fromStdString(const std::string& value)
{
    return QString::fromUtf8(value.data(), qsizetype(value.size()));
}

QString displayNameForSource(const QString& source, const QString& kind)
{
    if (kind == QStringLiteral("url")) {
        const QUrl url(source);
        return url.host().isEmpty() ? source : url.host();
    }

    const QFileInfo info(source);
    if (!info.fileName().isEmpty()) {
        return info.fileName();
    }
    return source;
}

QString stateLabel(const QString& state)
{
    if (state == QStringLiteral("loading")) {
        return AgentKnowledgeController::tr("Loading");
    }
    if (state == QStringLiteral("indexing")) {
        return AgentKnowledgeController::tr("Indexing");
    }
    if (state == QStringLiteral("ready")) {
        return AgentKnowledgeController::tr("Ready");
    }
    if (state == QStringLiteral("failed")) {
        return AgentKnowledgeController::tr("Failed");
    }
    if (state == QStringLiteral("cancelled")) {
        return AgentKnowledgeController::tr("Cancelled");
    }
    return AgentKnowledgeController::tr("Pending");
}

QString userFacingReferenceError(const QString& message)
{
    const QString folded = message.toCaseFolded();
    if (message.contains(QStringLiteral("OPERATION_TIMEOUT"))
        || folded.contains(QStringLiteral("timed out"))
        || folded.contains(QStringLiteral("timeout"))) {
        return AgentKnowledgeController::tr(
            "Embedding request timed out. Check the Embedding Base URL, API key, model, and network.");
    }
    if (folded.contains(QStringLiteral("embedding request failed"))) {
        return AgentKnowledgeController::tr(
            "Embedding request failed. Check the Embedding Base URL, API key, and model.");
    }
    return message;
}

} // namespace

struct AgentKnowledgeController::Runtime {
#ifdef REARK_HAS_WUWE
    AgentSettings settings;
    std::unique_ptr<wuwe::agent::knowledge::knowledge_pipeline> pipeline;
    wuwe::agent::knowledge::knowledge_document_loader loader;
    std::mutex mutex;

    Runtime(AgentSettings agentSettings, wuwe::agent::knowledge::knowledge_pipeline knowledgePipeline)
        : settings(std::move(agentSettings))
        , pipeline(std::make_unique<wuwe::agent::knowledge::knowledge_pipeline>(std::move(knowledgePipeline)))
        , loader(wuwe::agent::knowledge::knowledge_document_loader::make_default(
              toStdString(settings.tikaUrl.trimmed())))
    {
    }
#endif
};

AgentKnowledgeController::AgentKnowledgeController(QObject* parent)
    : QObject(parent)
    , sessionId_(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    setStatus(available() ? tr("Reference knowledge ready.") : tr("Wuwe knowledge runtime is not available."));
}

AgentKnowledgeController::~AgentKnowledgeController()
{
    requestActiveTaskStop();
    workers_.clear();
}

bool AgentKnowledgeController::available() const
{
#ifdef REARK_HAS_WUWE
    return true;
#else
    return false;
#endif
}

bool AgentKnowledgeController::busy() const
{
    return busy_;
}

QString AgentKnowledgeController::status() const
{
    return status_;
}

QString AgentKnowledgeController::errorMessage() const
{
    return errorMessage_;
}

QVariantList AgentKnowledgeController::references() const
{
    std::scoped_lock lock(mutex_);
    return references_;
}

bool AgentKnowledgeController::hasReadyReferences() const
{
    std::scoped_lock lock(mutex_);
    for (const QVariant& item : references_) {
        if (item.toMap().value(QStringLiteral("state")).toString() == QStringLiteral("ready")) {
            return true;
        }
    }
    return false;
}

void AgentKnowledgeController::addReferenceFile(const QUrl& fileUrl)
{
    addReferenceSource(pathFromUrl(fileUrl), QStringLiteral("file"));
}

void AgentKnowledgeController::addReferenceFolder(const QUrl& folderUrl)
{
    addReferenceSource(pathFromUrl(folderUrl), QStringLiteral("folder"));
}

void AgentKnowledgeController::addReferenceUrl(const QString& url)
{
    addReferenceSource(url.trimmed(), QStringLiteral("url"));
}

void AgentKnowledgeController::removeReference(const QString& id)
{
    bool shouldStopActiveTask = false;
    {
        std::scoped_lock lock(mutex_);
        for (qsizetype i = 0; i < references_.size(); ++i) {
            if (references_.at(i).toMap().value(QStringLiteral("id")).toString() == id) {
                references_.removeAt(i);
                break;
            }
        }
        shouldStopActiveTask = activeReferenceId_ == id;
    }
    if (shouldStopActiveTask) {
        requestActiveTaskStop();
    }
#ifdef REARK_HAS_WUWE
    std::shared_ptr<Runtime> runtime;
    QString sessionId;
    {
        std::scoped_lock lock(mutex_);
        runtime = runtime_;
        sessionId = sessionId_;
    }
    if (runtime && runtime->pipeline) {
        std::scoped_lock runtimeLock(runtime->mutex);
        const QString prefix = id + QStringLiteral("-");
        for (const auto& document : runtime->pipeline->retriever().list_documents()) {
            if (fromStdString(document.id).startsWith(prefix)) {
                runtime->pipeline->retriever().erase_document(document.id);
            }
        }
    }
#endif
    if (shouldStopActiveTask) {
        setBusy(false);
        setStatus(tr("Reference removed."));
    }
    emit referencesChanged();
}

void AgentKnowledgeController::clearSessionReferences()
{
    requestActiveTaskStop();
    {
        std::scoped_lock lock(mutex_);
        references_.clear();
        sessionId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
        activeTaskId_.clear();
        activeReferenceId_.clear();
        activeStopSource_.reset();
    }
    resetRuntime();
    setBusy(false);
    setErrorMessage({});
    setStatus(available() ? tr("Reference knowledge cleared.") : tr("Wuwe knowledge runtime is not available."));
    emit referencesChanged();
}

void AgentKnowledgeController::cancel()
{
    requestActiveTaskStop();
    setStatus(tr("Cancelling reference indexing..."));
}

QString AgentKnowledgeController::referenceSummaryForPrompt() const
{
    QStringList lines;
    std::scoped_lock lock(mutex_);
    for (const QVariant& item : references_) {
        const QVariantMap reference = item.toMap();
        if (reference.value(QStringLiteral("state")).toString() != QStringLiteral("ready")) {
            continue;
        }
        lines.append(QStringLiteral("- %1 (%2)")
            .arg(reference.value(QStringLiteral("displayName")).toString(),
                 reference.value(QStringLiteral("kind")).toString()));
    }
    return lines.join(QLatin1Char('\n'));
}

QString AgentKnowledgeController::searchReferencesForAgent(
    const QString& query,
    int limit,
    std::stop_token stopToken) const
{
    if (stopToken.stop_requested()) {
        return tr("Reference knowledge search cancelled.");
    }
    if (!available()) {
        return tr("Reference knowledge is not available in this ReArk build.");
    }

#ifdef REARK_HAS_WUWE
    std::shared_ptr<Runtime> runtime;
    QString sessionId;
    {
        std::scoped_lock lock(mutex_);
        runtime = runtime_;
        sessionId = sessionId_;
    }
    if (!runtime || !runtime->pipeline) {
        return tr("No reference knowledge has been indexed for this chat.");
    }

    try {
        std::scoped_lock runtimeLock(runtime->mutex);
        wuwe::agent::knowledge::knowledge_query knowledgeQuery;
        knowledgeQuery.text = toStdString(query.trimmed());
        knowledgeQuery.limit = static_cast<std::size_t>(std::clamp(limit <= 0 ? 6 : limit, 1, 12));
        knowledgeQuery.candidate_limit = knowledgeQuery.limit * 4;
        knowledgeQuery.filters["reark_session_id"] = toStdString(sessionId);

        const auto results = runtime->pipeline->retriever().retrieve(std::move(knowledgeQuery));
        if (results.empty()) {
            return tr("No reference knowledge matched the query.");
        }

        QString output;
        int index = 1;
        for (const auto& result : results) {
            if (stopToken.stop_requested()) {
                return tr("Reference knowledge search cancelled.");
            }
            const QString title = fromStdString(
                result.chunk.title.empty() ? result.chunk.document_id : result.chunk.title);
            const QString source = fromStdString(result.chunk.source_uri);
            output += QStringLiteral("[%1] %2").arg(index++).arg(title);
            if (!source.isEmpty()) {
                output += QStringLiteral(" | %1").arg(source);
            }
            if (result.chunk.start_line != 0) {
                output += QStringLiteral(" | lines %1-%2")
                    .arg(result.chunk.start_line)
                    .arg(result.chunk.end_line);
            }
            output += QStringLiteral(" | score %1\n").arg(result.score, 0, 'f', 3);
            output += fromStdString(result.chunk.content).simplified();
            output += QStringLiteral("\n\n");
        }
        return output.trimmed();
    } catch (const std::exception& ex) {
        return tr("Reference knowledge search failed: %1").arg(QString::fromUtf8(ex.what()));
    }
#else
    Q_UNUSED(query)
    Q_UNUSED(limit)
    return {};
#endif
}

void AgentKnowledgeController::addReferenceSource(const QString& source, const QString& kind)
{
    if (source.trimmed().isEmpty()) {
        return;
    }
    if (!available()) {
        setErrorMessage(tr("Wuwe knowledge runtime is not available in this build."));
        return;
    }
    if (busy_) {
        setErrorMessage(tr("A reference document is already being indexed."));
        return;
    }

    QString runtimeError;
    const auto runtime = ensureRuntime(&runtimeError);
    if (!runtime) {
        setErrorMessage(runtimeError);
        setStatus(runtimeError);
        return;
    }

    const QString id = nextReferenceId();
    const QString taskId = nextTaskId();
    auto stopSource = std::make_shared<std::stop_source>();
    QVariantMap reference;
    reference.insert(QStringLiteral("id"), id);
    reference.insert(QStringLiteral("displayName"), displayNameForSource(source, kind));
    reference.insert(QStringLiteral("source"), source);
    reference.insert(QStringLiteral("kind"), kind);
    reference.insert(QStringLiteral("state"), QStringLiteral("loading"));
    reference.insert(QStringLiteral("stateLabel"), stateLabel(QStringLiteral("loading")));
    reference.insert(QStringLiteral("progress"), 5);
    reference.insert(QStringLiteral("error"), QString());
    reference.insert(QStringLiteral("documentCount"), 0);
    reference.insert(QStringLiteral("chunkCount"), 0);

    QString sessionId;
    {
        std::scoped_lock lock(mutex_);
        references_.append(reference);
        activeTaskId_ = taskId;
        activeReferenceId_ = id;
        activeStopSource_ = stopSource;
        sessionId = sessionId_;
    }
    emit referencesChanged();
    setErrorMessage({});
    setStatus(tr("Loading reference: %1").arg(reference.value(QStringLiteral("displayName")).toString()));
    setBusy(true);

    QPointer<AgentKnowledgeController> self(this);
    workers_.emplace_back([self, runtime, id, taskId, source, kind, sessionId, stopSource](
                              std::stop_token threadStopToken) {
        Q_UNUSED(kind)
#ifdef REARK_HAS_WUWE
        const std::stop_token stopToken = stopSource->get_token();
        const auto eraseCapturedReferenceDocuments = [runtime, id] {
            if (!runtime || !runtime->pipeline) {
                return;
            }
            const QString prefix = id + QStringLiteral("-");
            std::scoped_lock runtimeLock(runtime->mutex);
            for (const auto& document : runtime->pipeline->retriever().list_documents()) {
                if (fromStdString(document.id).startsWith(prefix)) {
                    runtime->pipeline->retriever().erase_document(document.id);
                }
            }
        };
        const auto taskCanContinue = [&]() {
            return self
                && !stopToken.stop_requested()
                && !threadStopToken.stop_requested()
                && self->isActiveTask(taskId, id)
                && self->referenceExists(id);
        };
        const auto finishCancelled = [&]() {
            eraseCapturedReferenceDocuments();
            QMetaObject::invokeMethod(self.data(), [self, id, taskId] {
                if (!self) {
                    return;
                }
                if (self->isActiveTask(taskId, id)) {
                    self->clearActiveTask(taskId);
                    self->updateReference(id, QStringLiteral("cancelled"), 0);
                    self->setBusy(false);
                }
            }, Qt::QueuedConnection);
        };

        try {
            if (!taskCanContinue()) {
                if (self) {
                    finishCancelled();
                }
                return;
            }
            QMetaObject::invokeMethod(self.data(), [self, id, taskId] {
                if (self && self->isActiveTask(taskId, id)) {
                    self->updateReference(id, QStringLiteral("loading"), 12);
                }
            }, Qt::QueuedConnection);

            wuwe::agent::knowledge::knowledge_document_load_options loadOptions;
            loadOptions.metadata["reark_scope"] = "session";
            loadOptions.metadata["reark_session_id"] = toStdString(sessionId);
            loadOptions.metadata["reark_reference_id"] = toStdString(id);
            loadOptions.metadata["source_kind"] = toStdString(kind);

            auto documents = runtime->loader.load(std::filesystem::path(toStdString(source)), std::move(loadOptions));
            if (documents.empty()) {
                throw std::runtime_error("no supported documents found in reference source");
            }
            if (!taskCanContinue()) {
                finishCancelled();
                return;
            }

            QMetaObject::invokeMethod(self.data(), [self, id, taskId] {
                if (self && self->isActiveTask(taskId, id)) {
                    self->updateReference(id, QStringLiteral("indexing"), 35);
                }
            }, Qt::QueuedConnection);

            qsizetype chunkCount = 0;
            {
                std::scoped_lock runtimeLock(runtime->mutex);
                for (auto& document : documents) {
                    if (!taskCanContinue()) {
                        break;
                    }
                    document.id = toStdString(id) + "-" + document.id;
                    document.metadata["reark_scope"] = "session";
                    document.metadata["reark_session_id"] = toStdString(sessionId);
                    document.metadata["reark_reference_id"] = toStdString(id);
                    const auto chunks = runtime->pipeline->retriever().ingest(std::move(document));
                    chunkCount += static_cast<qsizetype>(chunks.size());
                }
            }
            if (!taskCanContinue()) {
                finishCancelled();
                return;
            }

            QMetaObject::invokeMethod(self.data(), [self, id, taskId, documentCount = qsizetype(documents.size()), chunkCount] {
                if (self && self->isActiveTask(taskId, id)) {
                    self->finishReference(id, documentCount, chunkCount);
                    self->clearActiveTask(taskId);
                }
            }, Qt::QueuedConnection);
        } catch (const std::exception& ex) {
            const QString message = userFacingReferenceError(QString::fromUtf8(ex.what()));
            QMetaObject::invokeMethod(self.data(), [self, id, taskId, message] {
                if (self && self->isActiveTask(taskId, id)) {
                    self->clearActiveTask(taskId);
                    if (self->updateReference(id, QStringLiteral("failed"), 0, message)) {
                        self->setErrorMessage(message);
                        self->setStatus(AgentKnowledgeController::tr("Reference indexing failed."));
                        self->setBusy(false);
                    }
                }
            }, Qt::QueuedConnection);
        }
#else
        Q_UNUSED(runtime)
        Q_UNUSED(id)
        Q_UNUSED(taskId)
        Q_UNUSED(source)
        Q_UNUSED(sessionId)
        Q_UNUSED(stopSource)
#endif
    });
}

void AgentKnowledgeController::setBusy(bool busy)
{
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    emit busyChanged();
}

void AgentKnowledgeController::setStatus(const QString& status)
{
    if (status_ == status) {
        return;
    }
    status_ = status;
    emit statusChanged();
}

void AgentKnowledgeController::setErrorMessage(const QString& errorMessage)
{
    if (errorMessage_ == errorMessage) {
        return;
    }
    errorMessage_ = errorMessage;
    emit errorMessageChanged();
}

bool AgentKnowledgeController::updateReference(
    const QString& id,
    const QString& state,
    int progress,
    const QString& error)
{
    bool changed = false;
    {
        std::scoped_lock lock(mutex_);
        for (QVariant& item : references_) {
            QVariantMap reference = item.toMap();
            if (reference.value(QStringLiteral("id")).toString() != id) {
                continue;
            }
            reference.insert(QStringLiteral("state"), state);
            reference.insert(QStringLiteral("stateLabel"), stateLabel(state));
            reference.insert(QStringLiteral("progress"), progress);
            reference.insert(QStringLiteral("error"), error);
            item = reference;
            changed = true;
            break;
        }
    }
    if (changed) {
        emit referencesChanged();
        setStatus(error.isEmpty() ? stateLabel(state) : error);
    }
    return changed;
}

void AgentKnowledgeController::finishReference(const QString& id, qsizetype documentCount, qsizetype chunkCount)
{
    bool changed = false;
    {
        std::scoped_lock lock(mutex_);
        for (QVariant& item : references_) {
            QVariantMap reference = item.toMap();
            if (reference.value(QStringLiteral("id")).toString() != id) {
                continue;
            }
            reference.insert(QStringLiteral("state"), QStringLiteral("ready"));
            reference.insert(QStringLiteral("stateLabel"), stateLabel(QStringLiteral("ready")));
            reference.insert(QStringLiteral("progress"), 100);
            reference.insert(QStringLiteral("error"), QString());
            reference.insert(QStringLiteral("documentCount"), documentCount);
            reference.insert(QStringLiteral("chunkCount"), chunkCount);
            item = reference;
            changed = true;
            break;
        }
    }
    if (changed) {
        emit referencesChanged();
        setStatus(tr("Reference ready."));
        setBusy(false);
    }
}

void AgentKnowledgeController::resetRuntime()
{
    std::scoped_lock lock(mutex_);
    runtime_.reset();
}

QString AgentKnowledgeController::nextReferenceId()
{
    return QStringLiteral("ref-%1-%2")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(nextReferenceNumber_++);
}

QString AgentKnowledgeController::nextTaskId()
{
    return QStringLiteral("task-%1-%2")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(nextTaskNumber_++);
}

void AgentKnowledgeController::requestActiveTaskStop()
{
    std::shared_ptr<std::stop_source> stopSource;
    {
        std::scoped_lock lock(mutex_);
        stopSource = activeStopSource_;
    }
    if (stopSource) {
        stopSource->request_stop();
    }
}

void AgentKnowledgeController::clearActiveTask(const QString& taskId)
{
    std::scoped_lock lock(mutex_);
    if (activeTaskId_ != taskId) {
        return;
    }

    activeTaskId_.clear();
    activeReferenceId_.clear();
    activeStopSource_.reset();
}

bool AgentKnowledgeController::isActiveTask(const QString& taskId, const QString& referenceId) const
{
    std::scoped_lock lock(mutex_);
    return activeTaskId_ == taskId && activeReferenceId_ == referenceId;
}

bool AgentKnowledgeController::referenceExists(const QString& referenceId) const
{
    std::scoped_lock lock(mutex_);
    return std::any_of(references_.cbegin(), references_.cend(), [&](const QVariant& item) {
        return item.toMap().value(QStringLiteral("id")).toString() == referenceId;
    });
}

std::shared_ptr<AgentKnowledgeController::Runtime> AgentKnowledgeController::ensureRuntime(QString* errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    if (!available()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Wuwe knowledge runtime is not available in this build.");
        }
        return {};
    }

#ifdef REARK_HAS_WUWE
    {
        std::scoped_lock lock(mutex_);
        if (runtime_) {
            return runtime_;
        }
    }

    const AgentSettings settings = AgentSettingsStore::load();
    const QString validationMessage = AgentSettingsStore::knowledgeValidationMessage(settings);
    if (!validationMessage.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = validationMessage;
        }
        return {};
    }

    try {
        auto embedding = std::make_shared<wuwe::agent::memory::openai_embedding_model>(
            wuwe::agent::memory::openai_embedding_model_config {
                .base_url = toStdString(settings.embeddingBaseUrl),
                .api_key = toStdString(settings.embeddingApiKey),
                .model = toStdString(settings.embeddingModel),
                .timeout = 30000,
            });

        auto pipeline = wuwe::agent::knowledge::knowledge_pipeline::make()
            .local()
            .with_embedding_model(std::move(embedding))
            .with_splitter(wuwe::agent::knowledge::knowledge_splitter(
                wuwe::agent::knowledge::chunking_policy {
                    .max_chars = 1800,
                    .overlap_chars = 240,
                    .include_document_summary_chunk = true,
                    .document_summary_chars = 4000,
                }))
            .with_indexing_policy(wuwe::agent::knowledge::knowledge_indexing_policy {
                .embedding_provider = "openai-compatible",
                .embedding_model = toStdString(settings.embeddingModel),
                .index_schema_version = 1,
                .embedding_batch_size = 32,
            })
            .build();

        auto runtime = std::make_shared<Runtime>(settings, std::move(pipeline));
        {
            std::scoped_lock lock(mutex_);
            runtime_ = runtime;
        }
        return runtime;
    } catch (const std::exception& ex) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Failed to initialize reference knowledge: %1")
                .arg(QString::fromUtf8(ex.what()));
        }
        return {};
    }
#else
    return {};
#endif
}
