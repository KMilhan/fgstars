/*
    SPDX-FileCopyrightText: 2021 Wolfgang Reissenberger <sterne-jaeger@openfuture.de>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QAction>
#include <QPointer>
#include <QToolBar>

#include "fitsview.h"

class FITSViewer;

class SummaryFITSView : public FITSView
{
        Q_OBJECT

    public:
        explicit SummaryFITSView(QWidget *parent = nullptr);

        // Floating toolbar
        void createFloatingToolBar();

        // process information widget
        QWidget *processInfoWidget;

    public Q_SLOTS:
        void openInDetachedViewer();

        // process information
        void showProcessInfo(bool show);
        void toggleShowProcessInfo()
        {
            showProcessInfo(!m_showProcessInfo);
        }

        void resizeEvent(QResizeEvent *event) override;

    private:
        // floating bar
        bool m_showProcessInfo { false };
        QPointer<FITSViewer> m_detachedViewer;
        int m_detachedViewerTabID { -1 };
        QAction *openDetachedViewerAction { nullptr };
        QAction *toggleProcessInfoAction { nullptr };

};
