/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDCLIENTBUFFERINTEGRATION_H
#define QWAYLANDCLIENTBUFFERINTEGRATION_H

#include <QtWaylandCompositor/qwaylandexport.h>
#include <QtWaylandCompositor/qwaylandsurface.h>
#include <QtCore/QSize>
#include <QtGui/qopengl.h>
#include <QtGui/QOpenGLContext>
#include <wayland-server.h>

QT_BEGIN_NAMESPACE

class QWaylandCompositor;

namespace QtWayland {
class Display;

class Q_COMPOSITOR_EXPORT ClientBufferIntegration
{
public:
    ClientBufferIntegration();
    virtual ~ClientBufferIntegration() { }

    void setCompositor(QWaylandCompositor *compositor) { m_compositor = compositor; }

    virtual void initializeHardware(struct ::wl_display *display) = 0;

    // Used when the hardware integration wants to provide its own texture for a given buffer.
    // In most cases the compositor creates and manages the texture so this is not needed.
    virtual GLuint textureForBuffer(struct ::wl_resource *buffer) { Q_UNUSED(buffer); return 0; }
    virtual void destroyTextureForBuffer(struct ::wl_resource *buffer) { Q_UNUSED(buffer); }

    // Called with the texture bound.
    virtual void bindTextureToBuffer(struct ::wl_resource *buffer) = 0;

    virtual QWaylandSurface::Origin origin(struct ::wl_resource *) const { return QWaylandSurface::OriginBottomLeft; }

    virtual void *lockNativeBuffer(struct ::wl_resource *) const { return 0; }
    virtual void unlockNativeBuffer(void *) const { return; }

    virtual QSize bufferSize(struct ::wl_resource *) const { return QSize(); }

protected:
    QWaylandCompositor *m_compositor;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDCLIENTBUFFERINTEGRATION_H
