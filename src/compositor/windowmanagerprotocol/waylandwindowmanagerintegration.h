/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Compositor.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WAYLANDWINDOWMANAGERINTEGRATION_H
#define WAYLANDWINDOWMANAGERINTEGRATION_H

#include <qwindowdefs.h>
#include <stdint.h>

#include <QObject>
#include <QMap>
#include <QVariant>

struct wl_client;
struct wl_object;

namespace Wayland {
    class Display;
}

class WindowManagerObject;
class WaylandManagedClient;

class WindowManagerServerIntegration : public QObject
{
    Q_OBJECT
public:
    WindowManagerServerIntegration(QObject *parent = 0);
    static WindowManagerServerIntegration *instance();
    void initialize(Wayland::Display *waylandDisplay);
    void removeClient(wl_client *client);

    WaylandManagedClient *managedClient(wl_client *client) const;

    void setVisibilityOnScreen(wl_client *client, bool visible);
    void setScreenOrientation(wl_client *client, wl_object *output, Qt::ScreenOrientation orientationInDegrees);
    void updateOrientation(wl_client *client);

    void updateWindowProperty(wl_client *client, struct wl_surface *surface, const char *name, struct wl_array *value);
    void setWindowProperty(wl_client *client, struct wl_surface *surface, const QString &name, const QVariant &value);

signals:
    void clientAuthenticated(wl_client *client);
    void windowPropertyChanged(wl_client *client, struct wl_surface *surface, const QString &name, const QVariant &value);

private:
    void mapClientToProcess(wl_client *client, uint32_t processId);
    void authenticateWithToken(wl_client *client, const char *token);

private:
    QMap<wl_client*, WaylandManagedClient*> m_managedClients;
    static WindowManagerServerIntegration *m_instance;

    WindowManagerObject *m_windowManagerObject;

    friend class WindowManagerObject;
};

class WaylandManagedClient
{
public:
    WaylandManagedClient();
    qint64 processId() const;
    QByteArray authenticationToken() const;

private:
    qint64 m_processId;
    QByteArray m_authenticationToken;

    friend class WindowManagerServerIntegration;
};

#endif // WAYLANDWINDOWMANAGERINTEGRATION_H