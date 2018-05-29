/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mockcompositor.h"

#include <QBackingStore>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <QMimeData>
#include <QPixmap>
#include <QDrag>

#include <QtTest/QtTest>

static const QSize screenSize(1600, 1200);

class TestWindow : public QWindow
{
    Q_OBJECT
public:
    TestWindow()
    {
        setSurfaceType(QSurface::RasterSurface);
        setGeometry(0, 0, 32, 32);
        create();
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::WindowStateChange)
            emit windowStateChangeEventReceived(static_cast<QWindowStateChangeEvent *>(event)->oldState());
        return QWindow::event(event);
    }

signals:
    void windowStateChangeEventReceived(uint oldState);
};

class tst_WaylandClientXdgShellV6 : public QObject
{
    Q_OBJECT
public:
    tst_WaylandClientXdgShellV6(MockCompositor *c)
        : m_compositor(c)
    {
        qRegisterMetaType<Qt::WindowState>();
        QSocketNotifier *notifier = new QSocketNotifier(m_compositor->waylandFileDescriptor(), QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, &tst_WaylandClientXdgShellV6::processWaylandEvents);
        // connect to the event dispatcher to make sure to flush out the outgoing message queue
        connect(QCoreApplication::eventDispatcher(), &QAbstractEventDispatcher::awake, this, &tst_WaylandClientXdgShellV6::processWaylandEvents);
        connect(QCoreApplication::eventDispatcher(), &QAbstractEventDispatcher::aboutToBlock, this, &tst_WaylandClientXdgShellV6::processWaylandEvents);
    }

public slots:
    void processWaylandEvents()
    {
        m_compositor->processWaylandEvents();
    }

    void cleanup()
    {
        // make sure the surfaces from the last test are properly cleaned up
        // and don't show up as false positives in the next test
        QTRY_VERIFY(!m_compositor->surface());
        QTRY_VERIFY(!m_compositor->xdgToplevelV6());
    }

private slots:
    void createDestroyWindow();
    void configure();
    void showMinimized();
    void setMinimized();
    void unsetMaximized();
    void focusWindowFollowsConfigure();
    void windowStateChangedEvents();

private:
    MockCompositor *m_compositor = nullptr;
};

void tst_WaylandClientXdgShellV6::createDestroyWindow()
{
    TestWindow window;
    window.show();

    QTRY_VERIFY(m_compositor->surface());

    window.destroy();
    QTRY_VERIFY(!m_compositor->surface());
}

void tst_WaylandClientXdgShellV6::configure()
{
    QSharedPointer<MockOutput> output;
    QTRY_VERIFY(output = m_compositor->output());

    TestWindow window;
    window.show();

    QSharedPointer<MockSurface> surface;
    QTRY_VERIFY(surface = m_compositor->surface());

    m_compositor->processWaylandEvents();
    QTRY_VERIFY(window.isVisible());
    QTRY_VERIFY(!window.isExposed()); //Window should not be exposed before the first configure event

    //TODO: according to xdg-shell protocol, a buffer should not be attached to a the surface
    //until it's configured. Ensure this in the test!

    QSharedPointer<MockXdgToplevelV6> toplevel;
    QTRY_VERIFY(toplevel = m_compositor->xdgToplevelV6());

    const QSize newSize(123, 456);
    m_compositor->sendXdgToplevelV6Configure(toplevel, newSize);
    QTRY_VERIFY(window.isExposed());
    QTRY_COMPARE(window.visibility(), QWindow::Windowed);
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState);
    QTRY_COMPARE(window.frameGeometry(), QRect(QPoint(), newSize));

    m_compositor->sendXdgToplevelV6Configure(toplevel, screenSize, { ZXDG_TOPLEVEL_V6_STATE_ACTIVATED, ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED });
    QTRY_COMPARE(window.visibility(), QWindow::Maximized);
    QTRY_COMPARE(window.windowStates(), Qt::WindowMaximized);
    QTRY_COMPARE(window.frameGeometry(), QRect(QPoint(), screenSize));

    m_compositor->sendXdgToplevelV6Configure(toplevel, screenSize, { ZXDG_TOPLEVEL_V6_STATE_ACTIVATED, ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN });
    QTRY_COMPARE(window.visibility(), QWindow::FullScreen);
    QTRY_COMPARE(window.windowStates(), Qt::WindowFullScreen);
    QTRY_COMPARE(window.frameGeometry(), QRect(QPoint(), screenSize));

    //The window should remember it's original size
    m_compositor->sendXdgToplevelV6Configure(toplevel, QSize(0, 0), { ZXDG_TOPLEVEL_V6_STATE_ACTIVATED });
    QTRY_COMPARE(window.visibility(), QWindow::Windowed);
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState);
    QTRY_COMPARE(window.frameGeometry(), QRect(QPoint(), newSize));
}

