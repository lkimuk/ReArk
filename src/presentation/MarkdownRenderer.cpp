#include "presentation/MarkdownRenderer.h"

#include <QFutureWatcher>
#include <QMetaObject>
#include <QRegularExpression>
#include <QStringList>
#include <QUuid>
#ifndef REARK_HAS_CMARK_GFM
#include <QTextDocument>
#endif
#include <QVariantMap>
#include <QtConcurrent>

#ifdef REARK_HAS_CMARK_GFM
#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm-extension_api.h>
#include <cmark-gfm.h>
#endif

namespace {

struct MarkdownInlineCode {
  QString token;
  QString code;
};

bool isFenceLine(const QString &line, QString *marker, QString *info) {
  const QString trimmed = line.trimmed();
  if (trimmed.size() < 3) {
    return false;
  }

  const QChar fenceChar = trimmed.at(0);
  if (fenceChar != QLatin1Char('`') && fenceChar != QLatin1Char('~')) {
    return false;
  }

  int count = 0;
  while (count < trimmed.size() && trimmed.at(count) == fenceChar) {
    ++count;
  }
  if (count < 3) {
    return false;
  }

  *marker = QString(count, fenceChar);
  *info = trimmed.mid(count).trimmed();
  return true;
}

QString normalizedLanguage(QString language) {
  language = language.trimmed().toLower();
  const int separator =
      language.indexOf(QRegularExpression(QStringLiteral("[\\s,{]")));
  if (separator > 0) {
    language.truncate(separator);
  }

  if (language == QStringLiteral("typescript") ||
      language == QStringLiteral("arkts")) {
    return QStringLiteral("ts");
  }
  if (language == QStringLiteral("javascript")) {
    return QStringLiteral("js");
  }
  if (language == QStringLiteral("cpp") || language == QStringLiteral("c++")) {
    return QStringLiteral("cpp");
  }
  if (language == QStringLiteral("python")) {
    return QStringLiteral("py");
  }
  if (language == QStringLiteral("shell") ||
      language == QStringLiteral("zsh")) {
    return QStringLiteral("sh");
  }
  if (language == QStringLiteral("cmakelists") ||
      language == QStringLiteral("cmake")) {
    return QStringLiteral("cmake");
  }
  return language;
}

bool isPlainTextLanguage(const QString &language) {
  const QString normalized = normalizedLanguage(language);
  return normalized.isEmpty() || normalized == QStringLiteral("text") ||
         normalized == QStringLiteral("plain") ||
         normalized == QStringLiteral("plaintext") ||
         normalized == QStringLiteral("txt") ||
         normalized == QStringLiteral("console");
}

QString displayLanguage(QString language) {
  const QString normalized = normalizedLanguage(std::move(language));
  if (normalized == QStringLiteral("ts")) {
    return QStringLiteral("TypeScript");
  }
  if (normalized == QStringLiteral("js")) {
    return QStringLiteral("JavaScript");
  }
  if (normalized == QStringLiteral("json")) {
    return QStringLiteral("JSON");
  }
  if (normalized == QStringLiteral("cpp")) {
    return QStringLiteral("C++");
  }
  if (normalized == QStringLiteral("py")) {
    return QStringLiteral("Python");
  }
  if (normalized == QStringLiteral("sh") ||
      normalized == QStringLiteral("bash")) {
    return QStringLiteral("Shell");
  }
  if (normalized == QStringLiteral("cmake")) {
    return QStringLiteral("CMake");
  }
  if (normalized == QStringLiteral("ets")) {
    return QStringLiteral("ArkTS");
  }
  if (isPlainTextLanguage(normalized)) {
    return QStringLiteral("Text");
  }
  if (normalized.isEmpty()) {
    return QStringLiteral("Code");
  }
  QString label = normalized;
  label[0] = label.at(0).toUpper();
  return label;
}

bool isSingleLinePlainBlock(const QString &code, const QString &language) {
  if (!isPlainTextLanguage(language)) {
    return false;
  }

  const QString trimmed = code.trimmed();
  return !trimmed.isEmpty() && !trimmed.contains(QLatin1Char('\n'));
}

QString normalizeCodeBlockText(QString code) {
  code.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  code.replace(QLatin1Char('\r'), QLatin1Char('\n'));
  QStringList lines = code.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
  while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
    lines.removeLast();
  }
  return lines.join(QLatin1Char('\n'));
}

