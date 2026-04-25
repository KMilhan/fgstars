#pragma once

#include <QObject>

class TestHeadlessUxScreenshot : public QObject
{
    Q_OBJECT
public:
    explicit TestHeadlessUxScreenshot(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCaptureScreenshots();
};
