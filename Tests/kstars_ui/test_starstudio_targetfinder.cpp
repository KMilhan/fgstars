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

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QSpinBox>
#include <QTabWidget>

#include <algorithm>
#include <cmath>

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
    KTRY_EKOS_GADGET(QLabel, workspaceTitleL);
    KTRY_EKOS_GADGET(QLabel, starStudioTargetStatusL);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);

    QCOMPARE(toolsWidget->currentWidget(), manager->captureModule());
    QVERIFY(globalSearchB->text().contains("Find Target"));
    QVERIFY(QString(openSkyMapB->text()).remove(QLatin1Char('&')).contains("Map"));
    QVERIFY(advancedEkosB->text().contains("Advanced"));
    QVERIFY(!advancedEkosB->isChecked());
    QCOMPARE(workspaceTitleL->text(), QString("Image Workspace"));
    QCOMPARE(starStudioTargetStatusL->text(), QString("No target"));
    QCOMPARE(starStudioCenterStatusL->text(), QString("Center idle"));

    QVERIFY(manager->selectStarStudioTarget("Altair"));

    QTRY_VERIFY_WITH_TIMEOUT(globalSearchB->text().contains("Find Target"), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(starStudioTargetStatusL->text().contains("Altair"), 5000);
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
    KTRY_EKOS_GADGET(QLabel, workspaceTitleL);
    KTRY_EKOS_GADGET(QLabel, starStudioTargetStatusL);
    KTRY_EKOS_GADGET(QLabel, starStudioCenterStatusL);
    KTRY_EKOS_GADGET(QWidget, layoutWidget);
    KTRY_EKOS_GADGET(QWidget, rightLayoutWidget);
    KTRY_EKOS_GADGET(QWidget, essentialSimulatorPanel);

    const QList<QWidget *> headerWidgets
    {
        workspaceTitleL,
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
        QVERIFY(rightLayoutWidget->isVisibleTo(manager));
        QVERIFY(essentialSimulatorPanel->isVisibleTo(manager));
        QVERIFY(capturePreview->isVisibleTo(manager));
        QVERIFY(capturePreview->findChild<QWidget *>(QStringLiteral("essentialSimulatorPanel")) == nullptr);
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
        QVERIFY(textFits(workspaceTitleL, workspaceTitleL->text()));
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

void TestStarStudioTargetFinder::testEssentialSimulatorControlsRespond()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    KTRY_EKOS_GADGET(CapturePreviewWidget, capturePreview);

    auto * const panel = manager->findChild<QWidget *>(QStringLiteral("essentialSimulatorPanel"));
    auto * const title = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorTitleL"));
    auto * const cameraState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorCameraStateL"));
    auto * const cooler = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorCoolerCB"));
    auto * const targetTemp = manager->findChild<QDoubleSpinBox *>(QStringLiteral("essentialSimulatorCameraTargetTempSB"));
    auto * const gain = manager->findChild<QSpinBox *>(QStringLiteral("essentialSimulatorGainSB"));
    auto * const focus = manager->findChild<QSpinBox *>(QStringLiteral("essentialSimulatorFocusSB"));
    auto * const cameraStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorCameraStatusL"));
    auto * const mountState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorMountStateL"));
    auto * const tracking = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorTrackingCB"));
    auto * const mountMode = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorMountModeCB"));
    auto * const mountStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorMountStatusL"));
    auto * const train = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorTrainCB"));
    auto * const trainStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorTrainStatusL"));
    auto * const siteState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorSiteStateL"));
    auto * const gpsd = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorGpsdCB"));
    auto * const gpsdStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGpsdStatusL"));
    auto * const guideState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGuideStateL"));
    auto * const guide = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorGuideCB"));
    auto * const guideMode = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorGuideModeCB"));
    auto * const guideStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGuideStatusL"));

    QVERIFY(panel != nullptr);
    QVERIFY(title != nullptr);
    QVERIFY(cameraState != nullptr);
    QVERIFY(cooler != nullptr);
    QVERIFY(targetTemp != nullptr);
    QVERIFY(gain != nullptr);
    QVERIFY(focus != nullptr);
    QVERIFY(cameraStatus != nullptr);
    QVERIFY(mountState != nullptr);
    QVERIFY(tracking != nullptr);
    QVERIFY(mountMode != nullptr);
    QVERIFY(mountStatus != nullptr);
    QVERIFY(train != nullptr);
    QVERIFY(trainStatus != nullptr);
    QVERIFY(siteState != nullptr);
    QVERIFY(gpsd != nullptr);
    QVERIFY(gpsdStatus != nullptr);
    QVERIFY(guideState != nullptr);
    QVERIFY(guide != nullptr);
    QVERIFY(guideMode != nullptr);
    QVERIFY(guideStatus != nullptr);
    QVERIFY(panel->isVisibleTo(manager));
    QVERIFY(capturePreview->findChild<QWidget *>(QStringLiteral("essentialSimulatorPanel")) == nullptr);
    QCOMPARE(panel->accessibleName(), QString("Star Studio Equipment"));
    QCOMPARE(title->text(), QString("Equipment"));
    QVERIFY(!gpsd->text().contains(QStringLiteral("GPSD")));
    QVERIFY(!guide->text().contains(QStringLiteral("PHD2")));

    cooler->setChecked(true);
    targetTemp->setValue(-10.0);
    gain->setValue(20);
    focus->setValue(80);
    QTRY_COMPARE_WITH_TIMEOUT(cameraState->text(), QString("Camera cooling"), 1000);
    QVERIFY(cameraStatus->text().contains(QStringLiteral("Sensor -9.4 C")));
    QVERIFY(cameraStatus->text().contains(QStringLiteral("target -10.0 C")));
    QVERIFY(cameraStatus->text().contains(QStringLiteral("cooler 76 percent")));
    QVERIFY(cameraStatus->text().contains(QStringLiteral("noise 3.6")));
    QVERIFY(cameraStatus->text().contains(QStringLiteral("focus blur 1.2")));
    cooler->setChecked(false);
    QTRY_COMPARE_WITH_TIMEOUT(cameraState->text(), QString("Cooling off"), 1000);
    QVERIFY(!targetTemp->isEnabled());
    QVERIFY(cameraStatus->text().contains(QStringLiteral("Sensor 19.0 C")));
    QVERIFY(cameraStatus->text().contains(QStringLiteral("cooling off")));

    tracking->setChecked(false);
    mountMode->setCurrentText(QStringLiteral("Move"));
    QTRY_COMPARE_WITH_TIMEOUT(mountState->text(), QString("Tracking off"), 1000);
    QVERIFY(tracking->isEnabled());
    QVERIFY(mountStatus->text().contains(QStringLiteral("Manual")));
    QVERIFY(mountStatus->text().contains(QStringLiteral("8.0")));
    mountMode->setCurrentText(QStringLiteral("Parked"));
    QTRY_COMPARE_WITH_TIMEOUT(mountState->text(), QString("Parked"), 1000);
    QVERIFY(!tracking->isEnabled());
    QCOMPARE(mountStatus->text(), QString("Tracking unavailable"));

    train->setCurrentText(QStringLiteral("Guide Port"));
    QTRY_VERIFY_WITH_TIMEOUT(trainStatus->text().contains(QStringLiteral("Guide port")), 1000);
    QVERIFY(trainStatus->text().contains(QStringLiteral("pulse corrections")));
    train->setCurrentText(QStringLiteral("Site Source"));
    QTRY_VERIFY_WITH_TIMEOUT(trainStatus->text().contains(QStringLiteral("position and time")), 1000);

    gpsd->setChecked(false);
    QTRY_COMPARE_WITH_TIMEOUT(siteState->text(), QString("Manual site"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(gpsdStatus->text().contains(QStringLiteral("Manual coordinates")), 1000);
    gpsd->setChecked(true);
    QTRY_COMPARE_WITH_TIMEOUT(siteState->text(), QString("GPSD fix"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(gpsdStatus->text().contains(QStringLiteral("Resolved site")), 1000);

    guide->setChecked(true);
    QVERIFY(guideMode->isEnabled());
    guideMode->setCurrentText(QStringLiteral("Dither"));
    QTRY_COMPARE_WITH_TIMEOUT(guideState->text(), QString("Dithering"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(guideStatus->text().contains(QStringLiteral("Dither")), 1000);
    QVERIFY(guideStatus->text().contains(QStringLiteral("0.72")));
    guide->setChecked(false);
    QTRY_COMPARE_WITH_TIMEOUT(guideState->text(), QString("Idle"), 1000);
    QVERIFY(!guideMode->isEnabled());
    QCOMPARE(guideStatus->text(), QString("No guiding input"));

    QDir artifacts(QStringLiteral(".artifacts/star-studio-essential-simulator"));
    QVERIFY(artifacts.mkpath(QStringLiteral(".")));
    QVERIFY(manager->grab().save(artifacts.filePath(QStringLiteral("essential-simulator.png"))));
}

void TestStarStudioTargetFinder::testEssentialSimulatorAllCombinations()
{
    auto * const manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    auto * const gain = manager->findChild<QSpinBox *>(QStringLiteral("essentialSimulatorGainSB"));
    auto * const cameraState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorCameraStateL"));
    auto * const cooler = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorCoolerCB"));
    auto * const targetTemp = manager->findChild<QDoubleSpinBox *>(QStringLiteral("essentialSimulatorCameraTargetTempSB"));
    auto * const focus = manager->findChild<QSpinBox *>(QStringLiteral("essentialSimulatorFocusSB"));
    auto * const cameraStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorCameraStatusL"));
    auto * const tracking = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorTrackingCB"));
    auto * const mountState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorMountStateL"));
    auto * const mountMode = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorMountModeCB"));
    auto * const mountStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorMountStatusL"));
    auto * const train = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorTrainCB"));
    auto * const trainStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorTrainStatusL"));
    auto * const siteState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorSiteStateL"));
    auto * const gpsd = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorGpsdCB"));
    auto * const gpsdStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGpsdStatusL"));
    auto * const guide = manager->findChild<QCheckBox *>(QStringLiteral("essentialSimulatorGuideCB"));
    auto * const guideState = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGuideStateL"));
    auto * const guideMode = manager->findChild<QComboBox *>(QStringLiteral("essentialSimulatorGuideModeCB"));
    auto * const guideStatus = manager->findChild<QLabel *>(QStringLiteral("essentialSimulatorGuideStatusL"));

    QVERIFY(gain != nullptr);
    QVERIFY(cameraState != nullptr);
    QVERIFY(cooler != nullptr);
    QVERIFY(targetTemp != nullptr);
    QVERIFY(focus != nullptr);
    QVERIFY(cameraStatus != nullptr);
    QVERIFY(tracking != nullptr);
    QVERIFY(mountState != nullptr);
    QVERIFY(mountMode != nullptr);
    QVERIFY(mountStatus != nullptr);
    QVERIFY(train != nullptr);
    QVERIFY(trainStatus != nullptr);
    QVERIFY(siteState != nullptr);
    QVERIFY(gpsd != nullptr);
    QVERIFY(gpsdStatus != nullptr);
    QVERIFY(guide != nullptr);
    QVERIFY(guideState != nullptr);
    QVERIFY(guideMode != nullptr);
    QVERIFY(guideStatus != nullptr);

    const QList<int> gainValues { 0, 15, 30 };
    const QList<bool> coolerValues { false, true };
    const QList<double> targetTemps { -10.0, 0.0, 10.0 };
    const QList<int> focusValues { 0, 50, 100 };
    const QList<bool> trackingValues { false, true };
    const QStringList mountModes { QStringLiteral("Sidereal"), QStringLiteral("Move"), QStringLiteral("Slew"), QStringLiteral("Parked") };
    const QStringList trainRoles
    {
        QStringLiteral("Mount"),
        QStringLiteral("Camera"),
        QStringLiteral("Rotator"),
        QStringLiteral("Guide Port"),
        QStringLiteral("Dust Cap"),
        QStringLiteral("Telescope"),
        QStringLiteral("Filter Wheel"),
        QStringLiteral("Focuser"),
        QStringLiteral("Reducer"),
        QStringLiteral("Flat Panel"),
        QStringLiteral("Dome"),
        QStringLiteral("Weather"),
        QStringLiteral("Site Source"),
        QStringLiteral("Polar Alignment"),
    };
    const QHash<QString, QString> trainSummaries
    {
        { QStringLiteral("Mount"), QStringLiteral("Mount controls pointing and tracking") },
        { QStringLiteral("Camera"), QStringLiteral("Camera captures frames with gain and noise") },
        { QStringLiteral("Rotator"), QStringLiteral("Rotator sets the camera angle") },
        { QStringLiteral("Guide Port"), QStringLiteral("Guide port receives pulse corrections") },
        { QStringLiteral("Dust Cap"), QStringLiteral("Dust cap reports cover state") },
        { QStringLiteral("Telescope"), QStringLiteral("Telescope provides aperture and focal length") },
        { QStringLiteral("Filter Wheel"), QStringLiteral("Filter wheel selects filters") },
        { QStringLiteral("Focuser"), QStringLiteral("Focuser changes focus position") },
        { QStringLiteral("Reducer"), QStringLiteral("Reducer changes focal ratio") },
        { QStringLiteral("Flat Panel"), QStringLiteral("Flat panel provides flat-field light") },
        { QStringLiteral("Dome"), QStringLiteral("Dome reports park and slave state") },
        { QStringLiteral("Weather"), QStringLiteral("Weather reports safe or alert") },
        { QStringLiteral("Site Source"), QStringLiteral("Site source provides position and time") },
        { QStringLiteral("Polar Alignment"), QStringLiteral("Polar alignment reports correction") },
    };
    const QList<bool> gpsdValues { false, true };
    const QList<bool> guideValues { false, true };
    const QStringList guideModes { QStringLiteral("Looping"), QStringLiteral("Guiding"), QStringLiteral("Dither") };

    int combinations = 0;
    for (const int gainValue : gainValues)
    {
        for (const int focusValue : focusValues)
        {
            for (const bool coolerEnabled : coolerValues)
            {
                for (const double targetTempValue : targetTemps)
                {
                    for (const bool trackingEnabled : trackingValues)
                    {
                        for (const QString &mountModeName : mountModes)
                        {
                            for (const QString &trainRole : trainRoles)
                            {
                                for (const bool gpsdEnabled : gpsdValues)
                                {
                                    for (const bool guideEnabled : guideValues)
                                    {
                                        for (const QString &guideModeName : guideModes)
                                        {
                                            gain->setValue(gainValue);
                                            focus->setValue(focusValue);
                                            cooler->setChecked(coolerEnabled);
                                            targetTemp->setValue(targetTempValue);
                                            tracking->setChecked(trackingEnabled);
                                            mountMode->setCurrentText(mountModeName);
                                            train->setCurrentText(trainRole);
                                            gpsd->setChecked(gpsdEnabled);
                                            guide->setChecked(guideEnabled);
                                            guideMode->setCurrentText(guideModeName);

                                            const double expectedSensorTemp = coolerEnabled ? targetTempValue + 0.2 + gainValue * 0.02
                                                                              : 18.0 + gainValue * 0.05;
                                            const int expectedCoolerPower = coolerEnabled
                                                                            ? std::clamp(static_cast<int>(std::lround((18.0 - targetTempValue) * 2.0 + gainValue)), 0, 100)
                                                                            : 0;
                                            const double expectedNoise = 1.2 + gainValue * 0.12;
                                            const double expectedBlur = std::abs(focusValue - 50) * 0.04;
                                            const bool expectedAtTarget = coolerEnabled && std::abs(expectedSensorTemp - targetTempValue) <= 0.5;
                                            QCOMPARE(targetTemp->isEnabled(), coolerEnabled);
                                            QCOMPARE(cameraState->text(),
                                                     !coolerEnabled ? QStringLiteral("Cooling off")
                                                     : expectedAtTarget ? QStringLiteral("Camera at target")
                                                     : QStringLiteral("Camera cooling"));
                                            QCOMPARE(cameraStatus->text(),
                                                     coolerEnabled
                                                     ? QStringLiteral("Sensor %1 C - target %2 C - cooler %3 percent - noise %4 e- - focus blur %5 px")
                                                     .arg(QString::number(expectedSensorTemp, 'f', 1),
                                                          QString::number(targetTempValue, 'f', 1),
                                                          QString::number(expectedCoolerPower),
                                                          QString::number(expectedNoise, 'f', 1),
                                                          QString::number(expectedBlur, 'f', 1))
                                                     : QStringLiteral("Sensor %1 C - cooling off - noise %2 e- - focus blur %3 px")
                                                     .arg(QString::number(expectedSensorTemp, 'f', 1),
                                                          QString::number(expectedNoise, 'f', 1),
                                                          QString::number(expectedBlur, 'f', 1)));

                                            if (mountModeName == QStringLiteral("Parked"))
                                            {
                                                QVERIFY(!tracking->isEnabled());
                                                QCOMPARE(mountState->text(), QStringLiteral("Parked"));
                                                QCOMPARE(mountStatus->text(), QStringLiteral("Tracking unavailable"));
                                            }
                                            else
                                            {
                                                QVERIFY(tracking->isEnabled());
                                                const double expectedDrift = trackingEnabled && mountModeName == QStringLiteral("Sidereal") ? 0.2 : 8.0;
                                                QCOMPARE(mountState->text(),
                                                         !trackingEnabled ? QStringLiteral("Tracking off")
                                                         : mountModeName == QStringLiteral("Slew") ? QStringLiteral("Slewing")
                                                         : mountModeName == QStringLiteral("Move") ? QStringLiteral("Moving")
                                                         : QStringLiteral("Tracking"));
                                                QCOMPARE(mountStatus->text(),
                                                         QStringLiteral("%1 - unparked - drift %2 arcsec/min")
                                                         .arg(trackingEnabled ? mountModeName : QStringLiteral("Manual"),
                                                              QString::number(expectedDrift, 'f', 1)));
                                            }

                                            QCOMPARE(trainStatus->text(), trainSummaries.value(trainRole));
                                            QCOMPARE(siteState->text(), gpsdEnabled ? QStringLiteral("GPSD fix") : QStringLiteral("Manual site"));
                                            QCOMPARE(gpsdStatus->text(),
                                                     gpsdEnabled ? QStringLiteral("Resolved site - 35.7N 139.7E")
                                                     : QStringLiteral("Manual coordinates - 35.7N 139.7E"));

                                            if (guideEnabled)
                                            {
                                                QVERIFY(guideMode->isEnabled());
                                                const double expectedRms = guideModeName == QStringLiteral("Guiding") ? 0.38
                                                                           : guideModeName == QStringLiteral("Dither") ? 0.72
                                                                           : 1.10;
                                                QCOMPARE(guideState->text(),
                                                         guideModeName == QStringLiteral("Dither") ? QStringLiteral("Dithering")
                                                         : guideModeName == QStringLiteral("Looping") ? QStringLiteral("Looping")
                                                         : QStringLiteral("Active"));
                                                QCOMPARE(guideStatus->text(),
                                                         QStringLiteral("%1 - RMS %2 px")
                                                         .arg(guideModeName, QString::number(expectedRms, 'f', 2)));
                                            }
                                            else
                                            {
                                                QVERIFY(!guideMode->isEnabled());
                                                QCOMPARE(guideState->text(), QStringLiteral("Idle"));
                                                QCOMPARE(guideStatus->text(), QStringLiteral("No guiding input"));
                                            }

                                            ++combinations;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    QCOMPARE(combinations, 72576);
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
