/*
    SPDX-FileCopyrightText: 2017 Csaba Kertesz <csaba.kertesz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kstars_ui_tests.h"

#include "kswizard.h"
#include "config-kstars.h"
#include "auxiliary/kspaths.h"
#include "test_kstars_startup.h"

#if defined(HAVE_INDI)
#include "test_ekos.h"
#include "test_ekos_simulator.h"
#include "test_ekos_focus.h"
#include "test_ekos_guide.h"
#include "ekos/manager.h"
#include "ekos/profileeditor.h"
#include "ekos/profilewizard.h"
#endif

#include <KActionCollection>
#include <Options.h>

#include <QFuture>
#include <QtConcurrentRun>

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtTest/QTest>
#else
#include <QTest>
#endif

#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>

#include <ctime>
#include <unistd.h>

QSystemTrayIcon * KStarsUiTests::m_Notifier { nullptr };

namespace
{
QtMessageHandler previousUiTestMessageHandler = nullptr;
bool uiTestMessageHandlerInstalled = false;

QString existingPathFromEnv(const char *name)
{
    const QString path = qEnvironmentVariable(name);
    return QFileInfo::exists(path) ? path : QString{};
}

bool isKnownUiTestNoise(const QString &message)
{
    return message.contains("Pass a QTimeZone instead of Qt::TimeZone")
           || message.startsWith("QLayout: Attempting to add QLayout \"\" to MinimizeWidget")
           || (message.contains("QObject::connect(") && message.contains("invalid nullptr parameter"));
}

void suppressKnownUiTestNoise(QtMsgType type, const QMessageLogContext &ctx, const QString &message)
{
    if (type == QtWarningMsg && isKnownUiTestNoise(message))
        return;

    if (previousUiTestMessageHandler != nullptr)
        previousUiTestMessageHandler(type, ctx, message);
}
}

void install_ui_test_message_handler()
{
    if (uiTestMessageHandlerInstalled)
        return;

    previousUiTestMessageHandler = qInstallMessageHandler(suppressKnownUiTestNoise);
    uiTestMessageHandlerInstalled = true;
}

void restore_ui_test_message_handler()
{
    if (!uiTestMessageHandlerInstalled)
        return;

    qInstallMessageHandler(previousUiTestMessageHandler);
    previousUiTestMessageHandler = nullptr;
    uiTestMessageHandlerInstalled = false;
}

QString KStarsUiTests::indiServerPath()
{
    const QString fromEnv = existingPathFromEnv("KSTARS_TEST_INDI_SERVER");
    if (!fromEnv.isEmpty())
        return fromEnv;

#ifdef KSTARS_TEST_VENDORED_INDI_SERVER
    if (QFileInfo::exists(QStringLiteral(KSTARS_TEST_VENDORED_INDI_SERVER)))
        return QStringLiteral(KSTARS_TEST_VENDORED_INDI_SERVER);
#endif

    const QString fromPath = QStandardPaths::findExecutable("indiserver");
    if (!fromPath.isEmpty())
        return fromPath;

    const QString localPath = QStringLiteral("/usr/local/bin/indiserver");
    if (QFileInfo::exists(localPath))
        return localPath;

    const QString systemPath = QStringLiteral("/usr/bin/indiserver");
    return QFileInfo::exists(systemPath) ? systemPath : QString{};
}

QString KStarsUiTests::indiDriversDir()
{
    const QString fromEnv = existingPathFromEnv("KSTARS_TEST_INDI_DRIVERS_DIR");
    if (!fromEnv.isEmpty())
        return fromEnv;

#ifdef KSTARS_TEST_VENDORED_INDI_DRIVERS_DIR
    if (QFileInfo::exists(QStringLiteral(KSTARS_TEST_VENDORED_INDI_DRIVERS_DIR) + QStringLiteral("/drivers.xml")))
        return QStringLiteral(KSTARS_TEST_VENDORED_INDI_DRIVERS_DIR);
#endif

    const QString dataDir = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "indi", QStandardPaths::LocateDirectory);
    if (!dataDir.isEmpty())
        return dataDir;

    return {};
}

bool KStarsUiTests::configureTestIndiRuntime()
{
    const QString serverPath = indiServerPath();
    if (serverPath.isEmpty())
        return false;

    Options::setIndiServerIsInternal(false);
    Options::setIndiServer(serverPath);

    const QString driversPath = indiDriversDir();
    if (!driversPath.isEmpty())
    {
        Options::setIndiDriversAreInternal(false);
        Options::setIndiDriversDir(driversPath);
    }

    return true;
}

void KStarsUiTests::notifierBegin()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    if (KStars::Instance() == nullptr)
        return;
    if (!m_Notifier)
        m_Notifier = new QSystemTrayIcon(QIcon::fromTheme("kstars_stars"), KStars::Instance());
    if (m_Notifier)
        KStarsUiTests::m_Notifier->show();
}

void KStarsUiTests::notifierHide()
{
    if (m_Notifier)
        m_Notifier->hide();
}

void KStarsUiTests::notifierMessage(QString title, QString message)
{
    qDebug() << message.replace('\n', ' ');
    if (m_Notifier)
        m_Notifier->showMessage(title, message, QIcon());
}

void KStarsUiTests::notifierEnd()
{
    if (m_Notifier)
    {
        delete m_Notifier;
        m_Notifier = nullptr;
    }
}

// We want to launch the application before running our tests
// Thus we want to explicitly call QApplication::exec(), and run our tests in parallel of the event loop
// We then reimplement QTEST_MAIN(KStarsUiTests);
// The same will have to be done when interacting with a modal dialog: exec() in main thread, tests in timer-based thread

void prepare_tests()
{
    // Configure our test UI
    QApplication::instance()->setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_SET_MAIN_SOURCE_PATH
    QApplication::processEvents();

    const bool hasIndiRuntime = KStarsUiTests::configureTestIndiRuntime();
    if (!hasIndiRuntime)
        qWarning() << "INDI runtime not configured: indiserver path not found for UI tests.";

    // Prepare our KStars configuration
    const QByteArray seedEnv = qgetenv("KSTARS_TEST_RANDOM_SEED");
    bool seedOk = false;
    unsigned int seed = seedEnv.isEmpty() ? 1u : seedEnv.toUInt(&seedOk);
    if (!seedOk && !seedEnv.isEmpty())
        seed = 1u;
    srand(seed);
    KSPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    KSPaths::writableLocation(QStandardPaths::AppConfigLocation);
    KSPaths::writableLocation(QStandardPaths::CacheLocation);

    // Explicitly provide the RC file from the main app resources, not the user-customized one
    KStars::setResourceFile(":/kxmlgui5/kstars/kstarsui.rc");
}

int run_wizards(int argc, char *argv[])
{
    int failure = 0;

    // This cleans the test user settings, creates our instance and manages the startup wizard
    if (!failure)
    {
        TestKStarsStartup * ti = new TestKStarsStartup();
        failure |= QTest::qExec(ti);//, argc, argv);
        delete ti;
    }

    Q_UNUSED(argc);
    Q_UNUSED(argv);

    return failure;
}

void execute_tests()
{
    QCoreApplication *app = QApplication::instance();

    // Limit execution duration
    QTimer::singleShot(60 * 60 * 1000, app, &QCoreApplication::quit);

    app->exec();

    // Clean our instance up if it is still alive
    if( KStars::Instance() != nullptr)
    {
        KStars::Instance()->close();
        delete KStars::Instance();
    }
}

#if !defined(HAVE_INDI)
QTEST_KSTARS_MAIN(KStarsUiTests)
#endif
