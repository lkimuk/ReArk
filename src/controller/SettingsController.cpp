#include "controller/SettingsController.h"

SettingsController::SettingsController(QObject* parent)
    : QObject(parent)
{
    loadAgentSettings();
}

QString SettingsController::agentProvider() const
{
    return agentProvider_;
}

void SettingsController::setAgentProvider(const QString& agentProvider)
{
    const QString trimmed = agentProvider.trimmed();
    if (agentProvider_ == trimmed) {
        return;
    }

    agentProvider_ = trimmed;
    emit agentSettingsChanged();
}

QString SettingsController::agentBaseUrl() const
{
    return agentBaseUrl_;
}

void SettingsController::setAgentBaseUrl(const QString& agentBaseUrl)
{
    const QString trimmed = agentBaseUrl.trimmed();
    if (agentBaseUrl_ == trimmed) {
        return;
    }

    agentBaseUrl_ = trimmed;
    emit agentSettingsChanged();
}

QString SettingsController::agentApiKey() const
{
    return agentApiKey_;
}

void SettingsController::setAgentApiKey(const QString& agentApiKey)
{
    if (agentApiKey_ == agentApiKey) {
        return;
    }

    agentApiKey_ = agentApiKey;
    emit agentSettingsChanged();
}

QString SettingsController::agentModel() const
{
    return agentModel_;
}

void SettingsController::setAgentModel(const QString& agentModel)
{
    const QString trimmed = agentModel.trimmed();
    if (agentModel_ == trimmed) {
        return;
    }

    agentModel_ = trimmed;
    emit agentSettingsChanged();
}

bool SettingsController::agentRequireApiKey() const
{
    return agentRequireApiKey_;
}

void SettingsController::setAgentRequireApiKey(bool agentRequireApiKey)
{
    if (agentRequireApiKey_ == agentRequireApiKey) {
        return;
    }

    agentRequireApiKey_ = agentRequireApiKey;
    emit agentSettingsChanged();
}

QString SettingsController::agentEmbeddingBaseUrl() const
{
    return agentEmbeddingBaseUrl_;
}

void SettingsController::setAgentEmbeddingBaseUrl(const QString& agentEmbeddingBaseUrl)
{
    const QString trimmed = agentEmbeddingBaseUrl.trimmed();
    if (agentEmbeddingBaseUrl_ == trimmed) {
        return;
    }

    agentEmbeddingBaseUrl_ = trimmed;
    emit agentSettingsChanged();
}

QString SettingsController::agentEmbeddingApiKey() const
{
    return agentEmbeddingApiKey_;
}

void SettingsController::setAgentEmbeddingApiKey(const QString& agentEmbeddingApiKey)
{
    if (agentEmbeddingApiKey_ == agentEmbeddingApiKey) {
        return;
    }

    agentEmbeddingApiKey_ = agentEmbeddingApiKey;
    emit agentSettingsChanged();
}

QString SettingsController::agentEmbeddingModel() const
{
    return agentEmbeddingModel_;
}

void SettingsController::setAgentEmbeddingModel(const QString& agentEmbeddingModel)
{
    const QString trimmed = agentEmbeddingModel.trimmed();
    if (agentEmbeddingModel_ == trimmed) {
        return;
    }

    agentEmbeddingModel_ = trimmed;
    emit agentSettingsChanged();
}

bool SettingsController::agentEmbeddingRequireApiKey() const
{
    return agentEmbeddingRequireApiKey_;
}

void SettingsController::setAgentEmbeddingRequireApiKey(bool agentEmbeddingRequireApiKey)
{
    if (agentEmbeddingRequireApiKey_ == agentEmbeddingRequireApiKey) {
        return;
    }

    agentEmbeddingRequireApiKey_ = agentEmbeddingRequireApiKey;
    emit agentSettingsChanged();
}

QString SettingsController::agentValidationMessage() const
{
    return agentValidationMessage_;
}

QVariantList SettingsController::agentProviders() const
{
    return AgentSettingsStore::availableProviders();
}

void SettingsController::reload()
{
    const QString previousBaseUrl = agentBaseUrl_;
    const QString previousProvider = agentProvider_;
    const QString previousApiKey = agentApiKey_;
    const QString previousModel = agentModel_;
    const bool previousRequireApiKey = agentRequireApiKey_;
    const QString previousEmbeddingBaseUrl = agentEmbeddingBaseUrl_;
    const QString previousEmbeddingApiKey = agentEmbeddingApiKey_;
    const QString previousEmbeddingModel = agentEmbeddingModel_;
    const bool previousEmbeddingRequireApiKey = agentEmbeddingRequireApiKey_;

    loadAgentSettings();

    if (agentProvider_ != previousProvider
        || agentBaseUrl_ != previousBaseUrl
        || agentApiKey_ != previousApiKey
        || agentModel_ != previousModel
        || agentRequireApiKey_ != previousRequireApiKey
        || agentEmbeddingBaseUrl_ != previousEmbeddingBaseUrl
        || agentEmbeddingApiKey_ != previousEmbeddingApiKey
        || agentEmbeddingModel_ != previousEmbeddingModel
        || agentEmbeddingRequireApiKey_ != previousEmbeddingRequireApiKey) {
        emit agentSettingsChanged();
    }
    if (agentEmbeddingBaseUrl_ != previousEmbeddingBaseUrl
        || agentEmbeddingApiKey_ != previousEmbeddingApiKey
        || agentEmbeddingModel_ != previousEmbeddingModel
        || agentEmbeddingRequireApiKey_ != previousEmbeddingRequireApiKey) {
        emit agentKnowledgeSettingsChanged();
    }
}