void tst_WaylandClientXdgShellV6::showMinimized()
{
    // On xdg-shell v6 there's really no way for the compositor to tell the window if it's minimized
    // There are wl_surface.enter events and so on, but there's really no way to differentiate
    // between a window preview and an unminimized window.
    TestWindow window;
    window.showMinimized();
    QCOMPARE(window.windowStates(), Qt::WindowMinimized);   // should return minimized until
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState); // rejected by handleWindowStateChanged

    // Make sure the window on the compositor side is/was created here, and not after the test
    // finishes, as that may mess up for later tests.
    QTRY_VERIFY(m_compositor->xdgToplevelV6());
}

void tst_WaylandClientXdgShellV6::setMinimized()
{
    TestWindow window;
    window.show();

    QSharedPointer<MockXdgToplevelV6> toplevel;
    QTRY_VERIFY(toplevel = m_compositor->xdgToplevelV6());

    m_compositor->sendXdgToplevelV6Configure(toplevel);
    QTRY_COMPARE(window.visibility(), QWindow::Windowed);
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState);

    QSignalSpy setMinimizedSpy(toplevel.data(), SIGNAL(setMinimizedRequested()));
    QSignalSpy windowStateChangeSpy(&window, SIGNAL(windowStateChangeEventReceived(uint)));

    window.setVisibility(QWindow::Minimized);
    QCOMPARE(window.visibility(), QWindow::Minimized);
    QCOMPARE(window.windowStates(), Qt::WindowMinimized);
    QTRY_COMPARE(setMinimizedSpy.count(), 1);
    {
        QTRY_VERIFY(windowStateChangeSpy.count() > 0);
        Qt::WindowStates oldStates(windowStateChangeSpy.takeFirst().at(0).toUInt());
        QCOMPARE(oldStates, Qt::WindowNoState);
    }

    // In the meantime the compositor may minimize, do nothing or reshow the window without
    // telling us.

    QTRY_COMPARE(window.visibility(), QWindow::Windowed); // verify that we don't know anything
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState);
    {
        QTRY_COMPARE(windowStateChangeSpy.count(), 1);
        Qt::WindowStates oldStates(windowStateChangeSpy.takeFirst().at(0).toUInt());
        QCOMPARE(oldStates, Qt::WindowNoState); // because the window never was minimized
    }

    // Setting visibility again should send another set_minimized request
    window.setVisibility(QWindow::Minimized);
    QTRY_COMPARE(setMinimizedSpy.count(), 2);
}

void tst_WaylandClientXdgShellV6::unsetMaximized()
{
    TestWindow window;
    window.show();

    QSharedPointer<MockXdgToplevelV6> toplevel;
    QTRY_VERIFY(toplevel = m_compositor->xdgToplevelV6());

    QSignalSpy unsetMaximizedSpy(toplevel.data(), SIGNAL(unsetMaximizedRequested()));

    QSignalSpy windowStateChangedSpy(&window, SIGNAL(windowStateChanged(Qt::WindowState)));

    m_compositor->sendXdgToplevelV6Configure(toplevel, screenSize, { ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED });

    QTRY_COMPARE(windowStateChangedSpy.count(), 1);
    QCOMPARE(windowStateChangedSpy.takeFirst().at(0).toUInt(), Qt::WindowMaximized);

    window.setWindowStates(Qt::WindowNoState);

    QTRY_COMPARE(unsetMaximizedSpy.count(), 1);
    QTRY_COMPARE(windowStateChangedSpy.count(), 1);
    QCOMPARE(windowStateChangedSpy.takeFirst().at(0).toUInt(), Qt::WindowNoState);

    m_compositor->sendXdgToplevelV6Configure(toplevel, QSize(0, 0), {});

    QTRY_COMPARE(windowStateChangedSpy.count(), 1);
    QCOMPARE(windowStateChangedSpy.takeFirst().at(0).toUInt(), Qt::WindowNoState);
}

