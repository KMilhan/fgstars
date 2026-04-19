/*  KStars UI tests
    SPDX-FileCopyrightText: 2020 Eric Dejouhanet <eric.dejouhanet@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TESTEKOSCAPTURE_H
#define TESTEKOSCAPTURE_H

#include "config-kstars.h"

#if defined(HAVE_INDI)

#include "test_ekos_capture_helper.h"

#include <QRect>

#include <QObject>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtTest/QTest>
#else
#include <QTest>
#endif


class TestEkosCapture : public QObject
{
        Q_OBJECT

    public:
        explicit TestEkosCapture(QObject *parent = nullptr);

    private:
        TestEkosCaptureHelper *m_CaptureHelper = new TestEkosCaptureHelper();

        bool publishGuideWorkspaceImage(const QString &fitsFile, const QRect &trackingBox);

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        void init();
        void cleanup();

        /** @brief Test addition, UI sync and removal of capture jobs. */
        void testAddCaptureJob();

        /** @brief Test that storing to a system temporary folder makes the capture a preview. */
        void testCaptureToTemporary();

        /** @brief Test embedded workspace ownership and default prominence. */
        void testEmbeddedWorkspaceHost();

        /** @brief Test that resizing Ekos keeps the shared workspace visually primary. */
        void testWorkspacePrioritySurvivesResize();

        /** @brief Test that the embedded workspace remains visible across image-heavy module switches. */
        void testPersistentWorkspaceAcrossTabs();

        /** @brief Test capturing a single frame in multiple attempts. */
        void testCaptureSingle();

        /** @brief Test that the embedded workspace can explicitly open the current history frame in FITS Viewer. */
        void testDetachedViewerFallbackFromWorkspaceHistory();

        /** @brief Test that capture updates the shared workspace session state. */
        void testWorkspaceSessionTracksCaptureState();

        /** @brief Test that guide frames publish a tracking box into the shared workspace. */
        void testWorkspaceShowsGuideTrackingContext();

        /** @brief Test capturing multiple frames in multiple attempts. */
        void testCaptureMultiple();

        /**
             * @brief Test dark flat frames capture after flat frame
             */
        void testCaptureDarkFlats();
};

#endif // HAVE_INDI
#endif // TESTEKOSCAPTURE_H
