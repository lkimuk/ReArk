#include "controller/AgentSettings.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QVector>

#ifdef REARK_HAS_WUWE
#include <wuwe/agent/llm/llm_provider_registry.h>
#endif

#include <optional>
#include <utility>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <wincrypt.h>
#endif

namespace {

constexpr auto kAgentBaseUrlKey = "Agent/BaseUrl";
constexpr auto kAgentProviderKey = "Agent/Provider";
constexpr auto kAgentApiKeyKey = "Agent/ApiKey";
constexpr auto kAgentProtectedApiKeyKey = "Agent/ApiKeyProtected";
constexpr auto kAgentModelKey = "Agent/Model";
constexpr auto kAgentRequireApiKeyKey = "Agent/RequireApiKey";
constexpr auto kAgentEmbeddingBaseUrlKey = "Agent/EmbeddingBaseUrl";
constexpr auto kAgentEmbeddingApiKeyKey = "Agent/EmbeddingApiKey";
constexpr auto kAgentProtectedEmbeddingApiKeyKey = "Agent/EmbeddingApiKeyProtected";
constexpr auto kAgentEmbeddingModelKey = "Agent/EmbeddingModel";
constexpr auto kAgentEmbeddingRequireApiKeyKey = "Agent/EmbeddingRequireApiKey";
constexpr auto kDefaultBaseUrl = "https://openrouter.ai/api";
constexpr auto kDefaultProvider = "OpenRouter";
constexpr auto kDefaultModel = "openai/gpt-4o-mini";
constexpr auto kDefaultEmbeddingModel = "text-embedding-3-small";

struct ProviderInfo {
    QString id;
    QString displayName;
    QString defaultBaseUrl;
    QString defaultModel;
    QStringList apiKeyEnvNames;
    bool baseUrlRequired = false;
    bool apiKeyRequired = true;
    bool streaming = false;
    bool tools = false;
    bool localRuntime = false;
};

QString envString(const char* name)
{
    return QString::fromUtf8(qgetenv(name));
}

bool envBool(const char* name, bool fallback)
{
    const QByteArray raw = qgetenv(name).trimmed().toLower();
    if (raw.isEmpty()) {
        return fallback;
    }
    return raw == "1" || raw == "true" || raw == "yes" || raw == "on";
}

bool looksLocalEndpoint(const QString& baseUrl)
{
    return baseUrl.startsWith(QStringLiteral("http://127.0.0.1"))
        || baseUrl.startsWith(QStringLiteral("http://localhost"))
        || baseUrl.startsWith(QStringLiteral("https://localhost"));
}

QString toQString(const std::string& value)
{
    return QString::fromUtf8(value.data(), qsizetype(value.size()));
}

QStringList fallbackProviderKeys()
{
    return {
        QStringLiteral("OpenAI"),
        QStringLiteral("OpenAICompatible"),
        QStringLiteral("OpenRouter"),
        QStringLiteral("Anthropic"),
        QStringLiteral("Gemini"),
        QStringLiteral("Ollama"),
        QStringLiteral("DeepSeek"),
        QStringLiteral("DashScope"),
        QStringLiteral("Qwen"),
    };
}

QVector<ProviderInfo> fallbackProviders()
{
    QVector<ProviderInfo> providers;
    providers.reserve(fallbackProviderKeys().size());
    for (const QString& key : fallbackProviderKeys()) {
        ProviderInfo provider;
        provider.id = key;
        provider.displayName = key;
        provider.tools = true;
        provider.streaming = true;
        if (key == QStringLiteral("OpenAICompatible")) {
            provider.baseUrlRequired = true;
            provider.apiKeyRequired = false;
        } else if (key == QStringLiteral("Ollama")) {
            provider.defaultBaseUrl = QStringLiteral("http://127.0.0.1:11434");
            provider.defaultModel = QStringLiteral("qwen2.5-coder:7b");
            provider.apiKeyRequired = false;
            provider.localRuntime = true;
        } else if (key == QStringLiteral("OpenRouter")) {
            provider.defaultBaseUrl = QString::fromLatin1(kDefaultBaseUrl);
            provider.defaultModel = QString::fromLatin1(kDefaultModel);
            provider.apiKeyEnvNames = { QStringLiteral("OPENROUTER_API_KEY") };
        } else if (key == QStringLiteral("OpenAI")) {
            provider.defaultBaseUrl = QStringLiteral("https://api.openai.com");
            provider.defaultModel = QStringLiteral("gpt-4o-mini");
            provider.apiKeyEnvNames = { QStringLiteral("OPENAI_API_KEY") };
        } else if (key == QStringLiteral("Anthropic")) {
            provider.defaultBaseUrl = QStringLiteral("https://api.anthropic.com");
            provider.defaultModel = QStringLiteral("claude-3-5-sonnet-latest");
            provider.apiKeyEnvNames = { QStringLiteral("ANTHROPIC_API_KEY") };
        } else if (key == QStringLiteral("Gemini")) {
            provider.defaultBaseUrl = QStringLiteral("https://generativelanguage.googleapis.com");
            provider.defaultModel = QStringLiteral("gemini-1.5-pro");
            provider.apiKeyEnvNames = { QStringLiteral("GEMINI_API_KEY"), QStringLiteral("GOOGLE_API_KEY") };
        } else if (key == QStringLiteral("DeepSeek")) {
            provider.defaultBaseUrl = QStringLiteral("https://api.deepseek.com");
            provider.defaultModel = QStringLiteral("deepseek-chat");
            provider.apiKeyEnvNames = { QStringLiteral("DEEPSEEK_API_KEY") };
        } else if (key == QStringLiteral("DashScope")) {
            provider.defaultBaseUrl = QStringLiteral("https://dashscope.aliyuncs.com/compatible-mode/v1");
            provider.defaultModel = QStringLiteral("qwen-plus");
            provider.apiKeyEnvNames = { QStringLiteral("DASHSCOPE_API_KEY") };
        } else if (key == QStringLiteral("Qwen")) {
            provider.defaultBaseUrl = QStringLiteral("https://dashscope.aliyuncs.com/compatible-mode/v1");
            provider.defaultModel = QStringLiteral("qwen-plus");
            provider.apiKeyEnvNames = {
                QStringLiteral("QWEN_API_KEY"),
                QStringLiteral("DASHSCOPE_API_KEY"),
                QStringLiteral("OPENAI_API_KEY")
            };
        }
        providers.push_back(std::move(provider));
    }
    return providers;
}

QVector<ProviderInfo> providerInfos()
{
#ifdef REARK_HAS_WUWE
    QVector<ProviderInfo> providers;
    const auto& wuweProviders = wuwe::list_llm_providers();
    providers.reserve(static_cast<qsizetype>(wuweProviders.size()));
    for (const auto& wuweProvider : wuweProviders) {
        ProviderInfo provider;
        provider.id = toQString(wuweProvider.id);
        provider.displayName = toQString(wuweProvider.display_name);
        provider.defaultBaseUrl = toQString(wuweProvider.default_base_url);
        provider.baseUrlRequired = wuweProvider.base_url_required;
        provider.apiKeyRequired = wuweProvider.api_key_required;
        provider.streaming = wuweProvider.capabilities.streaming;
        provider.tools = wuweProvider.capabilities.tools;
        provider.localRuntime = wuweProvider.capabilities.local_runtime;
        provider.apiKeyEnvNames.reserve(static_cast<qsizetype>(wuweProvider.api_key_env_names.size()));
        for (const auto& name : wuweProvider.api_key_env_names) {
            provider.apiKeyEnvNames.append(toQString(name));
        }
        if (!wuweProvider.recommended_models.empty()) {
            provider.defaultModel = toQString(wuweProvider.recommended_models.front());
        }
        if (auto config = wuwe::make_default_llm_config(wuweProvider.id)) {
            if (provider.defaultBaseUrl.isEmpty()) {
                provider.defaultBaseUrl = toQString(config->base_url);
            }
            if (provider.defaultModel.isEmpty()) {
                provider.defaultModel = toQString(config->model);
            }
            provider.apiKeyRequired = config->require_api_key;
        }
        providers.push_back(std::move(provider));
    }
    if (!providers.isEmpty()) {
        return providers;
    }
#endif
    return fallbackProviders();
}

std::optional<ProviderInfo> providerInfo(QString provider)
{
    provider = provider.trimmed();
    if (provider.isEmpty()) {
        return std::nullopt;
    }
    for (const auto& info : providerInfos()) {
        if (provider.compare(info.id, Qt::CaseInsensitive) == 0) {
            return info;
        }
    }
    return std::nullopt;
}

QString normalizedProvider(QString provider)
{
    provider = provider.trimmed();
    if (const auto info = providerInfo(provider)) {
        return info->id;
    }
    return {};
}

QString envString(const QString& name)
{
    return QString::fromUtf8(qgetenv(name.toUtf8().constData()));
}

QString providerKeyPrefix(const QString& provider)
{
    const QString normalized = ::normalizedProvider(provider);
    const QString key = normalized.isEmpty() ? provider.trimmed() : normalized;
    return QStringLiteral("Agent/Providers/%1/").arg(QString::fromLatin1(QUrl::toPercentEncoding(key)));
}

QString providerValueKey(const QString& provider, const QString& name)
{
    return providerKeyPrefix(provider) + name;
}

QString providerApiKeyFromEnvironment(const ProviderInfo& provider)
{
    const QString configured = envString("REARK_LLM_API_KEY");
    if (!configured.isEmpty()) {
        return configured;
    }
    for (const QString& name : provider.apiKeyEnvNames) {
        const QString value = envString(name);
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

#ifdef Q_OS_WIN
QByteArray protectSecret(const QString& secret)
{
    const QByteArray plain = secret.toUtf8();
    DATA_BLOB input {
        .cbData = static_cast<DWORD>(plain.size()),
        .pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plain.constData()))
    };
    DATA_BLOB output {};

    if (!CryptProtectData(
            &input,
            L"ReArk Agent API Key",
            nullptr,
            nullptr,
            nullptr,
            0,
            &output)) {
        return {};
    }

    QByteArray protectedBytes(
        reinterpret_cast<const char*>(output.pbData),
        static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return protectedBytes.toBase64();
}

QString unprotectSecret(const QString& protectedSecret)
{
    const QByteArray protectedBytes = QByteArray::fromBase64(protectedSecret.toUtf8());
    if (protectedBytes.isEmpty()) {
        return {};
    }

    DATA_BLOB input {
        .cbData = static_cast<DWORD>(protectedBytes.size()),
        .pbData = reinterpret_cast<BYTE*>(const_cast<char*>(protectedBytes.constData()))
    };
    DATA_BLOB output {};

    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
        return {};
    }

    const QString secret = QString::fromUtf8(
        reinterpret_cast<const char*>(output.pbData),
        static_cast<qsizetype>(output.cbData));
    LocalFree(output.pbData);
    return secret;
}
#else
QByteArray protectSecret(const QString& secret)
{
    return secret.toUtf8().toBase64();
}

QString unprotectSecret(const QString& protectedSecret)
{
    return QString::fromUtf8(QByteArray::fromBase64(protectedSecret.toUtf8()));
}
#endif

QString loadProtectedKey(
    QSettings& settings,
    const QString& protectedKeyName,
    const QString& legacyKeyName,
    const QString& fallback)
{
    const QString protectedKey = settings.value(protectedKeyName).toString();
    if (!protectedKey.isEmpty()) {
        return unprotectSecret(protectedKey);
    }

    const QString legacyPlaintextKey = settings.value(legacyKeyName).toString();
    if (!legacyPlaintextKey.isEmpty()) {
        settings.remove(legacyKeyName);
        const QByteArray protectedLegacyKey = protectSecret(legacyPlaintextKey);
        if (!protectedLegacyKey.isEmpty()) {
            settings.setValue(protectedKeyName, QString::fromLatin1(protectedLegacyKey));
        }
        return legacyPlaintextKey;
    }

    return fallback;
}

QString loadProtectedKey(
    QSettings& settings,
    const char* protectedKeyName,
    const char* legacyKeyName,
    const QString& fallback)
{
    return loadProtectedKey(
        settings,
        QString::fromLatin1(protectedKeyName),
        QString::fromLatin1(legacyKeyName),
        fallback);
}

bool saveProtectedKey(
    QSettings& settings,
    const QString& protectedKeyName,
    const QString& legacyKeyName,
    const QString& key)
{
    settings.remove(legacyKeyName);
    settings.remove(protectedKeyName);
    if (key.isEmpty()) {
        return true;
    }

    const QByteArray protectedKey = protectSecret(key);
    if (protectedKey.isEmpty()) {
        return false;
    }

    settings.setValue(protectedKeyName, QString::fromLatin1(protectedKey));
    return true;
}

bool saveProtectedKey(
    QSettings& settings,
    const char* protectedKeyName,
    const char* legacyKeyName,
    const QString& key)
{
    return saveProtectedKey(
        settings,
        QString::fromLatin1(protectedKeyName),
        QString::fromLatin1(legacyKeyName),
        key);
}

bool hasProviderSettings(QSettings& settings, const QString& provider)
{
    const QString prefix = providerKeyPrefix(provider);
    return settings.contains(prefix + QStringLiteral("BaseUrl"))
        || settings.contains(prefix + QStringLiteral("Model"))
        || settings.contains(prefix + QStringLiteral("RequireApiKey"))
        || settings.contains(prefix + QStringLiteral("ApiKey"))
        || settings.contains(prefix + QStringLiteral("ApiKeyProtected"));
}

bool saveProviderSettings(
    QSettings& settings,
    const AgentSettings& agentSettings)
{
    const QString provider = normalizedProvider(agentSettings.provider);
    if (provider.isEmpty()) {
        return false;
    }

    settings.setValue(providerValueKey(provider, QStringLiteral("BaseUrl")), agentSettings.baseUrl.trimmed());
    settings.setValue(providerValueKey(provider, QStringLiteral("Model")), agentSettings.model.trimmed());
    settings.setValue(providerValueKey(provider, QStringLiteral("RequireApiKey")), agentSettings.requireApiKey);
    return saveProtectedKey(
        settings,
        providerValueKey(provider, QStringLiteral("ApiKeyProtected")),
        providerValueKey(provider, QStringLiteral("ApiKey")),
        agentSettings.apiKey);
}

AgentSettings loadProviderRuntimeSettings(QSettings& settings, const QString& providerId)
{
    AgentSettings result;
    result.provider = normalizedProvider(providerId);
    if (result.provider.isEmpty()) {
        result.provider = AgentSettingsStore::defaultProvider();
    }

    const auto provider = providerInfo(result.provider);
    const QString configuredBaseUrl = envString("REARK_LLM_BASE_URL");
    const QString configuredModel = envString("REARK_LLM_MODEL");
    const QString providerDefaultBaseUrl = !configuredBaseUrl.isEmpty()
        ? configuredBaseUrl
        : (provider ? provider->defaultBaseUrl : AgentSettingsStore::defaultBaseUrl());
    const QString providerDefaultModel = !configuredModel.isEmpty()
        ? configuredModel
        : (provider && !provider->defaultModel.isEmpty() ? provider->defaultModel : AgentSettingsStore::defaultModel());
    const QString fallbackApiKey = provider ? providerApiKeyFromEnvironment(*provider) : AgentSettingsStore::defaultApiKey();
    const bool fallbackRequireApiKey = envBool(
        "REARK_LLM_REQUIRE_API_KEY",
        provider ? provider->apiKeyRequired : AgentSettingsStore::defaultRequireApiKey(providerDefaultBaseUrl));

    const QString prefix = providerKeyPrefix(result.provider);
    result.baseUrl = settings.value(prefix + QStringLiteral("BaseUrl"), providerDefaultBaseUrl).toString().trimmed();
    result.model = settings.value(prefix + QStringLiteral("Model"), providerDefaultModel).toString().trimmed();
    result.requireApiKey = settings.value(prefix + QStringLiteral("RequireApiKey"), fallbackRequireApiKey).toBool();
    result.apiKey = loadProtectedKey(
        settings,
        prefix + QStringLiteral("ApiKeyProtected"),
        prefix + QStringLiteral("ApiKey"),
        fallbackApiKey);
    return result;
}

void migrateLegacyProviderSettings(QSettings& settings, const QString& providerId)
{
    const QString provider = normalizedProvider(providerId);
    if (provider.isEmpty() || hasProviderSettings(settings, provider)) {
        return;
    }

    const QString legacyProvider = normalizedProvider(
        settings.value(QString::fromLatin1(kAgentProviderKey), AgentSettingsStore::defaultProvider()).toString());
    if (!legacyProvider.isEmpty() && legacyProvider != provider) {
        return;
    }
    if (legacyProvider.isEmpty() && provider != AgentSettingsStore::defaultProvider()) {
        return;
    }

    const auto providerMeta = providerInfo(provider);
    AgentSettings legacy;
    legacy.provider = provider;
    legacy.baseUrl = settings.value(
        QString::fromLatin1(kAgentBaseUrlKey),
        providerMeta ? providerMeta->defaultBaseUrl : AgentSettingsStore::defaultBaseUrl()).toString().trimmed();
    legacy.model = settings.value(
        QString::fromLatin1(kAgentModelKey),
        providerMeta && !providerMeta->defaultModel.isEmpty()
            ? providerMeta->defaultModel
            : AgentSettingsStore::defaultModel()).toString().trimmed();
    legacy.requireApiKey = settings.value(
        QString::fromLatin1(kAgentRequireApiKeyKey),
        providerMeta ? providerMeta->apiKeyRequired : AgentSettingsStore::defaultRequireApiKey(legacy.baseUrl)).toBool();
    legacy.apiKey = loadProtectedKey(
        settings,
        kAgentProtectedApiKeyKey,
        kAgentApiKeyKey,
        providerMeta ? providerApiKeyFromEnvironment(*providerMeta) : AgentSettingsStore::defaultApiKey());
    saveProviderSettings(settings, legacy);
}

} // namespace

