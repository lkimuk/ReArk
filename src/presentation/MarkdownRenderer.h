#ifndef REARK_MARKDOWN_RENDERER_H
#define REARK_MARKDOWN_RENDERER_H

#include <QObject>
#include <QHash>
#include <QVariantList>
#include <QString>

class MarkdownRenderer : public QObject {
    Q_OBJECT

public:
    explicit MarkdownRenderer(QObject* parent = nullptr);

    Q_INVOKABLE QVariantList renderBlocks(const QString& markdown, bool darkTheme) const;
    Q_INVOKABLE int renderBlocksAsync(const QString& markdown, bool darkTheme);

signals:
    void blocksReady(int requestId, const QVariantList& blocks);

private:
    void rememberRenderedBlocks(const QString& key, const QVariantList& blocks) const;

    mutable QHash<QString, QVariantList> blockCache_;
    mutable QStringList blockCacheOrder_;
    int nextRequestId_ = 0;
};

#endif // REARK_MARKDOWN_RENDERER_H
