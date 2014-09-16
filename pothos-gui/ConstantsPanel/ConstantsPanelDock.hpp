// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <QDockWidget>

class QTableWidget;
class QToolButton;
class QLineEdit;

class ConstantsPanelDock : public QDockWidget
{
    Q_OBJECT
public:
    ConstantsPanelDock(QWidget *parent);

private:
    QTableWidget *_table;
    QToolButton *_addButton;
    QLineEdit *_lineEdit;
};