QString htmlInlineCode(const QString &code, bool darkTheme) {
  const QString background =
      darkTheme ? QStringLiteral("#202226") : QStringLiteral("#edf2f8");
  const QString border =
      darkTheme ? QStringLiteral("#34383d") : QStringLiteral("#d2dbe8");
  const QString text =
      darkTheme ? QStringLiteral("#d8d8d8") : QStringLiteral("#172033");

  return QStringLiteral("<span style=\"color:%1; background-color:%2; "
                        "border:1px solid %3; "
                        "font-family:'%5', Consolas, 'Courier New', "
                        "monospace; font-size:12px; "
                        "white-space:pre;\">&nbsp;%4&nbsp;</span>")
      .arg(text, background, border, code.toHtmlEscaped(),
           QStringLiteral("Cascadia Mono"));
}

constexpr qsizetype kMaxCacheEntries = 96;

QString markdownCacheKey(const QString &markdown, bool darkTheme);
QString extractInlineCode(const QString &line, const QString &tokenPrefix,
                          QVector<MarkdownInlineCode> *inlineCodes);
QString inlineCodeTokenPrefix();
QString renderInlineCode(const MarkdownInlineCode &code, bool darkTheme);
QString markdownStyleSheet(bool darkTheme);
QString renderMarkdownTextHtml(const QString &markdown, bool darkTheme);
QString renderInlineMarkdownHtml(const QString &markdown, bool darkTheme);
QVariantMap renderCodeBlockModel(const QString &code, const QString &language,
                                 bool darkTheme);
QVariantMap renderTableBlockModel(const QStringList &headerCells,
                                  const QStringList &separatorCells,
                                  const QList<QStringList> &bodyRows,
                                  bool darkTheme);
QVariantList renderMarkdownBlocks(const QString &markdown, bool darkTheme);
void appendMarkdownTextBlock(QVariantList *blocks, const QStringList &lines,
                             bool darkTheme);
void appendMarkdownHtmlBlock(QVariantList *blocks, const QStringList &lines,
                             bool darkTheme);
QStringList splitMarkdownTableRow(const QString &line);
bool isMarkdownTableSeparator(const QString &line,
                              QStringList *cells = nullptr);
QString renderTableCellHtml(const QString &markdown, bool darkTheme);

} // namespace

MarkdownRenderer::MarkdownRenderer(QObject *parent) : QObject(parent) {}

QVariantList MarkdownRenderer::renderBlocks(const QString &markdown,
                                            bool darkTheme) const {
  const QString key = markdownCacheKey(markdown, darkTheme);
  const auto cached = blockCache_.constFind(key);
  if (cached != blockCache_.constEnd()) {
    return cached.value();
  }

  const QVariantList blocks = renderMarkdownBlocks(markdown, darkTheme);
  rememberRenderedBlocks(key, blocks);
  while (blockCacheOrder_.size() > kMaxCacheEntries) {
    blockCache_.remove(blockCacheOrder_.takeFirst());
  }
  return blocks;
}

int MarkdownRenderer::renderBlocksAsync(const QString &markdown,
                                        bool darkTheme) {
  const int requestId = ++nextRequestId_;
  const QString key = markdownCacheKey(markdown, darkTheme);
  const auto cached = blockCache_.constFind(key);
  if (cached != blockCache_.constEnd()) {
    const QVariantList blocks = cached.value();
    QMetaObject::invokeMethod(
        this,
        [this, requestId, blocks] { emit blocksReady(requestId, blocks); },
        Qt::QueuedConnection);
    return requestId;
  }

  auto *watcher = new QFutureWatcher<QVariantList>(this);
  connect(watcher, &QFutureWatcher<QVariantList>::finished, this,
          [this, watcher, requestId, key] {
            const QVariantList blocks = watcher->result();
            rememberRenderedBlocks(key, blocks);
            while (blockCacheOrder_.size() > kMaxCacheEntries) {
              blockCache_.remove(blockCacheOrder_.takeFirst());
            }
            emit blocksReady(requestId, blocks);
            watcher->deleteLater();
          });
  watcher->setFuture(QtConcurrent::run([markdown, darkTheme] {
    return renderMarkdownBlocks(markdown, darkTheme);
  }));
  return requestId;
}