AgentSettings AgentSettingsStore::load()
{
    QSettings settings;
    const QString activeProvider = normalizedProvider(
        settings.value(QString::fromLatin1(kAgentProviderKey), defaultProvider()).toString());
    migrateLegacyProviderSettings(settings, activeProvider.isEmpty() ? defaultProvider() : activeProvider);

    AgentSettings result = loadProviderRuntimeSettings(
        settings,
        activeProvider.isEmpty() ? defaultProvider() : activeProvider);
    result.embeddingBaseUrl = settings.value(
        QString::fromLatin1(kAgentEmbeddingBaseUrlKey),
        defaultEmbeddingBaseUrl()).toString().trimmed();
    result.embeddingApiKey = loadProtectedKey(
        settings,
        kAgentProtectedEmbeddingApiKeyKey,
        kAgentEmbeddingApiKeyKey,
        defaultEmbeddingApiKey());
    result.embeddingModel = settings.value(
        QString::fromLatin1(kAgentEmbeddingModelKey),
        defaultEmbeddingModel()).toString().trimmed();
    result.embeddingRequireApiKey = settings.value(
        QString::fromLatin1(kAgentEmbeddingRequireApiKeyKey),
        envBool("REARK_EMBEDDING_REQUIRE_API_KEY", defaultEmbeddingRequireApiKey(result.embeddingBaseUrl))).toBool();
    return result;
}

