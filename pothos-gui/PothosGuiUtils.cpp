// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosGuiUtils.hpp"
#include <Pothos/System.hpp>
#include <Poco/Path.h>
#include <Poco/SingletonHolder.h>
#include <QToolButton>

QMap<QString, QAction *> &getActionMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QAction *>> sh;
    return *sh.get();
}

QMap<QString, QMenu *> &getMenuMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QMenu *>> sh;
    return *sh.get();
}

QMap<QString, QObject *> &getObjectMap(void)
{
    static Poco::SingletonHolder<QMap<QString, QObject *>> sh;
    return *sh.get();
}

struct MySettings : QSettings
{
    MySettings(void):
        QSettings(settingsPath(), QSettings::IniFormat)
    {
        return;
    }

    static QString settingsPath(void)
    {
        Poco::Path path(Pothos::System::getUserConfigPath());
        path.append("PothosGui.conf");
        return QString::fromStdString(path.toString());
    }
};

QSettings &getSettings(void)
{
    static Poco::SingletonHolder<MySettings> sh;
    return *sh.get();
}

QString makeIconPath(const QString &name)
{
    Poco::Path path(Pothos::System::getDataPath());
    path.append("icons");
    if (not name.isEmpty()) path.append(name.toStdString());
    return QString::fromStdString(path.toString());
}

QIcon makeIconFromTheme(const QString &name)
{
    return QIcon::fromTheme(name, QIcon(makeIconPath(name+".png")));
}

QToolButton *makeToolButton(QWidget *parent, const QString &theme)
{
    auto tb = new QToolButton(parent);
    tb->setCursor(Qt::PointingHandCursor);
    tb->setFocusPolicy(Qt::NoFocus);
    tb->setIcon(makeIconFromTheme(theme));
    tb->setStyleSheet("background: transparent; border: none;");
    return tb;
}
