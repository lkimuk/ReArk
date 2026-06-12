#include "core/HyleDecompiler.h"

#include "core/PerformanceTrace.h"

#include <hyle.h>

#include <QFileInfo>
#include <QBuffer>
#include <QHash>
#include <QImage>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QStringList>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <optional>
#include <thread>
#include <system_error>
#include <utility>

namespace {

struct LinkedStopToken {
    std::stop_source source;
    std::stop_callback<std::function<void()>> sessionCallback;
    std::stop_callback<std::function<void()>> callerCallback;

    LinkedStopToken(std::stop_token sessionStopToken, std::stop_token callerStopToken)
        : sessionCallback(sessionStopToken, [this] {
            source.request_stop();
        })
        , callerCallback(callerStopToken, [this] {
            source.request_stop();
        })
    {
        if (sessionStopToken.stop_requested() || callerStopToken.stop_requested()) {
            source.request_stop();
        }
    }

    [[nodiscard]] std::stop_token token() const noexcept
    {
        return source.get_token();
    }
};

QString fromUtf8(const std::string& value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string toLower(std::string value)
{
    std::ranges::transform(value, value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

QString errorMessage(const char* context, const std::error_code& error)
{
    return QStringLiteral("%1: %2").arg(QString::fromLatin1(context), fromUtf8(error.message()));
}

std::string toUtf8Path(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

bool isAbcFile(const QString& filePath)
{
    return QFileInfo(filePath).suffix().compare(QStringLiteral("abc"), Qt::CaseInsensitive) == 0;
}

QString normalizeSourceContent(QString content);
QString languageKind(const std::string& language);

QString diagnosticsText(const std::vector<hyle::hap::diagnostic>& diagnostics)
{
    QString text;
    for (const auto& diagnostic : diagnostics) {
        if (!text.isEmpty()) {
            text += QLatin1Char('\n');
        }
        text += fromUtf8(diagnostic.source);
        text += QStringLiteral(": ");
        text += fromUtf8(diagnostic.message);
    }
    return text;
}

void applyDecompiledSource(
    HyleDecompiler::SourceResult& result,
    const hyle::hap::decompiled_source_file& source)
{
    result.content = normalizeSourceContent(fromUtf8(source.content));
    result.kind = languageKind(source.language);
    result.contentMode = QStringLiteral("text");
}

QString languageKind(const std::string& language)
{
    if (language.empty()) {
        return QStringLiteral("SRC");
    }
    return QString::fromStdString(toLower(language)).toUpper();
}

QString resourceKindName(hyle::hap::hap_resource_kind kind)
{
    using hyle::hap::hap_resource_kind;
    switch (kind) {
    case hap_resource_kind::directory:
        return QStringLiteral("DIR");
    case hap_resource_kind::abc:
        return QStringLiteral("ABC");
    case hap_resource_kind::json:
        return QStringLiteral("JSON");
    case hap_resource_kind::image:
        return QStringLiteral("IMAGE");
    case hap_resource_kind::media:
        return QStringLiteral("MEDIA");
    case hap_resource_kind::native_library:
        return QStringLiteral("LIB");
    case hap_resource_kind::signature:
        return QStringLiteral("SIGNATURE");
    case hap_resource_kind::resource_index:
        return QStringLiteral("RESOURCE_INDEX");
    case hap_resource_kind::text:
        return QStringLiteral("TXT");
    case hap_resource_kind::binary:
        return QStringLiteral("BIN");
    case hap_resource_kind::unknown:
        return QStringLiteral("UNKNOWN");
    }
    return QStringLiteral("UNKNOWN");
}

bool isTextResource(hyle::hap::hap_resource_kind kind)
{
    using hyle::hap::hap_resource_kind;
    return kind == hap_resource_kind::json
        || kind == hap_resource_kind::resource_index
        || kind == hap_resource_kind::signature
        || kind == hap_resource_kind::text;
}

bool isImageResource(hyle::hap::hap_resource_kind kind)
{
    return kind == hyle::hap::hap_resource_kind::image;
}

bool isImageMime(const QMimeType& mime)
{
    return mime.isValid() && mime.name().startsWith(QStringLiteral("image/"));
}

bool isMediaMime(const QMimeType& mime)
{
    return mime.isValid()
        && (mime.name().startsWith(QStringLiteral("video/"))
            || mime.name().startsWith(QStringLiteral("audio/")));
}

bool isImageResource(hyle::hap::hap_resource_kind kind, const std::string& path)
{
    if (isImageResource(kind)) {
        return true;
    }

    QMimeDatabase mimeDatabase;
    return isImageMime(mimeDatabase.mimeTypeForFile(fromUtf8(path), QMimeDatabase::MatchExtension));
}

QString bytesToUtf8Text(const std::vector<std::byte>& bytes)
{
    if (bytes.empty()) {
        return {};
    }
    return QString::fromUtf8(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<qsizetype>(bytes.size()));
}

QByteArray bytesToByteArray(const std::vector<std::byte>& bytes)
{
    if (bytes.empty()) {
        return {};
    }
    return QByteArray(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<qsizetype>(bytes.size()));
}

QImage imageFromResourceContent(const hyle::hap::hap_resource_content& content)
{
    QImage image;
    const QByteArray bytes = bytesToByteArray(content.bytes);
    image.loadFromData(bytes);
    return image;
}

void drawImagePreservingAspect(QPainter& painter, const QImage& image, const QRect& bounds)
{
    if (image.isNull() || bounds.isEmpty()) {
        return;
    }

    const QSize scaled = image.size().scaled(bounds.size(), Qt::KeepAspectRatio);
    const QPoint topLeft(
        bounds.x() + (bounds.width() - scaled.width()) / 2,
        bounds.y() + (bounds.height() - scaled.height()) / 2);
    painter.drawImage(QRect(topLeft, scaled), image);
}

QByteArray imageToPngBytes(const QImage& image)
{
    if (image.isNull()) {
        return {};
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return {};
    }
    if (!image.save(&buffer, "PNG")) {
        return {};
    }
    return bytes;
}

QByteArray composeLayeredAppIcon(const hyle::hap::hap_app_icon_content& icon)
{
    std::vector<QImage> backgroundLayers;
    std::vector<QImage> foregroundLayers;
    backgroundLayers.reserve(icon.layers.size());
    foregroundLayers.reserve(icon.layers.size());

    QSize canvasSize;
    for (std::size_t i = 0; i < icon.layers.size(); ++i) {
        const auto& layer = icon.layers.at(i);
        QImage image = imageFromResourceContent(layer);
        if (image.isNull()) {
            continue;
        }
        canvasSize = canvasSize.expandedTo(image.size());
        const bool foreground = i < icon.icon.layers.size()
            && icon.icon.layers.at(i).role == hyle::hap::hap_app_icon_layer_role::foreground;
        if (foreground) {
            foregroundLayers.push_back(std::move(image));
        } else {
            backgroundLayers.push_back(std::move(image));
        }
    }

    if ((backgroundLayers.empty() && foregroundLayers.empty()) || canvasSize.isEmpty()) {
        return {};
    }

    QImage composed(canvasSize, QImage::Format_ARGB32_Premultiplied);
    composed.fill(Qt::transparent);

    QPainter painter(&composed);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRect bounds(QPoint(0, 0), canvasSize);
    for (const auto& image : backgroundLayers) {
        drawImagePreservingAspect(painter, image, bounds);
    }
    for (const auto& image : foregroundLayers) {
        drawImagePreservingAspect(painter, image, bounds);
    }
    painter.end();

    return imageToPngBytes(composed);
}

QByteArray appIconBytes(const hyle::hap::hap_app_icon_content& icon)
{
    if (icon.icon.layered) {
        return composeLayeredAppIcon(icon);
    }
    return bytesToByteArray(icon.resource.bytes);
}

bool isJsonContent(const QByteArray& bytes)
{
    if (bytes.isEmpty()) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    return parseError.error == QJsonParseError::NoError && !document.isNull();
}

QMimeType detectResourceMimeType(const hyle::hap::hap_resource_content& content)
{
    QMimeDatabase mimeDatabase;
    const auto bytes = bytesToByteArray(content.bytes);
    auto mime = mimeDatabase.mimeTypeForFileNameAndData(fromUtf8(content.path), bytes);
    if (!mime.isValid() || mime.name() == QStringLiteral("application/octet-stream")) {
        mime = mimeDatabase.mimeTypeForData(bytes);
    }
    return mime;
}

bool isImageResource(const hyle::hap::hap_resource_content& content)
{
    return isImageResource(content.kind, content.path) || isImageMime(detectResourceMimeType(content));
}

bool isMediaResource(hyle::hap::hap_resource_kind kind, const std::string& path)
{
    if (kind == hyle::hap::hap_resource_kind::media) {
        return true;
    }

    QMimeDatabase mimeDatabase;
    return isMediaMime(mimeDatabase.mimeTypeForFile(fromUtf8(path), QMimeDatabase::MatchExtension));
}

bool isMediaResource(const hyle::hap::hap_resource_content& content)
{
    return isMediaResource(content.kind, content.path) || isMediaMime(detectResourceMimeType(content));
}

QString boolText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString signatureStatusText(hyle::hap::hap_signature_status status)
{
    using hyle::hap::hap_signature_status;
    switch (status) {
    case hap_signature_status::not_found:
        return QStringLiteral("not found");
    case hap_signature_status::profile_found:
        return QStringLiteral("profile found");
    case hap_signature_status::certificate_found:
        return QStringLiteral("certificate found");
    case hap_signature_status::certificate_parse_failed:
        return QStringLiteral("certificate parse failed");
    case hap_signature_status::malformed_profile:
        return QStringLiteral("malformed profile");
    }
    return QStringLiteral("unknown");
}

void appendField(QStringList& lines, const QString& name, const QString& value)
{
    if (!value.isEmpty()) {
        lines.append(QStringLiteral("%1: %2").arg(name, value));
    }
}

void appendField(QStringList& lines, const QString& name, bool value)
{
    lines.append(QStringLiteral("%1: %2").arg(name, boolText(value)));
}

void appendField(QStringList& lines, const QString& name, std::size_t value)
{
    if (value > 0) {
        lines.append(QStringLiteral("%1: %2").arg(name).arg(static_cast<qulonglong>(value)));
    }
}

QString formatCertificate(const hyle::hap::hap_certificate_info& certificate, int index)
{
    QStringList lines;
    lines.append(QStringLiteral("Certificate %1").arg(index + 1));
    lines.append(QStringLiteral("----------------"));
    appendField(lines, QStringLiteral("Status"), fromUtf8(certificate.status));
    appendField(lines, QStringLiteral("Status message"), fromUtf8(certificate.status_message));
    appendField(lines, QStringLiteral("Valid"), certificate.is_valid);
    appendField(lines, QStringLiteral("Signer"), fromUtf8(certificate.signer));
    appendField(lines, QStringLiteral("Issuer"), fromUtf8(certificate.issuer));
    appendField(lines, QStringLiteral("Valid from"), fromUtf8(certificate.valid_from));
    appendField(lines, QStringLiteral("Valid to"), fromUtf8(certificate.valid_to));
    appendField(lines, QStringLiteral("Algorithm"), fromUtf8(certificate.algorithm));
    appendField(lines, QStringLiteral("Key size"), certificate.key_size);
    appendField(lines, QStringLiteral("Thumbprint"), fromUtf8(certificate.thumbprint));

    if (!certificate.fingerprints.empty()) {
        lines.append(QStringLiteral("Fingerprints:"));
        for (const auto& fingerprint : certificate.fingerprints) {
            lines.append(QStringLiteral("  - %1").arg(fromUtf8(fingerprint)));
        }
    }

    return lines.join(QLatin1Char('\n'));
}

QString formatSignatureInfo(const hyle::hap::hap_signature_info& signature)
{
    QStringList lines;
    lines.append(QStringLiteral("HAP Signature"));
    lines.append(QStringLiteral("============="));
    lines.append(QString());

    appendField(lines, QStringLiteral("Status"), signatureStatusText(signature.status));
    appendField(lines, QStringLiteral("Signed"), signature.is_signed);
    appendField(lines, QStringLiteral("Profile found"), signature.profile_found);
    appendField(lines, QStringLiteral("Certificate found"), signature.certificate_found);
    appendField(lines, QStringLiteral("Certificate parsed"), signature.certificate_parsed);
    appendField(lines, QStringLiteral("AppGallery issued"), signature.app_gallery_issued);
    appendField(lines, QStringLiteral("Version name"), fromUtf8(signature.version_name));
    appendField(lines, QStringLiteral("Issuer"), fromUtf8(signature.issuer));
    appendField(lines, QStringLiteral("Certificate status"), fromUtf8(signature.certificate_status));
    appendField(lines, QStringLiteral("Certificate status message"), fromUtf8(signature.certificate_status_message));
    appendField(lines, QStringLiteral("Certificate valid"), signature.certificate_valid);
    appendField(lines, QStringLiteral("Signer"), fromUtf8(signature.signer));
    appendField(lines, QStringLiteral("Certificate issuer"), fromUtf8(signature.certificate_issuer));
    appendField(lines, QStringLiteral("Valid from"), fromUtf8(signature.valid_from));
    appendField(lines, QStringLiteral("Valid to"), fromUtf8(signature.valid_to));
    appendField(lines, QStringLiteral("Algorithm"), fromUtf8(signature.algorithm));
    appendField(lines, QStringLiteral("Key size"), signature.key_size);
    appendField(lines, QStringLiteral("Thumbprint"), fromUtf8(signature.thumbprint));

    if (!signature.fingerprints.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Fingerprints"));
        lines.append(QStringLiteral("------------"));
        for (const auto& fingerprint : signature.fingerprints) {
            lines.append(QStringLiteral("- %1").arg(fromUtf8(fingerprint)));
        }
    }

    if (!signature.certificates.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Certificates"));
        lines.append(QStringLiteral("------------"));
        for (int i = 0; i < static_cast<int>(signature.certificates.size()); ++i) {
            if (i > 0) {
                lines.append(QString());
            }
            lines.append(formatCertificate(signature.certificates.at(static_cast<std::size_t>(i)), i));
        }
    }

    if (!signature.diagnostics.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Diagnostics"));
        lines.append(QStringLiteral("-----------"));
        for (const auto& diagnostic : signature.diagnostics) {
            lines.append(QStringLiteral("- %1").arg(fromUtf8(diagnostic)));
        }
    }

    if (!signature.development_certificate_pem.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Development certificate PEM"));
        lines.append(QStringLiteral("---------------------------"));
        lines.append(fromUtf8(signature.development_certificate_pem));
    }

    return normalizeSourceContent(lines.join(QLatin1Char('\n')));
}

DecompiledSourceFile inspectSignatureFile(const std::string& hylePath)
{
    auto signature = hyle::hap::inspect_hap_signature(hylePath);
    if (!signature) {
        return {
            QStringLiteral("Signature.txt"),
            QStringLiteral("TXT"),
            errorMessage("Inspect signature failed", signature.error()),
            {},
            QStringLiteral("signature"),
            QStringLiteral("text"),
            0,
            false
        };
    }

    return {
        QStringLiteral("Signature.txt"),
        QStringLiteral("TXT"),
        formatSignatureInfo(*signature),
        {},
        QStringLiteral("signature"),
        QStringLiteral("text"),
        0,
        false
    };
}

DecompiledSourceFile inspectSummaryFile(const HyleDecompiler::SessionContext& context)
{
    auto summary = hyle::async::sync_wait(
        context.session.summary_async(
            context.scheduler(),
            context.stopToken()));
    if (!summary) {
        return {
            QStringLiteral("Summary"),
            QStringLiteral("TXT"),
            errorMessage("Summarize package failed", summary.error()),
            {},
            QStringLiteral("summary"),
            QStringLiteral("text"),
            0,
            false
        };
    }

    return {
        QStringLiteral("Summary"),
        QStringLiteral("TXT"),
        normalizeSourceContent(fromUtf8(hyle::hap::format_decompiled_package_summary(*summary))),
        {},
        QStringLiteral("summary"),
        QStringLiteral("text"),
        0,
        false
    };
}

DecompiledSourceFile lazySignatureFile()
{
    return {
        QStringLiteral("Signature.txt"),
        QStringLiteral("TXT"),
        {},
        {},
        QStringLiteral("signature"),
        QStringLiteral("text"),
        0,
        true
    };
}

DecompiledSourceFile lazySummaryFile()
{
    return {
        QStringLiteral("Summary"),
        QStringLiteral("TXT"),
        {},
        {},
        QStringLiteral("summary"),
        QStringLiteral("text"),
        0,
        true
    };
}

QString normalizeSourceContent(QString content)
{
    content.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    content.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    QStringList lines = content.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    return lines.join(QLatin1Char('\n'));
}

std::vector<DecompiledSourceFile> fromSourceFiles(
    const std::vector<hyle::hap::decompiled_source_file>& sourceFiles)
{
    std::vector<DecompiledSourceFile> files;
    files.reserve(sourceFiles.size());

    for (const auto& file : sourceFiles) {
        files.push_back({
            fromUtf8(file.path),
            languageKind(file.language),
            normalizeSourceContent(fromUtf8(file.content)),
            {},
            QStringLiteral("source"),
            QStringLiteral("text"),
            0,
            false
        });
    }

    return files;
}

std::optional<std::size_t> moduleIdForSourceFile(
    const hyle::hap::decompiled_source_file_info& sourceFile,
    const std::vector<hyle::hap::decompiled_module>& modules)
{
    for (const std::string& entry : sourceFile.module_entries) {
        const auto found = std::ranges::find_if(modules, [&entry](const hyle::hap::decompiled_module& module) {
            return module.entry_path == entry;
        });
        if (found != modules.end()) {
            return found->id;
        }
    }

    if (sourceFile.module_entries.empty() && modules.size() == 1U) {
        return modules.front().id;
    }

    return std::nullopt;
}

} // namespace

namespace HyleDecompiler {

SessionContext::SessionContext()
    : executor(std::max<std::size_t>(
          2U,
          static_cast<std::size_t>(std::thread::hardware_concurrency())))
{
}

hyle::async::scheduler SessionContext::scheduler() const noexcept
{
    return executor.get_scheduler();
}

std::stop_token SessionContext::stopToken() const noexcept
{
    return stopSource.get_token();
}

void SessionContext::requestStop() noexcept
{
    stopSource.request_stop();
}

OpenResult openFile(
    const QString& filePath,
    const std::shared_ptr<SessionContext>& context)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::openFile"));

    OpenResult result;
    result.context = context;
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        result.error = QObject::tr("File does not exist: %1").arg(filePath);
        return result;
    }
    if (!context) {
        result.error = QObject::tr("Decompiler worker is not available.");
        return result;
    }
    if (context->stopToken().stop_requested()) {
        result.error = QObject::tr("Open package cancelled.");
        return result;
    }

    const auto hylePath = toUtf8Path(filePath);

    if (isAbcFile(filePath)) {
        auto files = hyle::hap::decompile_abc_sources(hylePath);
        if (!files) {
            result.error = errorMessage("Decompile failed", files.error());
            return result;
        }
        result.files = fromSourceFiles(*files);
        result.status = QObject::tr("Decompiled %1 file(s)").arg(static_cast<int>(result.files.size()));
        return result;
    }

    auto session = hyle::async::sync_wait(
        hyle::hap::open_decompiled_package_async(
            context->scheduler(),
            hylePath,
            {},
            context->stopToken()));
    if (!session) {
        result.error = errorMessage("Open package failed", session.error());
        return result;
    }

    context->session = std::move(*session);

    auto launcherIcon = hyle::async::sync_wait(
        context->session.read_launcher_app_icon_async(
            context->scheduler(),
            context->stopToken()));
    if (launcherIcon) {
        result.appIconBytes = appIconBytes(*launcherIcon);
        result.appIconPath = fromUtf8(launcherIcon->icon.path);
        result.appIconLayered = launcherIcon->icon.layered;
        if (result.appIconPath.isEmpty()) {
            result.appIconPath = fromUtf8(launcherIcon->resource.path);
        }
    }

    const auto sourceFileCount = context->session.source_files().size();
    result.files.reserve(sourceFileCount + context->session.resources().size() + 2U);
    const auto& modules = context->session.modules();
    for (const auto& file : context->session.source_files()) {
        const auto moduleId = moduleIdForSourceFile(file, modules);
        result.files.push_back({
            fromUtf8(file.path),
            languageKind(file.language),
            {},
            {},
            QStringLiteral("source"),
            QStringLiteral("text"),
            file.id,
            true,
            false,
            moduleId,
            true
        });
    }
    for (const auto& resource : context->session.resources()) {
        result.files.push_back({
            fromUtf8(resource.path),
            resourceKindName(resource.kind),
            {},
            {},
            QStringLiteral("resource"),
            isImageResource(resource.kind, resource.path)
                ? QStringLiteral("image")
                : isMediaResource(resource.kind, resource.path) ? QStringLiteral("media")
                : isTextResource(resource.kind) ? QStringLiteral("text") : QStringLiteral("hex"),
            resource.id,
            !resource.is_directory,
            resource.is_directory
        });
    }

    result.files.push_back(lazySignatureFile());
    result.files.push_back(lazySummaryFile());

    result.status = QObject::tr("Indexed %1 source file(s)").arg(static_cast<int>(sourceFileCount));
    return result;
}

SourceResult readResourceContent(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name,
    std::stop_token stopToken)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::readResourceContent"));

    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;

    if (!context || !context->session.valid()) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    LinkedStopToken linkedStop(context->stopToken(), stopToken);
    auto content = hyle::async::sync_wait(
        context->session.read_resource_async(
            context->scheduler(),
            hyleId,
            linkedStop.token()));
    if (!content) {
        result.error = errorMessage("Read resource failed", content.error());
        return result;
    }

    const QByteArray bytes = bytesToByteArray(content->bytes);
    const bool textResource = isTextResource(content->kind);
    const bool jsonContent = !textResource && isJsonContent(bytes);

    result.kind = jsonContent ? QStringLiteral("JSON") : resourceKindName(content->kind);
    if (isImageResource(*content)) {
        result.contentMode = QStringLiteral("image");
        result.binaryContent = bytes;
    } else if (isMediaResource(*content)) {
        result.contentMode = QStringLiteral("media");
        result.binaryContent = bytes;
    } else if (textResource || jsonContent) {
        result.contentMode = QStringLiteral("text");
        result.binaryContent = bytes;
        result.content = normalizeSourceContent(QString::fromUtf8(bytes.constData(), bytes.size()));
    } else {
        result.contentMode = QStringLiteral("hex");
        result.binaryContent = bytes;
    }
    return result;
}

SourceResult readSignatureContent(
    const QString& filePath,
    int nodeIndex,
    const QString& name)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::readSignatureContent"));

    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;
    result.kind = QStringLiteral("TXT");
    result.contentMode = QStringLiteral("text");

    const auto signature = inspectSignatureFile(toUtf8Path(filePath));
    result.content = signature.content;
    return result;
}

SourceResult readSummaryContent(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    const QString& name,
    std::stop_token stopToken)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::readSummaryContent"));

    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;
    result.kind = QStringLiteral("TXT");
    result.contentMode = QStringLiteral("text");

