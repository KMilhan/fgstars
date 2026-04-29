/*
    Star Studio target-finder UI journey tests

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "test_starstudio_targetfinder.h"

#if defined(HAVE_INDI)

#include "kstars_ui_tests.h"
#include "test_ekos.h"
#include "test_ekos_simulator.h"
#include "ekos/align/align.h"
#include "ekos/capture/capture.h"
#include "ekos/capture/capturepreviewwidget.h"
#include "ekos/manager.h"
#include "ekos/mount/mount.h"
#include "ekos/workspacesession.h"

#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>

namespace
{
QRect managerRelativeGeometry(const QWidget *widget, const QWidget *manager)
{
    if (widget == nullptr || manager == nullptr)
        return {};

    return QRect(widget->mapTo(const_cast<QWidget *>(manager), QPoint(0, 0)), widget->size());
}

bool textFits(const QWidget *widget, const QString &text)
{
    if (widget == nullptr || text.isEmpty())
        return true;

    const int textWidth = widget->fontMetrics().horizontalAdvance(text);
    return textWidth + 18 <= widget->width();
}
}

TestStarStudioTargetFinder::TestStarStudioTargetFinder(QObject *parent) : QObject(parent)
{
}

void TestStarStudioTargetFinder::initTestCase()
{
    KVERIFY_EKOS_IS_HIDDEN();
    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
    KTRY_EKOS_START_SIMULATORS();
    KHACK_RESET_EKOS_TIME();
}

void TestStarStudioTargetFinder::cleanupTestCase()
{
    KTRY_EKOS_STOP_SIMULATORS();
    KTRY_CLOSE_EKOS();
    KVERIFY_EKOS_IS_HIDDEN();
}

void TestStarStudioTargetFinder::init()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    QTRY_VERIFY_WITH_TIMEOUT(manager->captureModule() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(manager->mountModule() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(manager->alignModule() != nullptr, 5000);

    KTRY_EKOS_GADGET(QTabWidget, toolsWidget);
    toolsWidget->setCurrentWidget(manager->captureModule());
    QTRY_COMPARE_WITH_TIMEOUT(toolsWidget->currentWidget(), manager->captureModule(), 1000);

    auto * const mount = manager->mountModule();
    if (mount->parkStatus() == ISD::PARK_PARKED)
        mount->unpark();
    QTRY_VERIFY_WITH_TIMEOUT(mount->parkStatus() == ISD::PARK_UNPARKED, 30000);
}

void TestStarStudioTargetFinder::testTargetFinderSelectsImagingTarget()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    KTRY_EKOS_GADGET(QTabWidget, toolsWidget);
    KTRY_EKOS_GADGET(QPushButton, globalSearchB);
    KTRY_EKOS_GADGET(QPushButton, goToTargetB);
    KTRY_EKOS_GADGET(QPushButton, centerSelectedTargetB);
    KTRY_EKOS_GADGET(QPushButton, addTargetPlanB);
    KTRY_EKOS_GADGET(QPushButton, openSkyMapB);
    KTRY_EKOS_GADGET(QPushButton, advancedEkosB);
    KTRY_EKOS_GADGET(QLabel, starStudioTargetStatusL);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);

    QCOMPARE(toolsWidget->currentWidget(), manager->captureModule());
    QVERIFY(globalSearchB->text().contains("Find Target") || globalSearchB->text().contains("Target:"));
    QVERIFY(QString(openSkyMapB->text()).remove(QLatin1Char('&')).contains("Map"));
    QVERIFY(advancedEkosB->text().contains("Advanced"));
    QVERIFY(!advancedEkosB->isChecked());
    QCOMPARE(starStudioTargetStatusL->text(), QString("No target"));
    QCOMPARE(starStudioCenterStatusL->text(), QString("Center idle"));

    QVERIFY(manager->selectStarStudioTarget("Altair"));

    QTRY_VERIFY_WITH_TIMEOUT(globalSearchB->text().contains("Altair"), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(starStudioTargetStatusL->text().contains("Alt"), 5000);
    auto * const session = manager->workspaceSession();
    QVERIFY(session != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(session->targetContext().has_value(), 5000);
    QCOMPARE(session->targetContext()->name, QString("Altair"));
    QCOMPARE(session->targetContext()->framingReady, session->targetContext()->visible);
    QVERIFY(goToTargetB->isEnabled());
    QVERIFY(centerSelectedTargetB->isEnabled());
    QVERIFY(addTargetPlanB->isEnabled());
    QCOMPARE(toolsWidget->currentWidget(), manager->captureModule());

    QTest::mouseClick(goToTargetB, Qt::LeftButton);
    QTRY_VERIFY_WITH_TIMEOUT(manager->mountModule()->slewStatus() != IPState::IPS_BUSY, 30000);

    auto * const targetNameT = manager->captureModule()->findChild<QLineEdit *>("targetNameT");
    QVERIFY(targetNameT != nullptr);
    QTRY_COMPARE_WITH_TIMEOUT(targetNameT->text(), QString("Altair"), 30000);
}

void TestStarStudioTargetFinder::testCenterTargetStartsFromCaptureContext()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    KTRY_EKOS_GADGET(QTabWidget, toolsWidget);
    KTRY_EKOS_GADGET(QPushButton, centerSelectedTargetB);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);

    QVERIFY(manager->selectStarStudioTarget("Polaris"));
    QCOMPARE(toolsWidget->currentWidget(), manager->captureModule());
    QVERIFY(centerSelectedTargetB->isEnabled());

    auto * const session = manager->workspaceSession();
    QVERIFY(session != nullptr);

    QTest::mouseClick(centerSelectedTargetB, Qt::LeftButton);
    QTRY_VERIFY_WITH_TIMEOUT(starStudioCenterStatusL->text().contains("Center"), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(session->centerTargetState() == Ekos::WorkspaceSession::CenterTargetState::Running, 5000);

    manager->alignModule()->abort();
    QTRY_VERIFY_WITH_TIMEOUT(session->centerTargetState() == Ekos::WorkspaceSession::CenterTargetState::Aborted
                             || session->centerTargetState() == Ekos::WorkspaceSession::CenterTargetState::Idle, 10000);
}

void TestStarStudioTargetFinder::testShellKeepsImageWorkspacePrimary()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    QVERIFY(manager->selectStarStudioTarget("Altair"));

    KTRY_EKOS_GADGET(QSplitter, deviceSplitter);
    KTRY_EKOS_GADGET(QTabWidget, toolsWidget);
    KTRY_EKOS_GADGET(CapturePreviewWidget, capturePreview);
    KTRY_EKOS_GADGET(QPushButton, globalSearchB);
    KTRY_EKOS_GADGET(QPushButton, goToTargetB);
    KTRY_EKOS_GADGET(QPushButton, centerSelectedTargetB);
    KTRY_EKOS_GADGET(QPushButton, addTargetPlanB);
    KTRY_EKOS_GADGET(QPushButton, openSkyMapB);
    KTRY_EKOS_GADGET(QPushButton, advancedEkosB);
    KTRY_EKOS_GADGET(QLabel, starStudioTargetStatusL);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);
    KTRY_EKOS_GADGET(QWidget, layoutWidget);
    KTRY_EKOS_GADGET(QWidget, rightLayoutWidget);

    const QList<QWidget *> headerWidgets
    {
        globalSearchB,
        goToTargetB,
        centerSelectedTargetB,
        addTargetPlanB,
        openSkyMapB,
        advancedEkosB,
        starStudioTargetStatusL,
        starStudioCenterStatusL,
    };

    QDir artifacts(QStringLiteral(".artifacts/star-studio-shell"));
    QVERIFY(artifacts.mkpath(QStringLiteral(".")));

    const QList<QSize> supportedSizes
    {
        QSize(1280, 860),
        QSize(980, 720),
        QSize(760, 720),
    };

    for (const QSize &size : supportedSizes)
    {
        manager->resize(size);
        QTRY_VERIFY_WITH_TIMEOUT(manager->width() >= size.width(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(manager->height() >= size.height(), 1000);
        QTest::qWait(250);

        QCOMPARE(toolsWidget->currentWidget(), manager->captureModule());
        QVERIFY(!toolsWidget->isVisibleTo(manager));
        QVERIFY(!layoutWidget->isVisibleTo(manager));
        QVERIFY(!rightLayoutWidget->isVisibleTo(manager));
        QVERIFY(capturePreview->isVisibleTo(manager));
        QVERIFY(capturePreview->summaryFITSView() != nullptr);
        QVERIFY(capturePreview->summaryFITSView()->isVisibleTo(manager));
        QVERIFY(deviceSplitter->sizes().size() == 2);
        QVERIFY2(deviceSplitter->sizes()[0] > deviceSplitter->sizes()[1],
                 "Star Studio image workspace should remain larger than the contextual control panel.");

        for (const QWidget *widget : headerWidgets)
        {
            QVERIFY(widget->isVisibleTo(manager));
            QVERIFY2(widget->width() > 0 && widget->height() > 0,
                     qPrintable(QStringLiteral("Header widget %1 has invalid geometry.").arg(widget->objectName())));
        }

        QVERIFY(textFits(globalSearchB, globalSearchB->text()));
        QVERIFY(textFits(goToTargetB, goToTargetB->text()));
        QVERIFY(textFits(centerSelectedTargetB, centerSelectedTargetB->text()));
        QVERIFY(textFits(addTargetPlanB, addTargetPlanB->text()));
        QVERIFY(textFits(openSkyMapB, openSkyMapB->text()));
        QVERIFY(textFits(advancedEkosB, advancedEkosB->text()));
        QVERIFY(textFits(starStudioTargetStatusL, starStudioTargetStatusL->text()));
        QVERIFY(textFits(starStudioCenterStatusL, starStudioCenterStatusL->text()));

        for (int index = 1; index < headerWidgets.size(); ++index)
        {
            const QRect previous = managerRelativeGeometry(headerWidgets[index - 1], manager);
            const QRect current = managerRelativeGeometry(headerWidgets[index], manager);
            QVERIFY2(previous.right() <= current.left(),
                     qPrintable(QStringLiteral("Star Studio header widgets overlap at %1x%2.")
                                .arg(size.width()).arg(size.height())));
        }

        const QString screenshotPath = artifacts.filePath(QStringLiteral("star-studio-shell-%1x%2.png")
                                       .arg(size.width()).arg(size.height()));
        QVERIFY(manager->grab().save(screenshotPath));
    }
}

void TestStarStudioTargetFinder::testCenterTargetRequiresSelectedTarget()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    KTRY_EKOS_GADGET(CapturePreviewWidget, capturePreview);
    KTRY_EKOS_GADGET(QPushButton, centerSelectedTargetB);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);

    QVERIFY(!centerSelectedTargetB->isEnabled());

    auto * const session = manager->workspaceSession();
    QVERIFY(session != nullptr);
    auto * const embeddedCenterTargetB = capturePreview->findChild<QPushButton *>(QStringLiteral("centerTargetB"));
    QVERIFY(embeddedCenterTargetB != nullptr);
    QVERIFY(!embeddedCenterTargetB->isEnabled());
    QCOMPARE(starStudioCenterStatusL->text(), QString("Center idle"));
    QCOMPARE(session->centerTargetState(), Ekos::WorkspaceSession::CenterTargetState::Idle);
}

void TestStarStudioTargetFinder::testCenterTargetRejectsTargetBelowHorizon()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    KTRY_EKOS_GADGET(QPushButton, centerSelectedTargetB);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);

    QVERIFY(manager->selectStarStudioTarget("Canopus"));

    auto * const session = manager->workspaceSession();
    QVERIFY(session != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(session->targetContext().has_value(), 5000);
    QVERIFY(!session->targetContext()->visible);
    QVERIFY(centerSelectedTargetB->isEnabled());

    QTest::mouseClick(centerSelectedTargetB, Qt::LeftButton);
    QTRY_COMPARE_WITH_TIMEOUT(starStudioCenterStatusL->text(), QString("Target too low"), 5000);
    QCOMPARE(session->centerTargetState(), Ekos::WorkspaceSession::CenterTargetState::TargetNotVisible);
}

QTEST_KSTARS_MAIN(TestStarStudioTargetFinder)

#endif // HAVE_INDI
