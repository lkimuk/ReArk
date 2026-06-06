#ifndef REARK_DECOMPILER_CONTROLLER_H
#define REARK_DECOMPILER_CONTROLLER_H

#include "core/HyleDecompiler.h"
#include "model/HexDocumentModel.h"
#include "model/OpenFileTabsModel.h"
#include "model/SourceTreeModel.h"

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>
#include <deque>
#include <set>

class ResourcePreviewProvider;

class DecompilerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(SourceTreeModel* treeModel READ treeModel CONSTANT)
    Q_PROPERTY(OpenFileTabsModel* tabsModel READ tabsModel CONSTANT)
    Q_PROPERTY(HexDocumentModel* hexModel READ hexModel CONSTANT)
    Q_PROPERTY(QString selectedContent READ selectedContent NOTIFY selectedContentChanged)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectedNameChanged)
    Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

public:
    explicit DecompilerController(ResourcePreviewProvider* previewProvider, QObject* parent = nullptr);

    [[nodiscard]] SourceTreeModel* treeModel();
    [[nodiscard]] OpenFileTabsModel* tabsModel();
    [[nodiscard]] HexDocumentModel* hexModel();
    [[nodiscard]] QString selectedContent() const;
    [[nodiscard]] QString selectedName() const;
    [[nodiscard]] QString diagnostics() const;
    [[nodiscard]] QString status() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] int selectedIndex() const;

    Q_INVOKABLE void decompileFile(const QString& filePath);
    Q_INVOKABLE void activateIndex(int index);
    Q_INVOKABLE void openActivePreviewFile() const;
    Q_INVOKABLE QVariantList quickOpenCandidates(const QString& query) const;
    Q_INVOKABLE QVariantList searchCandidates(const QString& query) const;
    Q_INVOKABLE QVariantList entryPointCandidates() const;
    Q_INVOKABLE void navigateToNode(int nodeIndex);
    Q_INVOKABLE QString formatJson(const QString& content) const;
    Q_INVOKABLE void copyTextToClipboard(const QString& text) const;
    Q_INVOKABLE void clear();

public slots:
    void setSelectedIndex(int index);

signals:
    void selectedContentChanged();
    void selectedNameChanged();
    void diagnosticsChanged();
    void statusChanged();
    void busyChanged();
    void selectedIndexChanged();

private:
    void setStatus(const QString& status);
    void setBusy(bool busy);
    void applyOpenResult(quint64 requestId, HyleDecompiler::OpenResult result);
    void applySourceResult(quint64 requestId, HyleDecompiler::SourceResult result);
    void openFileTab(int nodeIndex);
    void startNodeLoad(int nodeIndex, bool foreground);
    void resetLoadingState();
    void rebuildBackgroundPreloadQueue(int centerNode);
    void startNextBackgroundPreloads();
    void refreshActiveHexDocument();

    SourceTreeModel treeModel_;
    OpenFileTabsModel tabsModel_;
    HexDocumentModel hexModel_;
    ResourcePreviewProvider* previewProvider_ = nullptr;
    std::shared_ptr<HyleDecompiler::SessionContext> packageContext_;
    QString packagePath_;
    std::set<int> foregroundLoadingNodes_;
    std::set<int> backgroundLoadingNodes_;
    std::set<int> backgroundSkippedNodes_;
    std::deque<int> backgroundPreloadQueue_;
    int activeBackgroundPreloads_ = 0;
    QString status_ = tr("Ready");
    bool busy_ = false;
    quint64 openRequestId_ = 0;
};

#endif // REARK_DECOMPILER_CONTROLLER_H
