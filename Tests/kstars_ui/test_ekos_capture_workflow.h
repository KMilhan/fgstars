/*
    KStars UI tests for alignment

    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "config-kstars.h"
#include "test_ekos.h"
#include "test_ekos_capture_helper.h"

#if defined(HAVE_INDI)

#include <QObject>

class SkyObject;

// Select the flat source from the flats calibration options.
// Start with a delay of 1 sec a new thread that edits the calibration options:
//    select the source widget
//    set pre-mount and pre-dome park options
//    select a manual flat ADU value
//    click OK
// open the calibration dialog
#define KTRY_SELECT_FLAT_METHOD(sourceWidget, preMountPark, preDomePark) do { \
    bool completed = false; \
    QTimer::singleShot(5000, capture, [&]() { \
    QDialog *calibrationOptions = nullptr; \
    if (! QTest::qWaitFor([&](){return ((calibrationOptions = Ekos::Manager::Instance()->findChild<QDialog*>("Calibration")) != nullptr);}, 5000)) { \
        QFAIL(qPrintable("Calibrations options dialog not found!")); } \
    KTRY_GADGET(calibrationOptions, QAbstractButton, sourceWidget); \
    sourceWidget->setChecked(true); \
    KTRY_GADGET(calibrationOptions, QCheckBox, captureCalibrationParkMount);  \
    captureCalibrationParkMount->setChecked(preMountPark); \
    KTRY_GADGET(calibrationOptions, QCheckBox, captureCalibrationParkDome);  \
    captureCalibrationParkDome->setChecked(preDomePark); \
    KTRY_GADGET(calibrationOptions, QAbstractButton, captureCalibrationDurationManual);  \
    captureCalibrationDurationManual->setChecked(true); \
    QDialogButtonBox* buttons = calibrationOptions->findChild<QDialogButtonBox*>("buttonBox"); \
    QVERIFY(nullptr != buttons); \
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton); \
    }); \
    KTRY_CAPTURE_CLICK(calibrationB); \
    QTRY_VERIFY_WITH_TIMEOUT(completed == true, 5000);  } while (false)

#define KTRY_SELECT_FLAT_WALL(capture, azimuth, altitude) do { \
    bool completed = false; \
    QTimer::singleShot(1000, capture, [&]() { \
    QDialog *calibrationOptions = nullptr; \
    if (! QTest::qWaitFor([&](){return ((calibrationOptions = Ekos::Manager::Instance()->findChild<QDialog*>("Calibration")) != nullptr);}, 5000)) { \
        QFAIL(qPrintable("Calibrations options dialog not found!")); } \
    KTRY_GADGET(calibrationOptions, QAbstractButton, captureCalibrationWall); \
    captureCalibrationWall->setChecked(true); \
    QVERIFY(captureCalibrationWall->isChecked()); \
    KTRY_SET_LINEEDIT(calibrationOptions, azBox, azimuth); \
    KTRY_SET_LINEEDIT(calibrationOptions, altBox, altitude); \
    KTRY_GADGET(calibrationOptions, QAbstractButton, captureCalibrationDurationManual);  \
    captureCalibrationDurationManual->setChecked(true); \
    QDialogButtonBox* buttons = calibrationOptions->findChild<QDialogButtonBox*>("buttonBox"); \
    QVERIFY(nullptr != buttons); \
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton); \
    completed = true; \
    }); \
    KTRY_CLICK(Ekos::Manager::Instance()->captureModule(), calibrationB); \
    QTRY_VERIFY_WITH_TIMEOUT(completed == true, 5000); } while (false)


class TestEkosCaptureWorkflow : public QObject
{
        Q_OBJECT
    public:
        explicit TestEkosCaptureWorkflow(QObject *parent = nullptr);
        explicit TestEkosCaptureWorkflow(QString guider, QObject *parent = nullptr);


    protected:
        // destination where images will be located
        QTemporaryDir *destination;
        QDir *imageLocation = nullptr;

    protected Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        void init();
        void cleanup();

        bool prepareTestCase();

    private:
        // helper class
        TestEkosCaptureHelper *m_CaptureHelper = nullptr;

        QString target = "test";

        /**
         * @brief Setup capturing
         * @param refocusLimitTime time limit to trigger re-focusing
         * @param refocusHFR HFR limit to trigger re-focusing
         * @param refocusTemp temperature limit to trigger re-focusing
         * @param delay delay between frame captures
         * @return true iff preparation was successful
         */
        bool prepareCapture(int refocusLimitTime = 0.0, double refocusHFR = 0.0, double refocusTemp = 0.0, int delay = 0);

        /**
         * @brief Select a target in KStars and center the SkyMap on it.
         */
        bool selectTarget(const QString &targetName, SkyObject **selectedObject = nullptr);

        /**
         * @brief Select a target through the Find Object dialog.
         */
        bool selectTargetViaFindDialog(const QString &targetName);

        /**
         * @brief Slew the telescope using the currently focused SkyMap object.
         */
        bool slewFocusedTarget(const QString &expectedTargetName);

        /**
         * @brief Queue a simple repeated light-frame capture for a target.
         */
        bool queueRepeatedCapture(const QString &targetName, int frameCount, double exposureSeconds);

        /**
         * @brief Queue repeated capture by filling Capture widgets directly.
         */
        bool queueRepeatedCaptureViaUi(const QString &expectedTargetName, int frameCount, double exposureSeconds);

        /**
         * @brief Start the current queue and wait until capture finishes.
         */
        bool runCaptureToCompletion(const QString &expectedLastFrameSuffix);

        /**
         * @brief Count FITS files produced in the test image directory.
         */
        int capturedFitsCount() const;

        /**
         * @brief initCaptureSetting Initialize Capture settings with a single bool/double pair
         * @param setting a single setting with an enabling checkbox and the value
         * @param checkboxName name of the QCheckBox widget
         * @param spinBoxName name of the QDoubleSpinBox widget
         */
        void initCaptureSetting(TestEkosCaptureHelper::OptDouble setting, const QString checkboxName, const QString dspinBoxName);

        /**
         * @brief Helper function translating simple QString input into QTest test data rows
         * @param exptime exposure time of the sequence
         * @param sequenceList List of sequences with filter and count as QString("<filter>:<count"), ... list
         */
        void prepareTestData(double exptime, QList<QString> sequenceList);

        /**
         * @brief verifyCalibrationSettings Verify if the flats calibration settings match the test data.
         */
        bool verifyCalibrationSettings();

        /**
         * @brief fillScripts Open the scripts dialog and fill its values from {@see #scripts}
         */
        void fillCaptureScripts();

        // counter for images taken in a single test run
        int image_count;

        QDir *getImageLocation();


    private Q_SLOTS:
        /** @brief Test if re-focusing is triggered after the configured delay. */
        void testCaptureRefocusDelay();

        /** @brief Test data for @see testCaptureRefocusDelay() */
        void testCaptureRefocusDelay_data();

        /** @brief Test if re-focusing is triggered when the HFR gets worse. */
        void testCaptureRefocusHFR();

        /** @brief Test data for @see testCaptureRefocusHFR() */
        void testCaptureRefocusHFR_data();

        /**
         * @brief Regression test for the bug where an in-sequence HFR check switches
         *        the filter wheel to the Focus module's lock filter (Luminance) but
         *        never restores it to the original capture filter (Red) after the
         *        check passes.
         *
         * Steps:
         * 1. Enable in-sequence HFR with a small threshold (0.1 %) so the check
         *    fires after the first captured frame.
         * 2. Run autofocus with the Luminance filter (prepareFocusModule already
         *    set focusFilter = "Luminance" and locked every filter to "Luminance").
         * 3. Add a two-frame Red capture sequence (5 s each).
         * 4. Run the capture sequence and wait for CAPTURE_COMPLETE.
         * 5. Assert that the physical filter wheel is on Red (success), not
         *    Luminance (fail — demonstrates the unrestored-filter bug).
         */
        void testCaptureHFRCheckFilterRestoration();

        /** @brief Test if re-focusing is triggered when the temperature changes. */
        void testCaptureRefocusTemperature();

        /** @brief Test data for @see testCaptureRefocusTemperature() */
        void testCaptureRefocusTemperature_data();

        /** @brief Test if re-focusing is aborted if capture is aborted. */
        void testCaptureRefocusAbort();

        /** @brief Test data for @see testCaptureRefocusAbort() */
        void testCaptureRefocusAbort_data();

        /** @brief Test whether a pre/post-job/capture scripts are executed as expected */
        void testCaptureScriptsExecution();

        /** @brief Test data for @see testCaptureScriptsExecution() */
        void testCaptureScriptsExecution_data();

        /**
         * @brief User journey: choose a KStars target, run repeated capture, and finish cleanly.
         */
        void testTargetedRepeatCaptureJourney();

        /**
         * @brief User journey: switch targets mid-session and run repeated capture for both.
         */
        void testRetargetedRepeatCaptureJourney();

        /**
         * @brief Blackbox journey: Find dialog target selection, focused slew, repeated capture, and cleanup.
         */
        void testBlackboxTargetedRepeatCaptureJourney();

        /**
         * @brief Blackbox journey: retarget through Find dialog and capture both sessions.
         */
        void testBlackboxRetargetedRepeatCaptureJourney();

        /**
         * @brief Test if capture continues where it had been suspended by a
         * guiding deviation as soon as guiding is back below the deviation threshold
         */
        void testGuidingDeviationSuspendingCapture();

        /**
         * @brief Test if aborting a job suspended due to a guiding deviation
         * remains aborted when the guiding deviation is below the configured threshold.
         */
        void testGuidingDeviationAbortCapture();

        /**
         * @brief Test if a guiding deviation beyond the configured limit blocks the start of
         * capturing until the guiding deviation is below the configured deviation threshold.
         */
        void testInitialGuidingLimitCapture();

        /** @brief Test data for @see testInitialGuidingLimitCapture() */
        void testInitialGuidingLimitCapture_data();

        /**
         * @brief Wait with start of capturing until the target temperature has been reached
         */
        void testCaptureWaitingForTemperature();

        /** @brief Test data for {@see testCaptureWaitingForTemperature()} */
        void testCaptureWaitingForTemperature_data();

        /**
         * @brief Wait with start of capturing until the target rotator position has been reached
         */
        void testCaptureWaitingForRotator();

        /**
         * @brief Test capturing flats with manual flat light
         */
        void testFlatManualSource();

        /** @brief Test data for {@see testFlatManualSource()} */
        void testFlatManualSource_data();

        /**
         * @brief Test capturing flats, darks and bias with a lights panel
         */
        void testLightPanelSource();

        /** @brief Test data for {@see testLightPanelSource()} */
        void testLightPanelSource_data();

        /**
         * @brief Test capturing flats or darks with a dust cap
         */
        void testDustcapSource();

        /** @brief Test data for {@see testFlatDustcapSource()} */
        void testDustcapSource_data();

        /**
         * @brief Test capturing flats with the wall as flat light source
         */
        void testWallSource();

        /** @brief Test data for {@see testWallSource()} */
        void testWallSource_data();

        /**
         * @brief Check mount and dome parking before capturing flats.
         */
        //void testPreMountAndDomePark();

        /** @brief Test data for {@see testFlatPreMountAndDomePark()} */
        //void testPreMountAndDomePark_data();

        /**
         * @brief Check the flat capture behavior if "same focus" is selectee
         *        in the filter settings when capturing flats. Before capturing
         *        flats, the focuser should move to the position last determined
         *        with an autofocus run for the selected filter.
         */
        void testFlatSyncFocus();

        /**
         * @brief Test capturing darks with manual scope covering
         */
        void testDarkManualCovering();

        /** @brief Test data for {@see testDarkManualCovering()} */
        void testDarkManualCovering_data();

        /**
         * @brief Test capturing a simple darks library
         */
        void testDarksLibrary();

        /**
         * @brief testLoadEsqFile Test for correctly loading a capture sequence file. This test case concentrates
         * upon the general values stored in the file, that are valid for all sequence jobs contained.
         */
        void testLoadEsqFileGeneral();

        /** @brief Test data for {@see testLoadEsqFileGeneral()} */
        void testLoadEsqFileGeneral_data();

        /**
         * @brief testLoadEsqFileBasicJobSettings Test for basic job settings when loading capture sequence file.
         */
        void testLoadEsqFileBasicJobSettings();

        /** @brief Test data for {@see testLoadEsqFileBasicJobSettings()} */
        void testLoadEsqFileBasicJobSettings_data();

        /**
         * @brief testLoadEsqFileFlatSettings Test for flat job settings when loading capture sequence file.
         */
        void testLoadEsqFileCalibrationSettings();

        /** @brief Test data for {@see testLoadEsqFileCalibrationSettings()} */
        void testLoadEsqFileCalibrationSettings_data();
};

#endif // HAVE_INDI