QVariantMap SettingsController::agentProviderDefaults(const QString& provider) const
{
    return AgentSettingsStore::providerDefaults(provider);
}

QVariantMap SettingsController::agentProviderSettings(const QString& provider) const
{
    return AgentSettingsStore::providerSettings(provider);
}

bool SettingsController::saveAgentSettings(
    const QString& provider,
    const QString& baseUrl,
    const QString& apiKey,
    const QString& model,
    bool requireApiKey,
    const QString& embeddingBaseUrl,
    const QString& embeddingApiKey,
    const QString& embeddingModel,
    bool embeddingRequireApiKey)
{
    AgentSettings settings {
        .provider = provider.trimmed(),
        .baseUrl = baseUrl.trimmed(),
        .apiKey = apiKey,
        .model = model.trimmed(),
        .requireApiKey = requireApiKey,
        .embeddingBaseUrl = embeddingBaseUrl.trimmed(),
        .embeddingApiKey = embeddingApiKey,
        .embeddingModel = embeddingModel.trimmed(),
        .embeddingRequireApiKey = embeddingRequireApiKey
    };

    const QString validationMessage = AgentSettingsStore::validationMessage(settings);
    if (!validationMessage.isEmpty()) {
        setAgentValidationMessage(validationMessage);
        return false;
    }
    if (!AgentSettingsStore::save(settings)) {
        setAgentValidationMessage(tr("Failed to protect and save the API key."));
        return false;
    }

    setAgentSettings(settings);
    setAgentValidationMessage({});
    return true;
}

void SettingsController::resetAgentSettings()
{
    AgentSettingsStore::reset();
    reload();
}

void SettingsController::resetAgentRuntimeSettings()
{
    AgentSettingsStore::resetRuntimeSettings();
    reload();
}

void SettingsController::resetKnowledgeSettings()
{
    AgentSettingsStore::resetKnowledgeSettings();
    reload();
}

void SettingsController::loadAgentSettings()
{
    const AgentSettings settings = AgentSettingsStore::load();
    agentProvider_ = settings.provider;
    agentBaseUrl_ = settings.baseUrl;
    agentApiKey_ = settings.apiKey;
    agentModel_ = settings.model;
    agentRequireApiKey_ = settings.requireApiKey;
    agentEmbeddingBaseUrl_ = settings.embeddingBaseUrl;
    agentEmbeddingApiKey_ = settings.embeddingApiKey;
    agentEmbeddingModel_ = settings.embeddingModel;
    agentEmbeddingRequireApiKey_ = settings.embeddingRequireApiKey;
    setAgentValidationMessage({});
}

void SettingsController::setAgentSettings(const AgentSettings& settings)
{
    const bool changed = agentProvider_ != settings.provider
        || agentBaseUrl_ != settings.baseUrl
        || agentApiKey_ != settings.apiKey
        || agentModel_ != settings.model
        || agentRequireApiKey_ != settings.requireApiKey
        || agentEmbeddingBaseUrl_ != settings.embeddingBaseUrl
        || agentEmbeddingApiKey_ != settings.embeddingApiKey
        || agentEmbeddingModel_ != settings.embeddingModel
        || agentEmbeddingRequireApiKey_ != settings.embeddingRequireApiKey;
    const bool knowledgeChanged = agentEmbeddingBaseUrl_ != settings.embeddingBaseUrl
        || agentEmbeddingApiKey_ != settings.embeddingApiKey
        || agentEmbeddingModel_ != settings.embeddingModel
        || agentEmbeddingRequireApiKey_ != settings.embeddingRequireApiKey;

    agentProvider_ = settings.provider;
    agentBaseUrl_ = settings.baseUrl;
    agentApiKey_ = settings.apiKey;
    agentModel_ = settings.model;
    agentRequireApiKey_ = settings.requireApiKey;
    agentEmbeddingBaseUrl_ = settings.embeddingBaseUrl;
    agentEmbeddingApiKey_ = settings.embeddingApiKey;
    agentEmbeddingModel_ = settings.embeddingModel;
    agentEmbeddingRequireApiKey_ = settings.embeddingRequireApiKey;

    if (changed) {
        emit agentSettingsChanged();
    }
    if (knowledgeChanged) {
        emit agentKnowledgeSettingsChanged();
    }
}

void SettingsController::setAgentValidationMessage(const QString& message)
{
    if (agentValidationMessage_ == message) {
        return;
    }

    agentValidationMessage_ = message;
    emit agentValidationChanged();
}