bool AgentSettingsStore::save(const AgentSettings& settings)
{
    QSettings qsettings;
    const QString provider = normalizedProvider(settings.provider).isEmpty()
            ? defaultProvider()
            : normalizedProvider(settings.provider);
    qsettings.setValue(QString::fromLatin1(kAgentProviderKey), provider);
    qsettings.setValue(QString::fromLatin1(kAgentBaseUrlKey), settings.baseUrl.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentModelKey), settings.model.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentRequireApiKeyKey), settings.requireApiKey);
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingBaseUrlKey), settings.embeddingBaseUrl.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingModelKey), settings.embeddingModel.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingRequireApiKeyKey), settings.embeddingRequireApiKey);

    AgentSettings providerSettings = settings;
    providerSettings.provider = provider;
    return saveProviderSettings(qsettings, providerSettings)
        && saveProtectedKey(qsettings, kAgentProtectedApiKeyKey, kAgentApiKeyKey, settings.apiKey)
        && saveProtectedKey(
            qsettings,
            kAgentProtectedEmbeddingApiKeyKey,
            kAgentEmbeddingApiKeyKey,
            settings.embeddingApiKey);
}

void AgentSettingsStore::reset()
{
    resetRuntimeSettings();
    resetKnowledgeSettings();
}

