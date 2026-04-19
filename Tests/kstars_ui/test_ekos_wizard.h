#ifndef TESTEKOSWIZARD_H
#define TESTEKOSWIZARD_H

#include "../testhelpers.h"

#ifdef HAVE_INDI

class TestEkosWizard : public QObject
{
        Q_OBJECT

    public:
        explicit TestEkosWizard(QObject *parent = nullptr);

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();
        void init();
        void cleanup();

        void testProfileWizardDoesNotAutoOpen();
        void testProfileWizardCanBeOpenedManually();
};

#endif // HAVE_INDI
#endif // TESTEKOSWIZARD_H
