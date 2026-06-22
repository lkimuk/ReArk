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

private:
    struct Runtime;

    void setRunning(bool running);
    void setTranscript(const QString& transcript);
    void clearMessages();
    void appendMessage(const QString& role, const QString& text, const QString& state = {});
    void queueAssistantDelta(const QString& text);
    void flushPendingAssistantDelta();
    void appendToActiveAssistantMessage(const QString& text);
    void recordActiveAssistantActivity(
        const QString& type,
        const QString& title,
        const QString& detail = {},
        const QString& state = {});
    void finishActiveAssistantMessage(const QString& fallbackText = {});
    void finishInterruptedAssistantMessage(const QString& notice);
    void failActiveAssistantMessage();
    void rebuildTranscript();
    void appendTranscript(const QString& text);
    void setErrorMessage(const QString& errorMessage);
    void setStatus(const QString& status);
    void resetRun();
    void cancelCurrentRun(bool clearPendingQuestion);
    void startPendingQuestion();
    [[nodiscard]] QString unavailableMessage() const;

    DecompilerController* decompilerController_ = nullptr;
    AgentKnowledgeController* knowledgeController_ = nullptr;
    std::unique_ptr<Runtime> runtime_;
    QString transcript_;
    QVariantList messages_;
    AgentMessageModel* messageModel_ = nullptr;
    QString errorMessage_;
    QString status_;
    QString pendingAssistantDelta_;
    QTimer* assistantDeltaTimer_ = nullptr;
    int activeAssistantMessage_ = -1;
    QString pendingQuestion_;
    bool running_ = false;
};

#endif // REARK_AGENT_CONTROLLER_H