void AgentSettingsStore::resetRuntimeSettings()
{
    QSettings settings;
    settings.remove(QString::fromLatin1(kAgentProviderKey));
    settings.remove(QString::fromLatin1(kAgentBaseUrlKey));
    settings.remove(QString::fromLatin1(kAgentApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentProtectedApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentModelKey));
    settings.remove(QString::fromLatin1(kAgentRequireApiKeyKey));
    settings.remove(QStringLiteral("Agent/Providers"));
}

void AgentSettingsStore::resetKnowledgeSettings()
{
    QSettings settings;
    settings.remove(QString::fromLatin1(kAgentEmbeddingBaseUrlKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentProtectedEmbeddingApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingModelKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingRequireApiKeyKey));
}

QString AgentSettingsStore::validationMessage(const AgentSettings& settings)
{
    const QString baseUrl = settings.baseUrl.trimmed();
    const auto provider = providerInfo(settings.provider);
    if (!provider) {
        return QCoreApplication::translate("AgentSettings", "Provider is not supported.");
    }

    if (provider->baseUrlRequired && baseUrl.isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Base URL is required.");
    }

    if (!baseUrl.isEmpty()) {
        const QUrl url(baseUrl);
        if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()
            || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
            return QCoreApplication::translate("AgentSettings", "Base URL must be a valid HTTP or HTTPS endpoint.");
        }
    }

    if (settings.model.trimmed().isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Model is required.");
    }

    if (settings.requireApiKey && settings.apiKey.isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "API key is required for this endpoint.");
    }

    return {};
}

