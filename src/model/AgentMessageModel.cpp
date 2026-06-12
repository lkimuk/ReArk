#include "model/AgentMessageModel.h"

AgentMessageModel::AgentMessageModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int AgentMessageModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return messages_.size();
}

QVariant AgentMessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= messages_.size()) {
        return {};
    }

    const Message& message = messages_.at(index.row());
    switch (role) {
    case MessageRoleRole:
        return message.role;
    case MessageTextRole:
    case Qt::DisplayRole:
        return message.text;
    case MessageStateRole:
        return message.state;
    case MessageTimeRole:
        return message.time;
    default:
        return {};
    }
}

QHash<int, QByteArray> AgentMessageModel::roleNames() const
{
    return {
        { MessageRoleRole, "messageRole" },
        { MessageTextRole, "messageText" },
        { MessageStateRole, "messageState" },
        { MessageTimeRole, "messageTime" }
    };
}

int AgentMessageModel::appendMessage(
    const QString& role,
    const QString& text,
    const QString& state,
    const QString& time)
{
    const int row = messages_.size();
    beginInsertRows({}, row, row);
    messages_.append(Message {
        .role = role,
        .text = text,
        .state = state,
        .time = time
    });
    endInsertRows();
    return row;
}

void AgentMessageModel::clear()
{
    if (messages_.isEmpty()) {
        return;
    }

    beginResetModel();
    messages_.clear();
    endResetModel();
}

void AgentMessageModel::removeMessage(int row)
{
    if (row < 0 || row >= messages_.size()) {
        return;
    }

    beginRemoveRows({}, row, row);
    messages_.removeAt(row);
    endRemoveRows();
}

void AgentMessageModel::appendText(int row, const QString& text)
{
    if (text.isEmpty() || row < 0 || row >= messages_.size()) {
        return;
    }

    messages_[row].text += text;
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed, { MessageTextRole, Qt::DisplayRole });
}

void AgentMessageModel::finishStreaming(int row, const QString& fallbackText)
{
    if (row < 0 || row >= messages_.size()) {
        return;
    }

    Message& message = messages_[row];
    if (message.state != QStringLiteral("streaming")) {
        return;
    }

    if (!fallbackText.isEmpty() && message.text.trimmed().isEmpty()) {
        message.text = fallbackText;
    }
    message.state = QStringLiteral("done");

    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed, { MessageTextRole, MessageStateRole, Qt::DisplayRole });
}

void AgentMessageModel::failStreaming(int row)
{
    if (row < 0 || row >= messages_.size()) {
        return;
    }

    Message& message = messages_[row];
    if (message.state != QStringLiteral("streaming")) {
        return;
    }

    message.state = QStringLiteral("error");
    const QModelIndex changed = index(row);
    emit dataChanged(changed, changed, { MessageStateRole });
}