    if (!context || !context->session.valid()) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    if (stopToken.stop_requested() || context->stopToken().stop_requested()) {
        result.error = QObject::tr("Operation cancelled.");
        return result;
    }

    result.content = inspectSummaryFile(*context).content;
    return result;
}

SourceResult decompileSourceFile(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name,
    std::stop_token stopToken)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::decompileSourceFile"));

    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;

    if (!context || !context->session.valid()) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    LinkedStopToken linkedStop(context->stopToken(), stopToken);
    auto package = hyle::async::sync_wait(
        context->session.decompile_source_file_async(
            context->scheduler(),
            hyleId,
            {},
            linkedStop.token()));
    if (!package) {
        result.error = errorMessage("Decompile failed", package.error());
        return result;
    }

    result.diagnostics = diagnosticsText(package->diagnostics);

    if (!package->files.empty()) {
        applyDecompiledSource(result, package->files.front());
    } else {
        result.content = QObject::tr("// Hyle returned no source content.");
    }

    return result;
}

SourceBatchResult decompileSourceFiles(
    const std::shared_ptr<SessionContext>& context,
    std::vector<SourceRequest> requests)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::decompileSourceFiles"));

    SourceBatchResult batch;
    batch.files.reserve(requests.size());
    for (const auto& request : requests) {
        SourceResult result;
        result.nodeIndex = request.nodeIndex;
        result.name = request.name;
        batch.files.push_back(std::move(result));
    }

    if (requests.empty()) {
        return batch;
    }

    if (!context || !context->session.valid()) {
        for (auto& result : batch.files) {
            result.error = QObject::tr("Decompiler session is not available.");
        }
        return batch;
    }

    std::vector<std::size_t> sourceFileIds;
    sourceFileIds.reserve(requests.size());
    QHash<std::size_t, int> idToBatchIndex;
    QHash<QString, int> pathToBatchIndex;
    for (int i = 0; i < static_cast<int>(requests.size()); ++i) {
        const auto& request = requests.at(static_cast<std::size_t>(i));
        sourceFileIds.push_back(request.hyleId);
        idToBatchIndex.insert(request.hyleId, i);
        pathToBatchIndex.insert(request.path, i);
    }

    auto package = hyle::async::sync_wait(
        context->session.decompile_source_files_async(
            context->scheduler(),
            std::move(sourceFileIds),
            {},
            context->stopToken()));
    if (!package) {
        const QString error = errorMessage("Decompile failed", package.error());
        for (auto& result : batch.files) {
            result.error = error;
        }
        return batch;
    }

    const QString diagnostics = diagnosticsText(package->diagnostics);
    for (auto& result : batch.files) {
        result.diagnostics = diagnostics;
    }

    std::vector<bool> assigned(requests.size(), false);
    for (const auto& source : package->files) {
        const QString path = fromUtf8(source.path);
        int batchIndex = pathToBatchIndex.value(path, -1);
        if (batchIndex < 0) {
            const auto foundInfo = std::ranges::find_if(
                context->session.source_files(),
                [&source](const hyle::hap::decompiled_source_file_info& info) {
                    return info.path == source.path;
                });
            if (foundInfo != context->session.source_files().end()) {
                batchIndex = idToBatchIndex.value(foundInfo->id, -1);
            }
        }
        if (batchIndex >= 0 && batchIndex < static_cast<int>(batch.files.size())) {
            applyDecompiledSource(batch.files.at(static_cast<std::size_t>(batchIndex)), source);
            assigned.at(static_cast<std::size_t>(batchIndex)) = true;
        }
    }

    int fallbackIndex = 0;
    for (const auto& source : package->files) {
        while (fallbackIndex < static_cast<int>(assigned.size())
               && assigned.at(static_cast<std::size_t>(fallbackIndex))) {
            ++fallbackIndex;
        }
        if (fallbackIndex >= static_cast<int>(assigned.size())) {
            break;
        }
        applyDecompiledSource(batch.files.at(static_cast<std::size_t>(fallbackIndex)), source);
        assigned.at(static_cast<std::size_t>(fallbackIndex)) = true;
        ++fallbackIndex;
    }

    for (int i = 0; i < static_cast<int>(assigned.size()); ++i) {
        if (!assigned.at(static_cast<std::size_t>(i))) {
            batch.files.at(static_cast<std::size_t>(i)).content =
                QObject::tr("// Hyle returned no source content.");
        }
    }

    return batch;
}

bool isSourceFileCached(
    const std::shared_ptr<SessionContext>& context,
    std::size_t hyleId)
{
    return context && context->session.valid() && context->session.is_source_file_cached(hyleId, {});
}

DisassemblyResult disassembleSourceFileText(
    const std::shared_ptr<SessionContext>& context,
    int nodeIndex,
    std::size_t sourceFileId,
    const QString& name,
    std::stop_token stopToken)
{
    PerformanceTrace trace(QStringLiteral("HyleDecompiler::disassembleSourceFileText"));

    DisassemblyResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;

    if (!context || !context->session.valid()) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    LinkedStopToken linkedStop(context->stopToken(), stopToken);
    auto text = hyle::async::sync_wait(
        context->session.disassemble_source_file_text_async(
            context->scheduler(),
            sourceFileId,
            {},
            linkedStop.token()));
    if (!text) {
        result.error = errorMessage("Disassemble failed", text.error());
        return result;
    }

    result.content = normalizeSourceContent(fromUtf8(*text));
    return result;
}

} // namespace HyleDecompiler