QString AgentSettingsStore::knowledgeValidationMessage(const AgentSettings& settings)
{
    const QString baseUrl = settings.embeddingBaseUrl.trimmed();
    if (baseUrl.isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Embedding Base URL is required before adding reference knowledge.");
    }

    const QUrl url(baseUrl);
    if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return QCoreApplication::translate("AgentSettings", "Embedding Base URL must be a valid HTTP or HTTPS endpoint.");
    }

    if (settings.embeddingModel.trimmed().isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Embedding model is required before adding reference knowledge.");
    }

    if (settings.embeddingRequireApiKey && settings.embeddingApiKey.isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Embedding API key is required for this endpoint.");
    }

    return {};
}

QString AgentSettingsStore::normalizedProvider(const QString& provider)
{
    return ::normalizedProvider(provider);
}

QVariantList AgentSettingsStore::availableProviders()
{
    QVariantList result;
    for (const auto& provider : providerInfos()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), provider.id);
        item.insert(QStringLiteral("displayName"), provider.displayName.isEmpty() ? provider.id : provider.displayName);
        item.insert(QStringLiteral("defaultBaseUrl"), provider.defaultBaseUrl);
        item.insert(QStringLiteral("defaultModel"), provider.defaultModel);
        item.insert(QStringLiteral("baseUrlRequired"), provider.baseUrlRequired);
        item.insert(QStringLiteral("apiKeyRequired"), provider.apiKeyRequired);
        item.insert(QStringLiteral("apiKeyEnvNames"), provider.apiKeyEnvNames.join(QStringLiteral(", ")));
        item.insert(QStringLiteral("streaming"), provider.streaming);
        item.insert(QStringLiteral("tools"), provider.tools);
        item.insert(QStringLiteral("localRuntime"), provider.localRuntime);
        result.append(item);
    }
    return result;
}

