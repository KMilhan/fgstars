/*
    SPDX-FileCopyrightText: 2017 Csaba Kertesz <csaba.kertesz@gmail.com>
    SPDX-FileCopyrightText: 2020 Eric Dejouhanet <eric.dejouhanet@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kstars_ui_tests.h"

#if defined(HAVE_INDI)

#include "Options.h"
#include "test_ekos_wizard.h"
#include "test_ekos.h"
#include "ekos/manager.h"
#include "kswizard.h"
#include "auxiliary/kspaths.h"
#include "ekos/profilewizard.h"

#include <KActionCollection>

TestEkosWizard::TestEkosWizard(QObject *parent) : QObject(parent)
{
}

void TestEkosWizard::initTestCase()
{
    if (!KStarsUiTests::configureTestIndiRuntime())
        QSKIP("INDI server binary not found; skipping TestEkosWizard.", SkipAll);
    QVERIFY(QDir(Options::indiDriversDir()).exists());

    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
}

void TestEkosWizard::cleanupTestCase()
{
    for (auto d : KStars::Instance()->findChildren<QDialog *>())
        if (d->isVisible())
            d->hide();

    KTRY_CLOSE_EKOS();
    KVERIFY_EKOS_IS_HIDDEN();
}

void TestEkosWizard::init()
{
}

void TestEkosWizard::cleanup()
{
    for (auto d : KStars::Instance()->findChildren<QDialog * >())
        if (d->isVisible())
            d->hide();
}

void TestEkosWizard::testProfileWizardDoesNotAutoOpen()
{
    QTest::qWait(1000);
    auto * const wizard = KStars::Instance()->findChild<ProfileWizard *>();
    QVERIFY2(wizard == nullptr || wizard->isVisible() == false,
             "Ekos should not auto-launch the profile wizard over the shared workspace.");
}

void TestEkosWizard::testProfileWizardCanBeOpenedManually()
{
    auto * const wizardButton = Ekos::Manager::Instance()->findChild<QPushButton *>("wizardProfileB");
    QVERIFY(wizardButton != nullptr);

    bool wizardWasShown = false;
    QTimer::singleShot(0, KStars::Instance(), [&wizardWasShown]()
    {
        auto * const wizard = KStars::Instance()->findChild<ProfileWizard *>();
        if (wizard == nullptr)
            return;

        wizardWasShown = wizard->isVisible();
        wizard->reject();
    });

    QTest::mouseClick(wizardButton, Qt::LeftButton);
    QVERIFY2(wizardWasShown, "The profile wizard should open when the user explicitly requests it.");
}

// There is no test-main macro in this test because that sequence is done systematically
QTEST_KSTARS_MAIN(TestEkosWizard)

#endif // HAVE_INDI