namespace {

QString markdownCacheKey(const QString &markdown, bool darkTheme) {
  QString key;
  key.reserve(markdown.size() + 2);
  key.append(darkTheme ? QLatin1Char('1') : QLatin1Char('0'));
  key.append(QChar(0x1f));
  key.append(markdown);
  return key;
}

QString renderMarkdownTextHtml(const QString &markdown, bool darkTheme) {
  QVector<MarkdownInlineCode> inlineCodes;
  const QString tokenPrefix = inlineCodeTokenPrefix();
  QString prepared;
  prepared.reserve(markdown.size());

  const QStringList lines = markdown.split(QLatin1Char('\n'));
  for (const QString &line : lines) {
    prepared += extractInlineCode(line, tokenPrefix, &inlineCodes);
    prepared += QLatin1Char('\n');
  }

  if (prepared.trimmed().isEmpty() && inlineCodes.isEmpty()) {
    return {};
  }

#ifdef REARK_HAS_CMARK_GFM
  cmark_gfm_core_extensions_ensure_registered();

  constexpr int options = CMARK_OPT_DEFAULT | CMARK_OPT_VALIDATE_UTF8 |
                          CMARK_OPT_SMART | CMARK_OPT_GITHUB_PRE_LANG |
                          CMARK_OPT_TABLE_PREFER_STYLE_ATTRIBUTES;
  cmark_parser *parser = cmark_parser_new(options);
  if (parser != nullptr) {
    for (const char *extensionName :
         {"table", "strikethrough", "tasklist", "autolink"}) {
      if (auto *extension = cmark_find_syntax_extension(extensionName)) {
        cmark_parser_attach_syntax_extension(parser, extension);
      }
    }

    const QByteArray bytes = prepared.toUtf8();
    cmark_parser_feed(parser, bytes.constData(),
                      static_cast<size_t>(bytes.size()));
    cmark_node *document = cmark_parser_finish(parser);
    if (document != nullptr) {
      char *rendered = cmark_render_html(
          document, options, cmark_parser_get_syntax_extensions(parser));
      QString html =
          rendered != nullptr ? QString::fromUtf8(rendered) : QString();
      if (rendered != nullptr) {
        cmark_get_default_mem_allocator()->free(rendered);
      }
      cmark_node_free(document);
      cmark_parser_free(parser);
      for (const MarkdownInlineCode &code : inlineCodes) {
        html.replace(code.token, renderInlineCode(code, darkTheme));
      }
      return QStringLiteral("<style>%1</style>%2")
          .arg(markdownStyleSheet(darkTheme), html);
    }
    cmark_parser_free(parser);
  }
#endif

#ifndef REARK_HAS_CMARK_GFM
  QTextDocument document;
  document.setDocumentMargin(0);
  document.setDefaultStyleSheet(markdownStyleSheet(darkTheme));
  QTextDocument::MarkdownFeatures features =
      QTextDocument::MarkdownDialectGitHub;
  features.setFlag(QTextDocument::MarkdownNoHTML);
  document.setMarkdown(prepared, features);
  QString html = document.toHtml();
  for (const MarkdownInlineCode &code : inlineCodes) {
    html.replace(code.token, renderInlineCode(code, darkTheme));
  }
  return html;
#else
  QString fallback = prepared.toHtmlEscaped();
  fallback.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
  for (const MarkdownInlineCode &code : inlineCodes) {
    fallback.replace(code.token, renderInlineCode(code, darkTheme));
  }
  return QStringLiteral("<style>%1</style><p>%2</p>")
      .arg(markdownStyleSheet(darkTheme), fallback);
#endif
}

QString renderInlineMarkdownHtml(const QString &markdown, bool darkTheme) {
  QString html = renderMarkdownTextHtml(markdown, darkTheme).trimmed();
  static const QRegularExpression stylePrefix(
      QStringLiteral("^\\s*<style[^>]*>.*?</style>\\s*"),
      QRegularExpression::DotMatchesEverythingOption);
  html.remove(stylePrefix);

  static const QRegularExpression paragraphWrapper(
      QStringLiteral("^\\s*<p>(.*)</p>\\s*$"),
      QRegularExpression::DotMatchesEverythingOption);
  const QRegularExpressionMatch paragraphMatch = paragraphWrapper.match(html);
  if (paragraphMatch.hasMatch()) {
    html = paragraphMatch.captured(1).trimmed();
  }

  html.replace(QStringLiteral("\\|"), QStringLiteral("|"));
  return html;
}

QVariantMap renderCodeBlockModel(const QString &code, const QString &language,
                                 bool darkTheme) {
  const QString normalized = normalizedLanguage(language);
  const QString normalizedCode = normalizeCodeBlockText(code);
  const QStringList lines = normalizedCode.split(QLatin1Char('\n'));

  QVariantMap block;
  block.insert(QStringLiteral("type"), QStringLiteral("code"));
  block.insert(QStringLiteral("language"), normalized);
  block.insert(QStringLiteral("languageLabel"), displayLanguage(normalized));
  block.insert(QStringLiteral("code"), normalizedCode);
  block.insert(QStringLiteral("compact"),
               isSingleLinePlainBlock(normalizedCode, normalized));
  block.insert(QStringLiteral("lineCount"), std::max(1, int(lines.size())));
  return block;
}

QVariantMap renderTableCell(QString markdown, bool darkTheme) {
  QVariantMap cell;
  const QString plainText = markdown.trimmed();
  cell.insert(QStringLiteral("text"), plainText);
  cell.insert(QStringLiteral("html"),
              renderTableCellHtml(plainText, darkTheme));
  return cell;
}

QVariantMap renderTableBlockModel(const QStringList &headerCells,
                                  const QStringList &separatorCells,
                                  const QList<QStringList> &bodyRows,
                                  bool darkTheme) {
  QVariantMap block;
  block.insert(QStringLiteral("type"), QStringLiteral("table"));

  QVariantList alignments;
  for (const QString &separator : separatorCells) {
    const QString trimmed = separator.trimmed();
    if (trimmed.startsWith(QLatin1Char(':')) &&
        trimmed.endsWith(QLatin1Char(':'))) {
      alignments.append(QStringLiteral("center"));
    } else if (trimmed.endsWith(QLatin1Char(':'))) {
      alignments.append(QStringLiteral("right"));
    } else {
      alignments.append(QStringLiteral("left"));
    }
  }

  QVariantList headers;
  for (const QString &cell : headerCells) {
    headers.append(renderTableCell(cell, darkTheme));
  }

  QVariantList rows;
  QVector<int> columnWeights(headerCells.size(), 8);
  for (int column = 0; column < headerCells.size(); ++column) {
    columnWeights[column] =
        std::clamp(static_cast<int>(headerCells.at(column).size()), 8, 72);
  }
  for (const QStringList &row : bodyRows) {
    QVariantList renderedRow;
    for (int column = 0; column < headerCells.size(); ++column) {
      const QString cellText = column < row.size() ? row.at(column) : QString();
      const int cellWeight =
          std::clamp(static_cast<int>(cellText.size()), 8, 120);
      columnWeights[column] = std::max(columnWeights.at(column), cellWeight);
      renderedRow.append(renderTableCell(cellText, darkTheme));
    }
    rows.append(QVariant::fromValue(renderedRow));
  }

  QVariantList weights;
  for (int weight : columnWeights) {
    weights.append(weight);
  }

  block.insert(QStringLiteral("headers"), headers);
  block.insert(QStringLiteral("rows"), rows);
  block.insert(QStringLiteral("alignments"), alignments);
  block.insert(QStringLiteral("columnWeights"), weights);
  block.insert(QStringLiteral("columnCount"), headerCells.size());
  block.insert(QStringLiteral("rowCount"), bodyRows.size() + 1);
  return block;
}

void appendMarkdownHtmlBlock(QVariantList *blocks, const QStringList &lines,
                             bool darkTheme) {
  const QString markdown = lines.join(QLatin1Char('\n')).trimmed();
  if (markdown.isEmpty()) {
    return;
  }

  const QString html = renderMarkdownTextHtml(markdown, darkTheme);
  if (html.trimmed().isEmpty()) {
    return;
  }

  QVariantMap block;
  block.insert(QStringLiteral("type"), QStringLiteral("html"));
  block.insert(QStringLiteral("html"), html);
  blocks->append(block);
}

void appendMarkdownTextBlock(QVariantList *blocks, const QStringList &lines,
                             bool darkTheme) {
  QStringList pendingHtmlLines;
  const auto flushPendingHtml = [&] {
    appendMarkdownHtmlBlock(blocks, pendingHtmlLines, darkTheme);
    pendingHtmlLines.clear();
  };

  for (int i = 0; i < lines.size();) {
    QStringList separatorCells;
    if (i + 1 < lines.size() &&
        isMarkdownTableSeparator(lines.at(i + 1), &separatorCells)) {
      const QStringList headerCells = splitMarkdownTableRow(lines.at(i));
      if (headerCells.size() >= 2 && separatorCells.size() >= 2 &&
          separatorCells.size() == headerCells.size()) {
        flushPendingHtml();

        QList<QStringList> bodyRows;
        int rowIndex = i + 2;
        while (rowIndex < lines.size()) {
          const QString line = lines.at(rowIndex);
          if (line.trimmed().isEmpty()) {
            break;
          }
          QStringList rowCells = splitMarkdownTableRow(line);
          if (rowCells.size() < 2) {
            break;
          }
          while (rowCells.size() < headerCells.size()) {
            rowCells.append(QString());
          }
          if (rowCells.size() > headerCells.size()) {
            rowCells = rowCells.mid(0, headerCells.size());
          }
          bodyRows.append(rowCells);
          ++rowIndex;
        }

        blocks->append(renderTableBlockModel(headerCells, separatorCells,
                                             bodyRows, darkTheme));
        i = rowIndex;
        continue;
      }
    }

    pendingHtmlLines.append(lines.at(i));
    ++i;
  }

  flushPendingHtml();
}

QVariantList renderMarkdownBlocks(const QString &markdown, bool darkTheme) {
  QVariantList blocks;
  QStringList textLines;
  QStringList codeLines;
  bool inFence = false;
  QString fenceMarker;
  QString language;

  const QStringList lines = markdown.split(QLatin1Char('\n'));
  for (const QString &line : lines) {
    QString marker;
    QString info;
    if (!inFence && isFenceLine(line, &marker, &info)) {
      appendMarkdownTextBlock(&blocks, textLines, darkTheme);
      textLines.clear();
      inFence = true;
      fenceMarker = marker;
      language = info;
      codeLines.clear();
      continue;
    }

    if (inFence) {
      if (line.trimmed().startsWith(fenceMarker)) {
        blocks.append(renderCodeBlockModel(codeLines.join(QLatin1Char('\n')),
                                           language, darkTheme));
        inFence = false;
        fenceMarker.clear();
        language.clear();
        codeLines.clear();
      } else {
        codeLines.append(line);
      }
      continue;
    }

    textLines.append(line);
  }

  if (inFence) {
    textLines.append(fenceMarker + (language.isEmpty()
                                        ? QString()
                                        : QStringLiteral(" ") + language));
    textLines.append(codeLines);
  }
  appendMarkdownTextBlock(&blocks, textLines, darkTheme);
  return blocks;
}

QString extractInlineCode(const QString &line, const QString &tokenPrefix,
                          QVector<MarkdownInlineCode> *inlineCodes) {
  QString output;
  output.reserve(line.size());

  int i = 0;
  while (i < line.size()) {
    const int start = line.indexOf(QLatin1Char('`'), i);
    if (start < 0) {
      output += line.mid(i);
      break;
    }

    int ticks = 0;
    while (start + ticks < line.size() &&
           line.at(start + ticks) == QLatin1Char('`')) {
      ++ticks;
    }
    if (ticks <= 0) {
      output += line.mid(i);
      break;
    }

    const QString marker(ticks, QLatin1Char('`'));
    const int end = line.indexOf(marker, start + ticks);
    if (end < 0) {
      output += line.mid(i);
      break;
    }

    const QString code = line.mid(start + ticks, end - start - ticks);
    if (code.isEmpty()) {
      output += line.mid(i, end + ticks - i);
      i = end + ticks;
      continue;
    }

    const QString token =
        QStringLiteral("%1%2END").arg(tokenPrefix).arg(inlineCodes->size());
    inlineCodes->append(MarkdownInlineCode{token, code});
    output += line.mid(i, start - i);
    output += token;
    i = end + ticks;
  }

  return output;
}

QString inlineCodeTokenPrefix() {
  return QStringLiteral("REARKINLINECODE%1N")
      .arg(QUuid::createUuid().toString(QUuid::Id128));
}

QString renderInlineCode(const MarkdownInlineCode &code, bool darkTheme) {
  return htmlInlineCode(code.code, darkTheme);
}

QStringList splitMarkdownTableRow(const QString &line) {
  QString trimmed = line.trimmed();
  if (trimmed.startsWith(QLatin1Char('|'))) {
    trimmed.remove(0, 1);
  }
  if (trimmed.endsWith(QLatin1Char('|'))) {
    trimmed.chop(1);
  }

  QStringList cells;
  QString current;
  bool escaped = false;
  bool inCode = false;
  for (QChar ch : trimmed) {
    if (escaped) {
      current.append(ch);
      escaped = false;
      continue;
    }
    if (ch == QLatin1Char('\\')) {
      escaped = true;
      continue;
    }
    if (ch == QLatin1Char('`')) {
      inCode = !inCode;
      current.append(ch);
      continue;
    }
    if (ch == QLatin1Char('|') && !inCode) {
      cells.append(current.trimmed());
      current.clear();
      continue;
    }
    current.append(ch);
  }
  cells.append(current.trimmed());
  return cells;
}

bool isMarkdownTableSeparatorCell(const QString &cell) {
  const QString trimmed = cell.trimmed();
  if (trimmed.size() < 3) {
    return false;
  }
  static const QRegularExpression separatorPattern(
      QStringLiteral("^:?-{3,}:?$"));
  return separatorPattern.match(trimmed).hasMatch();
}

bool isMarkdownTableSeparator(const QString &line, QStringList *cells) {
  const QStringList parsedCells = splitMarkdownTableRow(line);
  if (parsedCells.size() < 2) {
    return false;
  }
  if (!std::ranges::all_of(parsedCells, isMarkdownTableSeparatorCell)) {
    return false;
  }
  if (cells != nullptr) {
    *cells = parsedCells;
  }
  return true;
}

QString renderTableCellHtml(const QString &markdown, bool darkTheme) {
  return renderInlineMarkdownHtml(markdown, darkTheme);
}

QString markdownStyleSheet(bool darkTheme) {
  const QString text =
      darkTheme ? QStringLiteral("#eef5ff") : QStringLiteral("#0f172a");
  const QString muted =
      darkTheme ? QStringLiteral("#98a7bb") : QStringLiteral("#64748b");
  const QString link =
      darkTheme ? QStringLiteral("#8fb3ff") : QStringLiteral("#315bdc");
  const QString codeText =
      darkTheme ? QStringLiteral("#d8d8d8") : QStringLiteral("#172033");
  const QString codeBackground =
      darkTheme ? QStringLiteral("#202226") : QStringLiteral("#eef3f9");
  const QString codeBorder =
      darkTheme ? QStringLiteral("#34383d") : QStringLiteral("#d5deeb");
  const QString quoteBorder =
      darkTheme ? QStringLiteral("#44546a") : QStringLiteral("#c6d2e2");
  const QString quoteBackground =
      darkTheme ? QStringLiteral("#121b27") : QStringLiteral("#f4f7fb");

  return QStringLiteral(R"(
body {
  color: %1;
  margin: 0;
  font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
  font-size: 13px;
  line-height: 1.48;
}
h1, h2, h3, h4, h5, h6 {
  color: %1;
  font-weight: 600;
  margin-top: 13px;
  margin-bottom: 7px;
}
h1 {
  font-size: 20px;
}
h2 {
  font-size: 18px;
}
h3, h4, h5, h6 {
  font-size: 15px;
}
p {
  margin-top: 0;
  margin-bottom: 10px;
}
p:last-child {
  margin-bottom: 0;
}
a {
  color: %3;
  text-decoration: none;
}
strong {
  font-weight: 600;
}
ul, ol {
  margin-top: 6px;
  margin-bottom: 11px;
  margin-left: 0;
  padding-left: 24px;
}
li {
  margin-top: 4px;
  margin-bottom: 4px;
}
pre {
  color: %4;
  background-color: %5;
  border: 1px solid %6;
  border-radius: 6px;
  font-family: Consolas, "Courier New", monospace;
  margin-top: 12px;
  margin-bottom: 14px;
  padding: 11px 12px;
  white-space: pre-wrap;
  line-height: 1.42;
}
code {
  color: %4;
  background-color: %5;
  border-radius: 4px;
  font-family: Consolas, "Courier New", monospace;
  padding-left: 5px;
  padding-right: 5px;
}
blockquote {
  color: %2;
  background-color: %8;
  border-left: 3px solid %7;
  margin: 10px 0;
  padding: 8px 11px;
}
hr {
  color: %6;
  background-color: %6;
  height: 1px;
  border: none;
  margin-top: 14px;
  margin-bottom: 14px;
}
table {
  border-collapse: collapse;
  margin-top: 10px;
  margin-bottom: 12px;
}
th, td {
  border: 1px solid %6;
  padding: 6px 9px;
}
th {
  background-color: %5;
}
)")
      .arg(text, muted, link, codeText, codeBackground, codeBorder, quoteBorder,
           quoteBackground);
}

} // namespace

void MarkdownRenderer::rememberRenderedBlocks(
    const QString &key, const QVariantList &blocks) const {
  if (blockCache_.contains(key)) {
    return;
  }
  blockCache_.insert(key, blocks);
  blockCacheOrder_.append(key);
}
