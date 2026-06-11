#include "core/AppInitializer.h"

#include "controller/DecompilerController.h"
#include "controller/AgentKnowledgeController.h"
#include "controller/AgentController.h"
#include "controller/LanguageController.h"
#include "controller/SettingsController.h"
#include "controller/UpdateController.h"
#include "core/ApplicationController.h"
#include "core/BuildInfoProvider.h"
#include "core/ResourcePreviewProvider.h"
#include "core/WindowChrome.h"
#include "model/RecentFilesModel.h"

#include <QIcon>
#include <QObject>
#include <QQmlContext>
#include <QQuickStyle>

#include <utility>

AppInitializer::AppInitializer(QGuiApplication& app, QQmlApplicationEngine& engine, QString initialFileUrl)
    : app_(app)
    , engine_(engine)
    , initialFileUrl_(std::move(initialFileUrl))
{
}

void AppInitializer::initializeAll()
{
    initializeApplication();
    initializeContext();
    initializeQmlModules();
}

void AppInitializer::autoDisableDebugOutput()
{
#ifdef QT_NO_DEBUG_OUTPUT
    qputenv("QT_LOGGING_RULES", QByteArray("qml=false"));
#endif
}

void AppInitializer::initializeApplication()
{
    QGuiApplication::setApplicationName(QStringLiteral("ReArk"));
    QGuiApplication::setOrganizationName(QStringLiteral("ReArk"));
    QGuiApplication::setApplicationVersion(QStringLiteral(REARK_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/images/app_icon.ico")));
    QQuickStyle::setStyle(QStringLiteral("Material"));
}

void AppInitializer::initializeContext()
{
    resourcePreviewProvider_ = new ResourcePreviewProvider();
    engine_.addImageProvider(QStringLiteral("rearkResources"), resourcePreviewProvider_);
    applicationController_ = new ApplicationController(&engine_);
    decompilerController_ = new DecompilerController(resourcePreviewProvider_, &engine_);
    agentKnowledgeController_ = new AgentKnowledgeController(&engine_);
    agentController_ = new AgentController(decompilerController_, agentKnowledgeController_, &engine_);
    languageController_ = new LanguageController(&engine_, &engine_);
    buildInfoProvider_ = new BuildInfoProvider(languageController_, &engine_);
    recentFilesModel_ = new RecentFilesModel(&engine_);
    settingsController_ = new SettingsController(&engine_);
    updateController_ = new UpdateController(&engine_);
    windowChrome_ = new WindowChrome(&engine_);

    QObject::connect(
        settingsController_,
        &SettingsController::agentKnowledgeSettingsChanged,
        agentKnowledgeController_,
        &AgentKnowledgeController::clearSessionReferences);

    auto* context = engine_.rootContext();
    context->setContextProperty(QStringLiteral("appVersion"), QStringLiteral(REARK_VERSION));
    context->setContextProperty(QStringLiteral("applicationController"), applicationController_);
    context->setContextProperty(QStringLiteral("buildInfo"), buildInfoProvider_);
    context->setContextProperty(QStringLiteral("recentFilesModel"), recentFilesModel_);
    context->setContextProperty(QStringLiteral("initialFileUrl"), initialFileUrl_);
    context->setContextProperty(QStringLiteral("decompilerController"), decompilerController_);
    context->setContextProperty(QStringLiteral("rearkAgentController"), agentController_);
    context->setContextProperty(QStringLiteral("rearkAgentKnowledgeController"), agentKnowledgeController_);
    context->setContextProperty(QStringLiteral("languageController"), languageController_);
    context->setContextProperty(QStringLiteral("rearkSettingsController"), settingsController_);
    context->setContextProperty(QStringLiteral("updateController"), updateController_);
    context->setContextProperty(QStringLiteral("windowChrome"), windowChrome_);
}

void AppInitializer::initializeQmlModules()
{
    engine_.addImportPath(QStringLiteral("qrc:/"));
}