QVariantMap AgentSettingsStore::providerDefaults(const QString& provider)
{
    QVariantMap result;
    const auto info = providerInfo(provider);
    if (!info) {
        return result;
    }

    result.insert(QStringLiteral("id"), info->id);
    result.insert(QStringLiteral("displayName"), info->displayName.isEmpty() ? info->id : info->displayName);
    result.insert(QStringLiteral("baseUrl"), info->defaultBaseUrl);
    result.insert(QStringLiteral("model"), info->defaultModel.isEmpty() ? defaultModel() : info->defaultModel);
    result.insert(QStringLiteral("baseUrlRequired"), info->baseUrlRequired);
    result.insert(QStringLiteral("apiKeyRequired"), info->apiKeyRequired);
    result.insert(QStringLiteral("apiKeyEnvNames"), info->apiKeyEnvNames.join(QStringLiteral(", ")));
    result.insert(QStringLiteral("streaming"), info->streaming);
    result.insert(QStringLiteral("tools"), info->tools);
    result.insert(QStringLiteral("localRuntime"), info->localRuntime);
    return result;
}

QVariantMap AgentSettingsStore::providerSettings(const QString& provider)
{
    QSettings settings;
    const QString normalized = normalizedProvider(provider);
    if (normalized.isEmpty()) {
        return {};
    }
    migrateLegacyProviderSettings(settings, normalized);
    const AgentSettings runtime = loadProviderRuntimeSettings(settings, normalized);
    QVariantMap result = providerDefaults(normalized);
    result.insert(QStringLiteral("baseUrl"), runtime.baseUrl);
    result.insert(QStringLiteral("model"), runtime.model);
    result.insert(QStringLiteral("apiKey"), runtime.apiKey);
    result.insert(QStringLiteral("apiKeyRequired"), runtime.requireApiKey);
    return result;
}

