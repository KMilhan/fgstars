#include "test_headless_ux_screenshot.h"
#include "kstars_ui_tests.h"
#include "test_ekos.h"
#include "test_ekos_simulator.h"
#include "ekos/manager.h"
#include <QDir>
#include <QPixmap>
#include <QTabWidget>
#include <vector>
#include <string>
#include <format>
#include <print>

TestHeadlessUxScreenshot::TestHeadlessUxScreenshot(QObject *parent) : QObject(parent) {}

void TestHeadlessUxScreenshot::initTestCase()
{
    if (!KStarsUiTests::configureTestIndiRuntime())
        QSKIP("INDI server binary not found; skipping TestHeadlessUxScreenshot.", SkipAll);

    KVERIFY_EKOS_IS_HIDDEN();
    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
    KTRY_EKOS_START_SIMULATORS();
    
    QTRY_VERIFY_WITH_TIMEOUT(Ekos::Manager::Instance() != nullptr, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(Ekos::Manager::Instance()->isVisible(), 2000);
    QTest::qWait(2000);
}

void TestHeadlessUxScreenshot::cleanupTestCase()
{
    KTRY_CLOSE_EKOS();
}

void TestHeadlessUxScreenshot::testCaptureScreenshots()
{
    auto *manager = Ekos::Manager::Instance();
    QVERIFY(manager != nullptr);

    std::string outDir = "/home/mill/oss/fgstars/.artifacts/ui-shots";
    QDir dir;
    dir.mkpath(QString::fromStdString(outDir));

    std::vector<QSize> sizes = {
        QSize(1280, 860),
        QSize(980, 700),
        QSize(700, 920),
        QSize(700, 700)
    };

    QTabWidget *toolsWidget = manager->findChild<QTabWidget*>("toolsWidget");
    QVERIFY(toolsWidget != nullptr);

    for (const QSize &size : sizes) {
        manager->resize(size);
        QTest::qWait(1000); // Allow layout to settle
        
        for (int i = 0; i < toolsWidget->count(); ++i) {
            toolsWidget->setCurrentIndex(i);
            QTest::qWait(1000); // Allow module UI to render
            
            QPixmap pixmap = manager->grab();
            QString tabName = toolsWidget->tabText(i).remove('&');
            if (tabName.isEmpty()) tabName = QString::number(i);
            std::string filename = std::format("{}/headless-{}x{}-{}.png", outDir, size.width(), size.height(), tabName.toStdString());
            QVERIFY(pixmap.save(QString::fromStdString(filename)));
            std::println("Saved screenshot to {}", filename);
        }
    }
}

QTEST_KSTARS_MAIN(TestHeadlessUxScreenshot)
