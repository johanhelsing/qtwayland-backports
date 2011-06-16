/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the config.tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandintegration.h"

#include "qwaylanddisplay.h"
#include "qwaylandshmsurface.h"
#include "qwaylandshmwindow.h"
#include "qwaylandnativeinterface.h"
#include "qwaylandclipboard.h"

#include "QtPlatformSupport/private/qgenericunixfontdatabase_p.h"

#include <QtGui/QWindowSystemInterface>
#include <QtGui/QPlatformCursor>
#include <QtGui/QGuiGLFormat>

#include <QtGui/private/qpixmap_raster_p.h>

#include "gl_integration/qwaylandglintegration.h"

QWaylandIntegration::QWaylandIntegration()
    : mFontDb(new QGenericUnixFontDatabase())
    , mDisplay(new QWaylandDisplay())
    , mNativeInterface(new QWaylandNativeInterface)
    , mClipboard(0)
{
}

QPlatformNativeInterface * QWaylandIntegration::nativeInterface() const
{
    return mNativeInterface;
}

QList<QPlatformScreen *>
QWaylandIntegration::screens() const
{
    return mDisplay->screens();
}

bool QWaylandIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPixmapData *QWaylandIntegration::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QPlatformWindow *QWaylandIntegration::createPlatformWindow(QWindow *window) const
{
    if (window->surfaceType() == QWindow::OpenGLSurface)
        return mDisplay->eglIntegration()->createEglWindow(window);

    return new QWaylandShmWindow(window);
}

QPlatformGLContext *QWaylandIntegration::createPlatformGLContext(const QGuiGLFormat &glFormat, QPlatformGLContext *share) const
{
    return mDisplay->eglIntegration()->createPlatformGLContext(glFormat, share);
}

QWindowSurface *QWaylandIntegration::createWindowSurface(QWindow *window, WId winId) const
{
    Q_UNUSED(winId);
    return new QWaylandShmWindowSurface(window);
}

QPlatformFontDatabase *QWaylandIntegration::fontDatabase() const
{
    return mFontDb;
}

QPlatformClipboard *QWaylandIntegration::clipboard() const
{
    if (!mClipboard)
        mClipboard = new QWaylandClipboard(mDisplay);
    return mClipboard;
}
