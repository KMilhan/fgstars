/*
    Star Studio target-finder UI journey tests

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TESTSTARSTUDIOTARGETFINDER_H
#define TESTSTARSTUDIOTARGETFINDER_H

#include "config-kstars.h"

#if defined(HAVE_INDI)

#include <QObject>

class TestStarStudioTargetFinder : public QObject
{
        Q_OBJECT

    public:
        explicit TestStarStudioTargetFinder(QObject *parent = nullptr);

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();
        void init();

        void testTargetFinderSelectsImagingTarget();
        void testCenterTargetStartsFromCaptureContext();
        void testShellKeepsImageWorkspacePrimary();
        void testEssentialSimulatorControlsRespond();
        void testCenterTargetRequiresSelectedTarget();
        void testCenterTargetRejectsTargetBelowHorizon();
};

#endif // HAVE_INDI
#endif // TESTSTARSTUDIOTARGETFINDER_H
