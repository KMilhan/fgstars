/*  KStars UI tests
    SPDX-FileCopyrightText: 2020 Eric Dejouhanet <eric.dejouhanet@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "test_ekos_capture.h"

#if defined(HAVE_INDI)

#include "Options.h"
#include "kstars_ui_tests.h"
#include "test_ekos.h"
#include "test_ekos_helper.h"
#include "test_ekos_simulator.h"
#include "ekos/capture/capture.h"
#include "ekos/capture/capturepreviewwidget.h"
#include "ekos/workspacesession.h"
#include "ekos/focus/focusmodule.h"
#include "ekos/align/align.h"
#include "ekos/guide/guide.h"
#include "fitsviewer/fitsviewer.h"
#include "fitsviewer/summaryfitsview.h"
#include "kstars.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <algorithm>
#include <array>
#include <optional>
#include <ranges>

TestEkosCapture::TestEkosCapture(QObject *parent) : QObject(parent)
{
}

void TestEkosCapture::initTestCase()
{
    KVERIFY_EKOS_IS_HIDDEN();
    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
    KTRY_EKOS_START_SIMULATORS();

    // HACK: Reset clock to initial conditions
    KHACK_RESET_EKOS_TIME();
}

void TestEkosCapture::cleanupTestCase()
{
    KTRY_EKOS_STOP_SIMULATORS();
    KTRY_CLOSE_EKOS();
    KVERIFY_EKOS_IS_HIDDEN();
}

void TestEkosCapture::init()
{
    Ekos::Manager * const ekos = Ekos::Manager::Instance();

    // Wait for Capture to come up, switch to Capture tab
    QTRY_VERIFY_WITH_TIMEOUT(ekos->captureModule() != nullptr, 5000);
    KTRY_EKOS_GADGET(QTabWidget, toolsWidget);
    toolsWidget->setCurrentWidget(ekos->captureModule());
    QTRY_COMPARE_WITH_TIMEOUT(toolsWidget->currentWidget(), ekos->captureModule(), 1000);
    // wait 0.5 sec to ensure that the capture module has been initialized
    QTest::qWait(500);

    // ensure that the scope is unparked
    Ekos::Mount *mount = Ekos::Manager::Instance()->mountModule();
    if (mount->parkStatus() == ISD::PARK_PARKED)
        mount->unpark();
    QTRY_VERIFY_WITH_TIMEOUT(mount->parkStatus() == ISD::PARK_UNPARKED, 30000);
}

void TestEkosCapture::cleanup()
{
    Ekos::Manager::Instance()->captureModule()->clearSequenceQueue();
    KTRY_CAPTURE_GADGET(QTableWidget, queueTable);
    QTRY_VERIFY_WITH_TIMEOUT(queueTable->rowCount() == 0, 2000);
}


void TestEkosCapture::testAddCaptureJob()
{
    KTRY_CAPTURE_GADGET(QDoubleSpinBox, captureExposureN);
    KTRY_CAPTURE_GADGET(QSpinBox, captureCountN);
    KTRY_CAPTURE_GADGET(QSpinBox, captureDelayN);
    KTRY_CAPTURE_GADGET(QComboBox, FilterPosCombo);
    KTRY_CAPTURE_GADGET(QComboBox, captureTypeS);
    KTRY_CAPTURE_GADGET(QPushButton, addToQueueB);
    KTRY_CAPTURE_GADGET(QTableWidget, queueTable);

    // These are the expected exhaustive list of capture frame names
    QString frameTypes[] = {"Light", "Bias", "Dark", "Flat"};
    int const frameTypeCount = sizeof(frameTypes) / sizeof(frameTypes[0]);

    // Verify our assumption about those frame types is correct (video type is excluded here)
    QTRY_COMPARE_WITH_TIMEOUT(captureTypeS->count(), frameTypeCount + 1, 5000);
    for (QString &frameType : frameTypes)
        if(captureTypeS->findText(frameType) < 0)
            QFAIL(qPrintable(QString("Frame '%1' expected by the test is not in the Capture frame list").arg(frameType)));

    // These are the expected exhaustive list of filter names from the default Filter Wheel Simulator
    QString filterTypes[] = {"Red", "Green", "Blue", "H_Alpha", "OIII", "SII", "LPR", "Luminance"};
    int const filterTypeCount = sizeof(filterTypes) / sizeof(filterTypes[0]);

    // Verify our assumption about those filters is correct - but wait for properties to be read from the device
    QTRY_COMPARE_WITH_TIMEOUT(FilterPosCombo->count(), filterTypeCount, 5000);
    for (QString &filterType : filterTypes)
        if(FilterPosCombo->findText(filterType) < 0)
            QFAIL(qPrintable(QString("Filter '%1' expected by the test is not in the Capture filter list").arg(filterType)));

    // Add a few capture jobs
    int const job_count = 50;
    QWARN("When clicking the 'Add' button, immediately starting to fill the next job overwrites the job being added.");
    for (int i = 0; i < job_count; i++)
    {
        captureExposureN->setValue(((double)i) / 10.0);
        captureCountN->setValue(i);
        captureDelayN->setValue(i);
        KTRY_CAPTURE_COMBO_SET(captureTypeS, frameTypes[i % frameTypeCount]);
        KTRY_CAPTURE_COMBO_SET(FilterPosCombo, filterTypes[i % filterTypeCount]);
        KTRY_CAPTURE_CLICK(addToQueueB);
        // Wait for the job to be added, else the next loop will overwrite the current job
        QTRY_COMPARE_WITH_TIMEOUT(queueTable->rowCount(), i + 1, 100);
    }

    // Count the number of rows
    QVERIFY(queueTable->rowCount() == job_count);

    // Check first capture job item, which could not accept exposure duration 0 and count 0
    QWARN("This test assumes that minimal exposure is 0.01 for the CCD Simulator.");
    queueTable->setCurrentCell(0, 1);
    QTRY_VERIFY_WITH_TIMEOUT(queueTable->currentRow() == 0, 1000);

    // It actually takes time before all signals syncing UI are processed, so wait for situation to settle
    QTRY_COMPARE_WITH_TIMEOUT(captureExposureN->value(), 0.01, 1000);
    QTRY_COMPARE_WITH_TIMEOUT(captureCountN->value(), 1, 1000);
    QTRY_COMPARE_WITH_TIMEOUT(captureDelayN->value(), 0, 1000);
    QTRY_COMPARE_WITH_TIMEOUT(captureTypeS->currentText(), frameTypes[0], 1000);
    QTRY_COMPARE_WITH_TIMEOUT(FilterPosCombo->currentText(), filterTypes[0], 1000);

    // Select a few cells and verify the feedback on the left side UI
    srand(42);
    for (int index = 1; index < job_count / 2; index += rand() % 4 + 1)
    {
        constexpr int timeout = 2000;
        QVERIFY(index < queueTable->rowCount());
        queueTable->setCurrentCell(index, 1);
        QTRY_VERIFY_WITH_TIMEOUT(queueTable->currentRow() == index, timeout);

        // It actually takes time before all signals syncing UI are processed, so wait for situation to settle
        QTRY_VERIFY_WITH_TIMEOUT(std::fabs(captureExposureN->value() - static_cast<double>(index) / 10.0) < 0.1, timeout);
        QTRY_COMPARE_WITH_TIMEOUT(captureCountN->value(), index, timeout);
        QTRY_COMPARE_WITH_TIMEOUT(captureDelayN->value(), index, timeout);
        QTRY_COMPARE_WITH_TIMEOUT(captureTypeS->currentText(), frameTypes[index % frameTypeCount], timeout);
        QTRY_COMPARE_WITH_TIMEOUT(FilterPosCombo->currentText(), filterTypes[index % filterTypeCount], timeout);
    }

    // Remove all the rows
    // TODO: test edge cases with the selected row
    KTRY_CAPTURE_GADGET(QPushButton, removeFromQueueB);
    for (int i = job_count; 0 < i; i--)
    {
        KTRY_CAPTURE_CLICK(removeFromQueueB);
        QVERIFY(i - 1 == queueTable->rowCount());
    }
}

void TestEkosCapture::testCaptureToTemporary()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    // Add five exposures
    KTRY_CAPTURE_ADD_LIGHT(0.1, 5, 0, "Red", "test", destination.path());

    // Start capturing and wait for procedure to end (visual icon changing)
    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), 30000);

    KTRY_GADGET(Ekos::Manager::Instance(), CapturePreviewWidget, capturePreview);
    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData()->filename().endsWith("005.fits"), 5000);

    const auto fitsViewers = KStars::Instance()->findChildren<FITSViewer *>();
    QVERIFY(std::ranges::all_of(fitsViewers, [](const auto *viewer)
    {
        return viewer == nullptr || viewer->isVisible() == false;
    }));

    QWARN("Test capturing to temporary is no longer valid since we don't create temporary files any more.");
    //    QWARN("When storing to a recognized system temporary folder, only one FITS file is created.");
    //    QTRY_VERIFY_WITH_TIMEOUT(searchFITS(QDir(destination.path())).count() == 1, 1000);
    //    QCOMPARE(searchFITS(QDir(destination.path()))[0], QString("Light_005.fits"));
}

void TestEkosCapture::testEmbeddedWorkspaceHost()
{
    Ekos::Manager * const ekos = Ekos::Manager::Instance();

    KTRY_GADGET(ekos, CapturePreviewWidget, capturePreview);
    KTRY_GADGET(ekos, QSplitter, deviceSplitter);
    KTRY_GADGET(ekos, QWidget, rightLayoutWidget);

    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QCOMPARE(ekos->getSummaryPreview(), static_cast<FITSView *>(workspaceView));
    QCOMPARE(workspaceView->parentWidget(), capturePreview->previewWidget);
    QVERIFY(capturePreview->previewWidget->layout() != nullptr);
    QVERIFY(Options::useSummaryPreview());
    QVERIFY(!Options::useFITSViewer());

    const QList<int> sizes = deviceSplitter->sizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY2(sizes[0] > sizes[1], "Embedded workspace pane should remain larger than the contextual side panel by default.");
    QTRY_VERIFY_WITH_TIMEOUT(capturePreview->previewWidget->width() > rightLayoutWidget->width(), 1000);
}

void TestEkosCapture::testWorkspacePrioritySurvivesResize()
{
    Ekos::Manager * const ekos = Ekos::Manager::Instance();

    KTRY_GADGET(ekos, CapturePreviewWidget, capturePreview);
    KTRY_GADGET(ekos, QSplitter, deviceSplitter);
    KTRY_GADGET(ekos, QWidget, rightLayoutWidget);

    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);

    const QSize originalSize = ekos->size();
    const QList<int> originalSplitterSizes = deviceSplitter->sizes();

    ekos->resize(QSize(700, 700));
    QTRY_VERIFY_WITH_TIMEOUT(ekos->width() >= 700, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(ekos->height() >= 700, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->isVisible(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(capturePreview->previewWidget->width() > rightLayoutWidget->width(), 1000);

    deviceSplitter->setSizes(QList<int>({1, 1000}));
    QTRY_VERIFY_WITH_TIMEOUT(deviceSplitter->sizes().size() == 2, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(deviceSplitter->sizes()[0] < deviceSplitter->sizes()[1], 1000);
    QTRY_VERIFY_WITH_TIMEOUT(capturePreview->previewWidget->width() < rightLayoutWidget->width(), 1000);

    const std::array recoverySizes { QSize(760, 700), QSize(1180, 860) };
    for (const QSize &targetSize : recoverySizes)
    {
        ekos->resize(targetSize);
        QTRY_VERIFY_WITH_TIMEOUT(ekos->width() >= targetSize.width(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(ekos->height() >= targetSize.height(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(workspaceView->isVisible(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(capturePreview->previewWidget->width() > rightLayoutWidget->width(), 1000);

        const QList<int> sizes = deviceSplitter->sizes();
        QCOMPARE(sizes.size(), 2);
        QVERIFY2(sizes[0] > sizes[1],
                 "Embedded workspace pane should recover and remain larger than the contextual side panel after resizing.");
    }

    deviceSplitter->setSizes(originalSplitterSizes);
    ekos->resize(originalSize);
    QTRY_VERIFY_WITH_TIMEOUT(ekos->size() == originalSize, 1000);
}

void TestEkosCapture::testPersistentWorkspaceAcrossTabs()
{
    Ekos::Manager * const ekos = Ekos::Manager::Instance();

    KTRY_GADGET(ekos, QSplitter, splitter);
    KTRY_GADGET(ekos, QSplitter, deviceSplitter);
    KTRY_GADGET(ekos, CapturePreviewWidget, capturePreview);
    QTRY_VERIFY_WITH_TIMEOUT(ekos->focusModule() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(ekos->alignModule() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(ekos->guideModule() != nullptr, 5000);

    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);

    QCOMPARE(deviceSplitter->parentWidget(), splitter);
    QVERIFY2(splitter->indexOf(deviceSplitter) == 0, "Persistent workspace shell should live above the tabbed module stack.");

    const std::array<QWidget *, 5> modules = {
        ekos->setupTab,
        ekos->captureModule(),
        ekos->focusModule(),
        ekos->alignModule(),
        ekos->guideModule()
    };

    for (QWidget * const module : modules)
    {
        QVERIFY(module != nullptr);
        KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(module, 1000);
        QTRY_VERIFY_WITH_TIMEOUT(deviceSplitter->isVisible(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(capturePreview->isVisible(), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(workspaceView->isVisible(), 1000);
    }
}

void TestEkosCapture::testCaptureSingle()
{
    // We cannot use a system temporary due to what testCaptureToTemporary marks
    QTemporaryDir destination(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/test-XXXXXX");
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    // Add an exposure
    KTRY_CAPTURE_ADD_LIGHT(0.5, 1, 0, "Red", "test", destination.path());

    // Start capturing and wait for procedure to end (visual icon changing)
    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), 2000);

    // Verify a FITS file was created
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->searchFITS(QDir(destination.path())).count() == 1, 1000);
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[0].startsWith("test_Light_"));
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[0].endsWith("001.fits"));

    KTRY_GADGET(Ekos::Manager::Instance(), CapturePreviewWidget, capturePreview);
    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData()->filename().endsWith("001.fits"), 5000);

    const auto fitsViewers = KStars::Instance()->findChildren<FITSViewer *>();
    QVERIFY(std::ranges::all_of(fitsViewers, [](const auto *viewer)
    {
        return viewer == nullptr || viewer->isVisible() == false;
    }));

    // Reset sequence state - this makes a confirmation dialog appear
    volatile bool dialogValidated = false;
    QTimer::singleShot(200, [&]
    {
        QDialog * const dialog = qobject_cast <QDialog*> (QApplication::activeModalWidget());
        if(dialog != nullptr)
        {
            QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Yes), Qt::LeftButton);
            dialogValidated = true;
        }
    });
    KTRY_CAPTURE_CLICK(resetB);
    QTRY_VERIFY_WITH_TIMEOUT(dialogValidated, 1000);

    // Capture again
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), 2000);

    // Verify an additional FITS file was created - asynchronously eventually
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->searchFITS(QDir(destination.path())).count() == 2, 2000);
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[0].startsWith("test_Light_"));
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[0].endsWith("001.fits"));
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[1].startsWith("test_Light_"));
    QVERIFY(m_CaptureHelper->searchFITS(QDir(destination.path()))[1].endsWith("002.fits"));

    // TODO: test storage options
}

void TestEkosCapture::testCaptureMultiple()
{
    // We cannot use a system temporary due to what testCaptureToTemporary marks
    QTemporaryDir destination(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/test-XXXXXX");
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    // Add a few exposures
    KTRY_CAPTURE_ADD_LIGHT(0.5, 1, 0, "Red", "test", destination.path() + "/%T");
    KTRY_CAPTURE_ADD_LIGHT(0.7, 2, 0, "SII", "test", destination.path() + "/%T");
    KTRY_CAPTURE_ADD_LIGHT(0.2, 5, 0, "Green", "test", destination.path() + "/%T");
    KTRY_CAPTURE_ADD_LIGHT(0.9, 2, 0, "Luminance", "test", destination.path() + "/%T");
    KTRY_CAPTURE_ADD_LIGHT(0.5, 1, 1, "H_Alpha", "test", destination.path() + "/%T");
    QWARN("A sequence of exposures under 1 second will always take 1 second to capture each of them.");
    //size_t const duration = (500+0)*1+(700+0)*2+(200+0)*5+(900+0)*2+(500+1000)*1;
    size_t const duration = 1000 * (1 + 2 + 5 + 2 + 1);
    size_t const count = 1 + 2 + 5 + 2 + 1;

    // Start capturing and wait for procedure to end (visual icon changing) - leave enough time for frames to store
    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), duration * 2);

    // Verify the proper number of FITS file were created
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->searchFITS(QDir(destination.path())).count() == count, 1000);

    // Reset sequence state - this makes a confirmation dialog appear
    volatile bool dialogValidated = false;
    QTimer::singleShot(200, [&]
    {
        QDialog * const dialog = qobject_cast <QDialog*> (QApplication::activeModalWidget());
        if(dialog != nullptr)
        {
            QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Yes), Qt::LeftButton);
            dialogValidated = true;
        }
    });
    KTRY_CAPTURE_CLICK(resetB);
    QTRY_VERIFY_WITH_TIMEOUT(dialogValidated, 500);

    // Capture again
    KTRY_CAPTURE_CLICK(startB);
    QTRY_VERIFY_WITH_TIMEOUT(!startB->icon().name().compare("media-playback-stop"), 500);
    QTRY_VERIFY_WITH_TIMEOUT(!startB->icon().name().compare("media-playback-start"), duration * 2);

    // Verify the proper number of additional FITS file were created again
    QTRY_VERIFY_WITH_TIMEOUT(m_CaptureHelper->searchFITS(QDir(destination.path())).count() == 2 * count, 1000);

    // TODO: test storage options
}

void TestEkosCapture::testDetachedViewerFallbackFromWorkspaceHistory()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    KTRY_CAPTURE_ADD_LIGHT(0.1, 3, 0, "Red", "history", destination.path());

    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), 30000);

    KTRY_GADGET(Ekos::Manager::Instance(), CapturePreviewWidget, capturePreview);
    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData()->filename().endsWith("003.fits"), 5000);

    capturePreview->showPreviousFrame();
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData()->filename().endsWith("002.fits"), 5000);

    workspaceView->openInDetachedViewer();

    QPointer<FITSViewer> detachedViewer;
    QTRY_VERIFY_WITH_TIMEOUT(([&]()
    {
        const auto &viewers = KStars::Instance()->getFITSViewers();
        const auto viewerIt = std::ranges::find_if(viewers, [](const auto &viewer)
        {
            return viewer && viewer->isVisible();
        });
        if (viewerIt == viewers.cend())
            return false;

        detachedViewer = viewerIt->get();
        return detachedViewer != nullptr;
    })(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(detachedViewer->isVisible(), 5000);

    QSharedPointer<FITSView> detachedView;
    QTRY_VERIFY_WITH_TIMEOUT(detachedViewer->getCurrentView(detachedView), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(detachedView != nullptr && detachedView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(detachedView->imageData()->filename().endsWith("002.fits"), 5000);

    detachedViewer->close();
    QTRY_VERIFY_WITH_TIMEOUT(detachedViewer->isVisible() == false, 5000);
}

void TestEkosCapture::testWorkspaceSessionTracksCaptureState()
{
    QTemporaryDir destination;
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    KTRY_CAPTURE_ADD_LIGHT(0.1, 1, 0, "Red", "workspace", destination.path());

    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-start"), 30000);

    auto * const session = Ekos::Manager::Instance()->workspaceSession();
    QVERIFY(session != nullptr);

    KTRY_GADGET(Ekos::Manager::Instance(), CapturePreviewWidget, capturePreview);
    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(session->frame(Ekos::WorkspaceSession::Source::Capture) != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(session->frame(Ekos::WorkspaceSession::Source::Capture)->filename().endsWith("001.fits"), 5000);

    workspaceView->showProcessInfo(true);
    QTRY_VERIFY_WITH_TIMEOUT(session->overlayVisible(QString::fromLatin1(Ekos::WorkspaceSession::CaptureProcessInfoOverlayKey))
                             == std::optional<bool>(true), 5000);
    workspaceView->showProcessInfo(false);
    QTRY_VERIFY_WITH_TIMEOUT(session->overlayVisible(QString::fromLatin1(Ekos::WorkspaceSession::CaptureProcessInfoOverlayKey))
                             == std::optional<bool>(false), 5000);

    const auto initialZoom = session->viewport(Ekos::WorkspaceSession::Source::Capture).zoom;
    workspaceView->ZoomIn();
    QTRY_VERIFY_WITH_TIMEOUT(session->viewport(Ekos::WorkspaceSession::Source::Capture).zoom > initialZoom, 5000);

    if (workspaceView->horizontalScrollBar()->maximum() > 0 || workspaceView->verticalScrollBar()->maximum() > 0)
    {
        workspaceView->horizontalScrollBar()->setValue(workspaceView->horizontalScrollBar()->maximum() / 2);
        workspaceView->verticalScrollBar()->setValue(workspaceView->verticalScrollBar()->maximum() / 2);
        QTRY_COMPARE_WITH_TIMEOUT(session->viewport(Ekos::WorkspaceSession::Source::Capture).scrollPosition,
                                  QPoint(workspaceView->horizontalScrollBar()->value(),
                                         workspaceView->verticalScrollBar()->value()), 5000);
    }
}

void TestEkosCapture::testWorkspaceShowsGuideTrackingContext()
{
    const auto expectedTrackingBox = QRect {64, 72, 96, 96};
    QVERIFY(publishGuideWorkspaceImage(QFINDTESTDATA("../fitsviewer/m47_sim_stars.fits"), expectedTrackingBox));

    auto * const session = Ekos::Manager::Instance()->workspaceSession();
    QVERIFY(session != nullptr);
    const auto overlay = session->guideOverlay();
    QVERIFY(overlay.has_value());
    QVERIFY(overlay->trackingBoxEnabled);
    QCOMPARE(overlay->trackingBox, expectedTrackingBox);
    QTRY_VERIFY_WITH_TIMEOUT(session->frame(Ekos::WorkspaceSession::Source::Guide) != nullptr, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(session->frame(Ekos::WorkspaceSession::Source::Guide)->filename().endsWith("m47_sim_stars.fits"),
                             5000);

    KTRY_GADGET(Ekos::Manager::Instance(), CapturePreviewWidget, capturePreview);
    SummaryFITSView * const workspaceView = capturePreview->summaryFITSView();
    QVERIFY(workspaceView != nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(workspaceView->imageData()->filename(),
                              session->frame(Ekos::WorkspaceSession::Source::Guide)->filename(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->isTrackingBoxEnabled(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(workspaceView->getTrackingBox(), expectedTrackingBox, 5000);

    const auto captureFrame = session->frame(Ekos::WorkspaceSession::Source::Capture);
    KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(Ekos::Manager::Instance()->captureModule(), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData() != nullptr, 5000);
    if (captureFrame != nullptr)
        QTRY_COMPARE_WITH_TIMEOUT(workspaceView->imageData()->filename(), captureFrame->filename(), 5000);
    else
        QTRY_VERIFY_WITH_TIMEOUT(workspaceView->imageData()->filename().endsWith("m47_sim_stars.fits"), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(!workspaceView->isTrackingBoxEnabled(), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(workspaceView->getTrackingBox(), QRect(), 5000);
}

bool TestEkosCapture::publishGuideWorkspaceImage(const QString &fitsFile, const QRect &trackingBox)
{
    QFileInfo targetFits(fitsFile);
    KVERIFY2_SUB(targetFits.exists(), QString("Test FITS file %1 is missing.").arg(targetFits.filePath()).toLocal8Bit());

    auto * const manager = Ekos::Manager::Instance();
    auto * const guide = manager->guideModule();
    KVERIFY_SUB(guide != nullptr);

    KWRAP_SUB(KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(guide, 1000));

    auto * const guideView = guide->findChild<FITSView *>();
    KVERIFY_SUB(guideView != nullptr);

    QSignalSpy loadedSpy(guideView, &FITSView::loaded);
    guideView->loadFile(targetFits.filePath());
    KTRY_VERIFY_WITH_TIMEOUT_SUB(!loadedSpy.isEmpty(), 5000);
    KTRY_VERIFY_WITH_TIMEOUT_SUB(guideView->imageData() != nullptr, 5000);
    KTRY_VERIFY_WITH_TIMEOUT_SUB(guideView->imageData()->filename().endsWith(targetFits.fileName()), 5000);

    guideView->setTrackingBox(trackingBox);
    guideView->setTrackingBoxEnabled(true);
    guideView->updateFrame();

    const auto publishedView = QSharedPointer<FITSView>(guideView, [](FITSView *) {});
    guide->newImage(publishedView);
    KWRAP_SUB(KTRY_SWITCH_TO_MODULE_WITH_TIMEOUT(guide, 1000));
    KTRY_VERIFY_WITH_TIMEOUT_SUB(manager->workspaceSession()->activeSource() == Ekos::WorkspaceSession::Source::Guide,
                                 5000);
    KTRY_VERIFY_WITH_TIMEOUT_SUB(manager->workspaceSession()->frame(Ekos::WorkspaceSession::Source::Guide) != nullptr,
                                 5000);
    KTRY_VERIFY_WITH_TIMEOUT_SUB(manager->workspaceSession()->frame(Ekos::WorkspaceSession::Source::Guide)->filename()
                                 .endsWith(targetFits.fileName()), 5000);
    return true;
}

void TestEkosCapture::testCaptureDarkFlats()
{
    // ensure that we know that the CCD has a shutter
    m_CaptureHelper->ensureCCDShutter(true);
    // We cannot use a system temporary due to what testCaptureToTemporary marks
    QTemporaryDir destination(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/test-XXXXXX");
    QVERIFY(destination.isValid());
    QVERIFY(destination.autoRemove());

    KTRY_CAPTURE_GADGET(QTableWidget, queueTable);

    // Set to flat first so that the flat calibration button is enabled
    KTRY_CAPTURE_GADGET(QComboBox, captureTypeS);
    KTRY_CAPTURE_COMBO_SET(captureTypeS, "Flat");

    volatile bool dialogValidated = false;
    QTimer::singleShot(200, [&]
    {
        QDialog *dialog = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(dialog = Ekos::Manager::Instance()->findChild<QDialog *>("Calibration"), 2000);

        // Set Flat duration to ADU
        QRadioButton *ADUC = dialog->findChild<QRadioButton *>("captureCalibrationUseADU");
        QVERIFY(ADUC);
        ADUC->setChecked(true);

        // Set ADU to 4000
        QSpinBox *ADUValue = dialog->findChild<QSpinBox *>("captureCalibrationADUValue");
        QVERIFY(ADUValue);
        ADUValue->setValue(4000);


        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok), Qt::LeftButton);
        dialogValidated = true;
    });

    // Toggle flat calibration dialog
    KTRY_CAPTURE_CLICK(calibrationB);

    QTRY_VERIFY_WITH_TIMEOUT(dialogValidated, 5000);

    // Add flats
    KTRY_CAPTURE_ADD_FLAT(1, 2, 0, "Red", destination.path());
    KTRY_CAPTURE_ADD_FLAT(0.1, 3, 0, "Green", destination.path());

    // Generate Dark Flats
    KTRY_CAPTURE_CLICK(generateDarkFlatsB);

    // We should have 4 jobs in total (2 flats and 2 dark flats)
    QTRY_VERIFY2_WITH_TIMEOUT(queueTable->rowCount() == 4,
                              QString("Row number wrong, %1 expected, %2 found!").arg(4).arg(queueTable->rowCount()).toStdString().c_str(), 1000);

    // Start capturing and wait for procedure to end (visual icon changing) - leave enough time for frames to store
    KTRY_CAPTURE_GADGET(QPushButton, startB);
    QCOMPARE(startB->icon().name(), QString("media-playback-start"));
    KTRY_CAPTURE_CLICK(startB);
    QTRY_COMPARE_WITH_TIMEOUT(startB->icon().name(), QString("media-playback-stop"), 500);

    // Accept any dialogs
    QTimer::singleShot(5000, [&]
    {
        auto dialog = qobject_cast <QMessageBox*> (QApplication::activeModalWidget());
        if(dialog)
        {
            dialog->accept();
        }
    });

    // In the full capture suite, the button widget can be recreated while the
    // sequence is running. Wait on the files the test cares about instead of a
    // raw widget pointer so we still verify end-to-end completion safely.
    QTRY_COMPARE_WITH_TIMEOUT(m_CaptureHelper->searchFITS(QDir(destination.path())).count(), 10, 120000);

    // Verify dark flat job (3) matches flat job (1) time
    queueTable->selectRow(0);
    KTRY_CAPTURE_GADGET(NonLinearDoubleSpinBox, captureExposureN);
    auto flatDuration = captureExposureN->value();

    queueTable->selectRow(2);
    auto darkDuration = captureExposureN->value();

    QTRY_COMPARE(darkDuration, flatDuration);


}

QTEST_KSTARS_MAIN(TestEkosCapture)

#endif // HAVE_INDI
