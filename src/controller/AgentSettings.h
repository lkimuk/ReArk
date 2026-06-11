#ifndef REARK_AGENT_SETTINGS_H
#define REARK_AGENT_SETTINGS_H

#include <QString>

struct AgentSettings {
    QString baseUrl;
    QString apiKey;
    QString model;
    bool requireApiKey = true;
    QString embeddingBaseUrl;
    QString embeddingApiKey;
    QString embeddingModel;
    bool embeddingRequireApiKey = true;
    QString tikaUrl;
};

class AgentSettingsStore {
public:
    [[nodiscard]] static AgentSettings load();
    [[nodiscard]] static bool save(const AgentSettings& settings);
    static void reset();

    [[nodiscard]] static QString validationMessage(const AgentSettings& settings);
    [[nodiscard]] static QString knowledgeValidationMessage(const AgentSettings& settings);
    [[nodiscard]] static QString defaultBaseUrl();
    [[nodiscard]] static QString defaultApiKey();
    [[nodiscard]] static QString defaultModel();
    [[nodiscard]] static bool defaultRequireApiKey(const QString& baseUrl);
    [[nodiscard]] static QString defaultEmbeddingBaseUrl();
    [[nodiscard]] static QString defaultEmbeddingApiKey();
    [[nodiscard]] static QString defaultEmbeddingModel();
    [[nodiscard]] static bool defaultEmbeddingRequireApiKey(const QString& baseUrl);
    [[nodiscard]] static QString defaultTikaUrl();
};

#endif // REARK_AGENT_SETTINGS_H
