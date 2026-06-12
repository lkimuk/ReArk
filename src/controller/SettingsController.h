#ifndef REARK_SETTINGS_CONTROLLER_H
#define REARK_SETTINGS_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "controller/AgentSettings.h"

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString agentProvider READ agentProvider WRITE setAgentProvider NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentBaseUrl READ agentBaseUrl WRITE setAgentBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentApiKey READ agentApiKey WRITE setAgentApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentModel READ agentModel WRITE setAgentModel NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentRequireApiKey READ agentRequireApiKey WRITE setAgentRequireApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingBaseUrl READ agentEmbeddingBaseUrl WRITE setAgentEmbeddingBaseUrl NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingApiKey READ agentEmbeddingApiKey WRITE setAgentEmbeddingApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentEmbeddingModel READ agentEmbeddingModel WRITE setAgentEmbeddingModel NOTIFY agentSettingsChanged)
    Q_PROPERTY(bool agentEmbeddingRequireApiKey READ agentEmbeddingRequireApiKey WRITE setAgentEmbeddingRequireApiKey NOTIFY agentSettingsChanged)
    Q_PROPERTY(QString agentValidationMessage READ agentValidationMessage NOTIFY agentValidationChanged)
    Q_PROPERTY(QVariantList agentProviders READ agentProviders NOTIFY agentProvidersChanged)

public:
    explicit SettingsController(QObject* parent = nullptr);

    [[nodiscard]] QString agentProvider() const;
    void setAgentProvider(const QString& agentProvider);

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

    [[nodiscard]] QString agentValidationMessage() const;
    [[nodiscard]] QVariantList agentProviders() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QVariantMap agentProviderDefaults(const QString& provider) const;
    Q_INVOKABLE QVariantMap agentProviderSettings(const QString& provider) const;
    Q_INVOKABLE bool saveAgentSettings(
        const QString& provider,
        const QString& baseUrl,
        const QString& apiKey,
        const QString& model,
        bool requireApiKey,
        const QString& embeddingBaseUrl,
        const QString& embeddingApiKey,
        const QString& embeddingModel,
        bool embeddingRequireApiKey);
    Q_INVOKABLE void resetAgentSettings();
    Q_INVOKABLE void resetAgentRuntimeSettings();
    Q_INVOKABLE void resetKnowledgeSettings();

signals:
    void agentSettingsChanged();
    void agentKnowledgeSettingsChanged();
    void agentValidationChanged();
    void agentProvidersChanged();

private:
    void loadAgentSettings();
    void setAgentSettings(const AgentSettings& settings);
    void setAgentValidationMessage(const QString& message);

    QString agentProvider_;
    QString agentBaseUrl_;
    QString agentApiKey_;
    QString agentModel_;
    QString agentEmbeddingBaseUrl_;
    QString agentEmbeddingApiKey_;
    QString agentEmbeddingModel_;
    QString agentValidationMessage_;
    bool agentRequireApiKey_ = true;
    bool agentEmbeddingRequireApiKey_ = true;
};

#endif // REARK_SETTINGS_CONTROLLER_H
