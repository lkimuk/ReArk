#include "controller/AgentSettings.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QSettings>
#include <QUrl>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <wincrypt.h>
#endif

namespace {

constexpr auto kAgentBaseUrlKey = "Agent/BaseUrl";
constexpr auto kAgentApiKeyKey = "Agent/ApiKey";
constexpr auto kAgentProtectedApiKeyKey = "Agent/ApiKeyProtected";
constexpr auto kAgentModelKey = "Agent/Model";
constexpr auto kAgentRequireApiKeyKey = "Agent/RequireApiKey";
constexpr auto kAgentEmbeddingBaseUrlKey = "Agent/EmbeddingBaseUrl";
constexpr auto kAgentEmbeddingApiKeyKey = "Agent/EmbeddingApiKey";
constexpr auto kAgentProtectedEmbeddingApiKeyKey = "Agent/EmbeddingApiKeyProtected";
constexpr auto kAgentEmbeddingModelKey = "Agent/EmbeddingModel";
constexpr auto kAgentEmbeddingRequireApiKeyKey = "Agent/EmbeddingRequireApiKey";
constexpr auto kAgentTikaUrlKey = "Agent/TikaUrl";
constexpr auto kDefaultBaseUrl = "https://openrouter.ai/api";
constexpr auto kDefaultModel = "openai/gpt-4o-mini";
constexpr auto kDefaultEmbeddingModel = "text-embedding-3-small";

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
    const char* protectedKeyName,
    const char* legacyKeyName,
    const QString& fallback)
{
    const QString protectedKey = settings.value(QString::fromLatin1(protectedKeyName)).toString();
    if (!protectedKey.isEmpty()) {
        return unprotectSecret(protectedKey);
    }

    const QString legacyPlaintextKey = settings.value(QString::fromLatin1(legacyKeyName)).toString();
    if (!legacyPlaintextKey.isEmpty()) {
        settings.remove(QString::fromLatin1(legacyKeyName));
        const QByteArray protectedLegacyKey = protectSecret(legacyPlaintextKey);
        if (!protectedLegacyKey.isEmpty()) {
            settings.setValue(QString::fromLatin1(protectedKeyName), QString::fromLatin1(protectedLegacyKey));
        }
        return legacyPlaintextKey;
    }

    return fallback;
}

bool saveProtectedKey(
    QSettings& settings,
    const char* protectedKeyName,
    const char* legacyKeyName,
    const QString& key)
{
    settings.remove(QString::fromLatin1(legacyKeyName));
    settings.remove(QString::fromLatin1(protectedKeyName));
    if (key.isEmpty()) {
        return true;
    }

    const QByteArray protectedKey = protectSecret(key);
    if (protectedKey.isEmpty()) {
        return false;
    }

    settings.setValue(QString::fromLatin1(protectedKeyName), QString::fromLatin1(protectedKey));
    return true;
}

} // namespace

AgentSettings AgentSettingsStore::load()
{
    QSettings settings;
    AgentSettings result;
    result.baseUrl = settings.value(QString::fromLatin1(kAgentBaseUrlKey), defaultBaseUrl()).toString().trimmed();
    result.apiKey = loadProtectedKey(
        settings,
        kAgentProtectedApiKeyKey,
        kAgentApiKeyKey,
        defaultApiKey());
    result.model = settings.value(QString::fromLatin1(kAgentModelKey), defaultModel()).toString().trimmed();
    result.requireApiKey = settings.value(
        QString::fromLatin1(kAgentRequireApiKeyKey),
        envBool("REARK_LLM_REQUIRE_API_KEY", defaultRequireApiKey(result.baseUrl))).toBool();
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
    result.tikaUrl = settings.value(QString::fromLatin1(kAgentTikaUrlKey), defaultTikaUrl()).toString().trimmed();
    return result;
}

bool AgentSettingsStore::save(const AgentSettings& settings)
{
    QSettings qsettings;
    qsettings.setValue(QString::fromLatin1(kAgentBaseUrlKey), settings.baseUrl.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentModelKey), settings.model.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentRequireApiKeyKey), settings.requireApiKey);
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingBaseUrlKey), settings.embeddingBaseUrl.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingModelKey), settings.embeddingModel.trimmed());
    qsettings.setValue(QString::fromLatin1(kAgentEmbeddingRequireApiKeyKey), settings.embeddingRequireApiKey);
    qsettings.setValue(QString::fromLatin1(kAgentTikaUrlKey), settings.tikaUrl.trimmed());

    return saveProtectedKey(qsettings, kAgentProtectedApiKeyKey, kAgentApiKeyKey, settings.apiKey)
        && saveProtectedKey(
            qsettings,
            kAgentProtectedEmbeddingApiKeyKey,
            kAgentEmbeddingApiKeyKey,
            settings.embeddingApiKey);
}

void AgentSettingsStore::reset()
{
    QSettings settings;
    settings.remove(QString::fromLatin1(kAgentBaseUrlKey));
    settings.remove(QString::fromLatin1(kAgentApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentProtectedApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentModelKey));
    settings.remove(QString::fromLatin1(kAgentRequireApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingBaseUrlKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentProtectedEmbeddingApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingModelKey));
    settings.remove(QString::fromLatin1(kAgentEmbeddingRequireApiKeyKey));
    settings.remove(QString::fromLatin1(kAgentTikaUrlKey));
}

QString AgentSettingsStore::validationMessage(const AgentSettings& settings)
{
    const QString baseUrl = settings.baseUrl.trimmed();
    if (baseUrl.isEmpty()) {
        return QCoreApplication::translate("AgentSettings", "Base URL is required.");
    }

    const QUrl url(baseUrl);
    if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return QCoreApplication::translate("AgentSettings", "Base URL must be a valid HTTP or HTTPS endpoint.");
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

    const QString tikaUrl = settings.tikaUrl.trimmed();
    if (!tikaUrl.isEmpty()) {
        const QUrl tika(tikaUrl);
        if (!tika.isValid() || tika.scheme().isEmpty() || tika.host().isEmpty()
            || (tika.scheme() != QStringLiteral("http") && tika.scheme() != QStringLiteral("https"))) {
            return QCoreApplication::translate("AgentSettings", "Tika URL must be a valid HTTP or HTTPS endpoint.");
        }
    }

    return {};
}

QString AgentSettingsStore::defaultBaseUrl()
{
    const QString configured = envString("REARK_LLM_BASE_URL");
    return configured.isEmpty() ? QString::fromLatin1(kDefaultBaseUrl) : configured;
}

QString AgentSettingsStore::defaultApiKey()
{
    const QString configured = envString("REARK_LLM_API_KEY");
    return configured.isEmpty() ? envString("OPENROUTER_API_KEY") : configured;
}

QString AgentSettingsStore::defaultModel()
{
    const QString configured = envString("REARK_LLM_MODEL");
    return configured.isEmpty() ? QString::fromLatin1(kDefaultModel) : configured;
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

QString AgentSettingsStore::defaultTikaUrl()
{
    return envString("REARK_TIKA_URL");
}