QString AgentSettingsStore::defaultProvider()
{
    const QString configured = normalizedProvider(envString("REARK_LLM_PROVIDER"));
    if (!configured.isEmpty()) {
        return configured;
    }
    if (const QString preferred = normalizedProvider(QString::fromLatin1(kDefaultProvider)); !preferred.isEmpty()) {
        return preferred;
    }
    const auto providers = providerInfos();
    return providers.isEmpty() ? QString::fromLatin1(kDefaultProvider) : providers.front().id;
}

QString AgentSettingsStore::defaultBaseUrl()
{
    const QString configured = envString("REARK_LLM_BASE_URL");
    if (!configured.isEmpty()) {
        return configured;
    }
    const auto provider = providerInfo(defaultProvider());
    if (provider && !provider->defaultBaseUrl.isEmpty()) {
        return provider->defaultBaseUrl;
    }
    return QString::fromLatin1(kDefaultBaseUrl);
}

QString AgentSettingsStore::defaultApiKey()
{
    const auto provider = providerInfo(defaultProvider());
    if (provider) {
        return providerApiKeyFromEnvironment(*provider);
    }
    const QString configured = envString("REARK_LLM_API_KEY");
    return configured.isEmpty() ? envString("OPENROUTER_API_KEY") : configured;
}

QString AgentSettingsStore::defaultModel()
{
    const QString configured = envString("REARK_LLM_MODEL");
    if (!configured.isEmpty()) {
        return configured;
    }
    const auto provider = providerInfo(defaultProvider());
    if (provider && !provider->defaultModel.isEmpty()) {
        return provider->defaultModel;
    }
    return QString::fromLatin1(kDefaultModel);
}

bool AgentSettingsStore::defaultRequireApiKey(const QString& baseUrl)
{
    return !looksLocalEndpoint(baseUrl.isEmpty() ? defaultBaseUrl() : baseUrl.trimmed());
}

QString AgentSettingsStore::defaultEmbeddingBaseUrl()
{
    const QString configured = envString("REARK_EMBEDDING_BASE_URL");
    return configured.isEmpty() ? QString::fromLatin1("https://api.openai.com") : configured;
}

QString AgentSettingsStore::defaultEmbeddingApiKey()
{
    const QString configured = envString("REARK_EMBEDDING_API_KEY");
    if (!configured.isEmpty()) {
        return configured;
    }
    return defaultApiKey();
}

QString AgentSettingsStore::defaultEmbeddingModel()
{
    const QString configured = envString("REARK_EMBEDDING_MODEL");
    return configured.isEmpty() ? QString::fromLatin1(kDefaultEmbeddingModel) : configured;
}

bool AgentSettingsStore::defaultEmbeddingRequireApiKey(const QString& baseUrl)
{
    return !looksLocalEndpoint(baseUrl.isEmpty() ? defaultEmbeddingBaseUrl() : baseUrl.trimmed());
}
