#ifndef REARK_SETTINGS_CONTROLLER_H
#define REARK_SETTINGS_CONTROLLER_H

#include <QObject>
#include <QString>

#include "controller/AgentSettings.h"

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString agentBaseUrl READ agentBaseUrl WRITE setAgentBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentApiKey READ agentApiKey WRITE setAgentApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentModel READ agentModel WRITE setAgentModel NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentRequireApiKey READ agentRequireApiKey WRITE setAgentRequireApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingBaseUrl READ agentEmbeddingBaseUrl WRITE setAgentEmbeddingBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingApiKey READ agentEmbeddingApiKey WRITE setAgentEmbeddingApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingModel READ agentEmbeddingModel WRITE setAgentEmbeddingModel NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentEmbeddingRequireApiKey READ agentEmbeddingRequireApiKey WRITE setAgentEmbeddingRequireApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentTikaUrl READ agentTikaUrl WRITE setAgentTikaUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentValidationMessage READ agentValidationMessage NOTIFY agentValidationChanged)

public:
    explicit SettingsController(QObject* parent = nullptr);

    [[nodiscard]] QString agentBaseUrl() const;
    void setAgentBaseUrl(const QString& agentBaseUrl);

    [[nodiscard]] QString agentApiKey() const;
    void setAgentApiKey(const QString& agentApiKey);

    [[nodiscard]] QString agentModel() const;
    void setAgentModel(const QString& agentModel);

    [[nodiscard]] bool agentRequireApiKey() const;
    void setAgentRequireApiKey(bool agentRequireApiKey);

    [[nodiscard]] QString agentEmbeddingBaseUrl() const;
    void setAgentEmbeddingBaseUrl(const QString& agentEmbeddingBaseUrl);

    [[nodiscard]] QString agentEmbeddingApiKey() const;
    void setAgentEmbeddingApiKey(const QString& agentEmbeddingApiKey);

    [[nodiscard]] QString agentEmbeddingModel() const;
    void setAgentEmbeddingModel(const QString& agentEmbeddingModel);

    [[nodiscard]] bool agentEmbeddingRequireApiKey() const;
    void setAgentEmbeddingRequireApiKey(bool agentEmbeddingRequireApiKey);

    [[nodiscard]] QString agentTikaUrl() const;
    void setAgentTikaUrl(const QString& agentTikaUrl);

    [[nodiscard]] QString agentValidationMessage() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool saveAgentSettings(
        const QString& baseUrl,
        const QString& apiKey,
        const QString& model,
        bool requireApiKey,
        const QString& embeddingBaseUrl,
        const QString& embeddingApiKey,
        const QString& embeddingModel,
        bool embeddingRequireApiKey,
        const QString& tikaUrl);
    Q_INVOKABLE void resetAgentSettings();

signals:
    void agentSettingsChanged();
    void agentKnowledgeSettingsChanged();
    void agentValidationChanged();

private:
    void loadAgentSettings();
    void setAgentSettings(const AgentSettings& settings);
    void setAgentValidationMessage(const QString& message);

    QString agentBaseUrl_;
    QString agentApiKey_;
    QString agentModel_;
    QString agentEmbeddingBaseUrl_;
    QString agentEmbeddingApiKey_;
    QString agentEmbeddingModel_;
    QString agentTikaUrl_;
    QString agentValidationMessage_;
    bool agentRequireApiKey_ = true;
    bool agentEmbeddingRequireApiKey_ = true;
};

#endif // REARK_SETTINGS_CONTROLLER_H
