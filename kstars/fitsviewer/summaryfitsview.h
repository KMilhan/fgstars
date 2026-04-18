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
namespace Ekos
{
class WorkspaceSession;
}

class SummaryFITSView : public FITSView
{
        Q_OBJECT

    public:
        explicit SummaryFITSView(QWidget *parent = nullptr);
        void setWorkspaceSession(Ekos::WorkspaceSession *session);

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
        void syncWorkspaceSession();

        // floating bar
        bool m_showProcessInfo { false };
        QPointer<FITSViewer> m_detachedViewer;
        QPointer<Ekos::WorkspaceSession> m_workspaceSession;
        int m_detachedViewerTabID { -1 };
        QAction *openDetachedViewerAction { nullptr };
        QAction *toggleProcessInfoAction { nullptr };

};
