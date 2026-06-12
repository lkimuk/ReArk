#ifndef REARK_AGENT_MESSAGE_MODEL_H
#define REARK_AGENT_MESSAGE_MODEL_H

#include <QAbstractListModel>
#include <QString>

class AgentMessageModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum MessageRole {
        MessageRoleRole = Qt::UserRole + 1,
        MessageTextRole,
        MessageStateRole,
        MessageTimeRole
    };
    Q_ENUM(MessageRole)

    explicit AgentMessageModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    int appendMessage(const QString& role, const QString& text, const QString& state, const QString& time);
    void clear();
    void removeMessage(int row);
    void appendText(int row, const QString& text);
    void finishStreaming(int row, const QString& fallbackText);
    void failStreaming(int row);

private:
    struct Message {
        QString role;
        QString text;
        QString state;
        QString time;
    };
    QList<Message> messages_;
};

#endif // REARK_AGENT_MESSAGE_MODEL_H
