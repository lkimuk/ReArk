#ifndef REARK_AGENT_CONTROLLER_H
#define REARK_AGENT_CONTROLLER_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>
#include <optional>
#include <stop_token>

class AgentMessageModel;
class DecompilerController;
class AgentKnowledgeController;
class QTimer;

class AgentController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString transcript READ transcript NOTIFY transcriptChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QAbstractItemModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(bool hasMessages READ hasMessages NOTIFY messagesChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool hasReasoningDetails READ hasReasoningDetails NOTIFY reasoningDetailsChanged)
    Q_PROPERTY(QString reasoningResultJson READ reasoningResultJson NOTIFY reasoningDetailsChanged)
    Q_PROPERTY(QString reasoningTraceJson READ reasoningTraceJson NOTIFY reasoningDetailsChanged)
    Q_PROPERTY(QString reasoningUsageJson READ reasoningUsageJson NOTIFY reasoningDetailsChanged)

public:
    explicit AgentController(
        DecompilerController* decompilerController,
        AgentKnowledgeController* knowledgeController,
        QObject* parent = nullptr);
    ~AgentController() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool running() const;
    [[nodiscard]] QString transcript() const;
    [[nodiscard]] QVariantList messages() const;
    [[nodiscard]] QAbstractItemModel* messageModel() const;
    [[nodiscard]] bool hasMessages() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] QString status() const;
    [[nodiscard]] bool hasReasoningDetails() const;
    [[nodiscard]] QString reasoningResultJson() const;
    [[nodiscard]] QString reasoningTraceJson() const;
    [[nodiscard]] QString reasoningUsageJson() const;

    Q_INVOKABLE void ask(const QString& question);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void newChat();
    Q_INVOKABLE void copyTextToClipboard(const QString& text) const;

signals:
    void runningChanged();
    void transcriptChanged();
    void messagesChanged();
    void errorMessageChanged();
    void statusChanged();
    void reasoningDetailsChanged();

private:
    struct Runtime;

    void setRunning(bool running);
    void setTranscript(const QString& transcript);
    void clearMessages();
    void appendMessage(const QString& role, const QString& text, const QString& state = {});
    void queueAssistantDelta(const QString& text);
    void flushPendingAssistantDelta();
    void appendToActiveAssistantMessage(const QString& text);
    void finishActiveAssistantMessage(const QString& fallbackText = {});
    void failActiveAssistantMessage();
    void rebuildTranscript();
    void appendTranscript(const QString& text);
    void setErrorMessage(const QString& errorMessage);
    void setStatus(const QString& status);
    void clearReasoningDetails();
    void setReasoningDetails(const QString& resultJson, const QString& traceJson, const QString& usageJson);
    void resetRun();
    [[nodiscard]] QString unavailableMessage() const;

    DecompilerController* decompilerController_ = nullptr;
    AgentKnowledgeController* knowledgeController_ = nullptr;
    std::unique_ptr<Runtime> runtime_;
    QString transcript_;
    QVariantList messages_;
    AgentMessageModel* messageModel_ = nullptr;
    QString errorMessage_;
    QString status_;
    QString reasoningResultJson_;
    QString reasoningTraceJson_;
    QString reasoningUsageJson_;
    QString pendingAssistantDelta_;
    QTimer* assistantDeltaTimer_ = nullptr;
    int activeAssistantMessage_ = -1;
    bool running_ = false;
};

#endif // REARK_AGENT_CONTROLLER_H
