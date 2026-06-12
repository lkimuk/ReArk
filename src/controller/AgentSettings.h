#ifndef REARK_AGENT_SETTINGS_H
#define REARK_AGENT_SETTINGS_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct AgentSettings {
    QString provider;
    QString baseUrl;
    QString apiKey;
    QString model;
    bool requireApiKey = true;
    QString embeddingBaseUrl;
    QString embeddingApiKey;
    QString embeddingModel;
    bool embeddingRequireApiKey = true;
};

class AgentSettingsStore {
public:
    [[nodiscard]] static AgentSettings load();
    [[nodiscard]] static bool save(const AgentSettings& settings);
    static void reset();
    static void resetRuntimeSettings();
    static void resetKnowledgeSettings();

    [[nodiscard]] static QString validationMessage(const AgentSettings& settings);
    [[nodiscard]] static QString knowledgeValidationMessage(const AgentSettings& settings);
    [[nodiscard]] static QString normalizedProvider(const QString& provider);
    [[nodiscard]] static QVariantList availableProviders();
    [[nodiscard]] static QVariantMap providerDefaults(const QString& provider);
    [[nodiscard]] static QVariantMap providerSettings(const QString& provider);
    [[nodiscard]] static QString defaultProvider();
    [[nodiscard]] static QString defaultBaseUrl();
    [[nodiscard]] static QString defaultApiKey();
    [[nodiscard]] static QString defaultModel();
    [[nodiscard]] static bool defaultRequireApiKey(const QString& baseUrl);
    [[nodiscard]] static QString defaultEmbeddingBaseUrl();
    [[nodiscard]] static QString defaultEmbeddingApiKey();
    [[nodiscard]] static QString defaultEmbeddingModel();
    [[nodiscard]] static bool defaultEmbeddingRequireApiKey(const QString& baseUrl);
};

#endif // REARK_AGENT_SETTINGS_H
