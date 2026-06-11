#ifndef REARK_APP_INITIALIZER_H
#define REARK_APP_INITIALIZER_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

class BuildInfoProvider;
class ApplicationController;
class AgentController;
class AgentKnowledgeController;
class DecompilerController;
class LanguageController;
class RecentFilesModel;
class ResourcePreviewProvider;
class SettingsController;
class UpdateController;
class WindowChrome;

class AppInitializer {
public:
    AppInitializer(QGuiApplication& app, QQmlApplicationEngine& engine, QString initialFileUrl);

    void initializeAll();
    static void autoDisableDebugOutput();

private:
    void initializeApplication();
    void initializeContext();
    void initializeQmlModules();

    QGuiApplication& app_;
    QQmlApplicationEngine& engine_;
    QString initialFileUrl_;
    ApplicationController* applicationController_ = nullptr;
    AgentKnowledgeController* agentKnowledgeController_ = nullptr;
    AgentController* agentController_ = nullptr;
    BuildInfoProvider* buildInfoProvider_ = nullptr;
    ResourcePreviewProvider* resourcePreviewProvider_ = nullptr;
    RecentFilesModel* recentFilesModel_ = nullptr;
    DecompilerController* decompilerController_ = nullptr;
    LanguageController* languageController_ = nullptr;
    SettingsController* settingsController_ = nullptr;
    UpdateController* updateController_ = nullptr;
    WindowChrome* windowChrome_ = nullptr;
};

#endif // REARK_APP_INITIALIZER_H
