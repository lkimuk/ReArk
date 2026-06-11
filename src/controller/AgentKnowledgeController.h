#ifndef REARK_AGENT_KNOWLEDGE_CONTROLLER_H
#define REARK_AGENT_KNOWLEDGE_CONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QVariantList>

#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

class AgentKnowledgeController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QVariantList references READ references NOTIFY referencesChanged)
    Q_PROPERTY(bool hasReadyReferences READ hasReadyReferences NOTIFY referencesChanged)

public:
    explicit AgentKnowledgeController(QObject* parent = nullptr);
    ~AgentKnowledgeController() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] QString status() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] QVariantList references() const;
    [[nodiscard]] bool hasReadyReferences() const;

    Q_INVOKABLE void addReferenceFile(const QUrl& fileUrl);
    Q_INVOKABLE void addReferenceFolder(const QUrl& folderUrl);
    Q_INVOKABLE void addReferenceUrl(const QString& url);
    Q_INVOKABLE void removeReference(const QString& id);
    Q_INVOKABLE void clearSessionReferences();
    Q_INVOKABLE void cancel();

    [[nodiscard]] QString referenceSummaryForPrompt() const;
    [[nodiscard]] QString searchReferencesForAgent(
        const QString& query,
        int limit,
        std::stop_token stopToken) const;

signals:
    void busyChanged();
    void statusChanged();
    void errorMessageChanged();
    void referencesChanged();

private:
    struct Runtime;

    void addReferenceSource(const QString& source, const QString& kind);
    void setBusy(bool busy);
    void setStatus(const QString& status);
    void setErrorMessage(const QString& errorMessage);
    bool updateReference(const QString& id, const QString& state, int progress, const QString& error = {});
    void finishReference(const QString& id, qsizetype documentCount, qsizetype chunkCount);
    void resetRuntime();
    [[nodiscard]] QString nextReferenceId();
    [[nodiscard]] QString nextTaskId();
    [[nodiscard]] std::shared_ptr<Runtime> ensureRuntime(QString* errorMessage);
    void requestActiveTaskStop();
    void clearActiveTask(const QString& taskId);
    [[nodiscard]] bool isActiveTask(const QString& taskId, const QString& referenceId) const;
    [[nodiscard]] bool referenceExists(const QString& referenceId) const;

    mutable std::mutex mutex_;
    std::shared_ptr<Runtime> runtime_;
    QVariantList references_;
    QString sessionId_;
    QString activeTaskId_;
    QString activeReferenceId_;
    QString status_;
    QString errorMessage_;
    std::shared_ptr<std::stop_source> activeStopSource_;
    std::vector<std::jthread> workers_;
    int nextReferenceNumber_ = 1;
    int nextTaskNumber_ = 1;
    bool busy_ = false;
};

#endif // REARK_AGENT_KNOWLEDGE_CONTROLLER_H