void tst_WaylandClientXdgShellV6::focusWindowFollowsConfigure()
{
    TestWindow window;
    window.show();

    QSharedPointer<MockXdgToplevelV6> toplevel;
    QTRY_VERIFY(toplevel = m_compositor->xdgToplevelV6());
    QTRY_VERIFY(!window.isActive());

    QSignalSpy windowStateChangeSpy(&window, SIGNAL(windowStateChangeEventReceived(uint)));

    m_compositor->sendXdgToplevelV6Configure(toplevel, QSize(0, 0), { ZXDG_TOPLEVEL_V6_STATE_ACTIVATED });
    QTRY_VERIFY(window.isActive());

    m_compositor->sendXdgToplevelV6Configure(toplevel, QSize(0, 0), {});
    QTRY_VERIFY(!window.isActive());
}

void tst_WaylandClientXdgShellV6::windowStateChangedEvents()
{
    TestWindow window;
    window.show();

    QSharedPointer<MockXdgToplevelV6> toplevel;
    QTRY_VERIFY(toplevel = m_compositor->xdgToplevelV6());

    QSignalSpy eventSpy(&window, SIGNAL(windowStateChangeEventReceived(uint)));
    QSignalSpy signalSpy(&window, SIGNAL(windowStateChanged(Qt::WindowState)));

    m_compositor->sendXdgToplevelV6Configure(toplevel, screenSize, { ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED });

    QTRY_COMPARE(window.windowStates(), Qt::WindowMaximized);
    QTRY_COMPARE(window.windowState(), Qt::WindowMaximized);
    {
        QTRY_COMPARE(eventSpy.count(), 1);
        Qt::WindowStates oldStates(eventSpy.takeFirst().at(0).toUInt());
        QCOMPARE(oldStates, Qt::WindowNoState);

        QTRY_COMPARE(signalSpy.count(), 1);
        uint newState = signalSpy.takeFirst().at(0).toUInt();
        QCOMPARE(newState, Qt::WindowMaximized);
    }

    m_compositor->sendXdgToplevelV6Configure(toplevel, screenSize, { ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN });

    QTRY_COMPARE(window.windowStates(), Qt::WindowFullScreen);
    QTRY_COMPARE(window.windowState(), Qt::WindowFullScreen);
    {
        QTRY_COMPARE(eventSpy.count(), 1);
        Qt::WindowStates oldStates(eventSpy.takeFirst().at(0).toUInt());
        QCOMPARE(oldStates, Qt::WindowMaximized);

        QTRY_COMPARE(signalSpy.count(), 1);
        uint newState = signalSpy.takeFirst().at(0).toUInt();
        QCOMPARE(newState, Qt::WindowFullScreen);
    }

    m_compositor->sendXdgToplevelV6Configure(toplevel, QSize(0, 0), {});

    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState);
    QTRY_COMPARE(window.windowState(), Qt::WindowNoState);
    {
        QTRY_COMPARE(eventSpy.count(), 1);
        Qt::WindowStates oldStates(eventSpy.takeFirst().at(0).toUInt());
        QCOMPARE(oldStates, Qt::WindowFullScreen);

        QTRY_COMPARE(signalSpy.count(), 1);
        uint newState = signalSpy.takeFirst().at(0).toUInt();
        QCOMPARE(newState, Qt::WindowNoState);
    }
}

int main(int argc, char **argv)
{
    setenv("XDG_RUNTIME_DIR", ".", 1);
    setenv("QT_QPA_PLATFORM", "wayland", 1); // force QGuiApplication to use wayland plugin
    setenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell-v6", 1);

    // wayland-egl hangs in the test setup when we try to initialize. Until it gets
    // figured out, avoid clientBufferIntegration() from being called in
    // QWaylandWindow::createDecorations().
    setenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1", 1);

    MockCompositor compositor;
    compositor.setOutputMode(screenSize);

    QGuiApplication app(argc, argv);
    compositor.applicationInitialized();

    tst_WaylandClientXdgShellV6 tc(&compositor);
    return QTest::qExec(&tc, argc, argv);
}

#include <tst_xdgshellv6.moc>
