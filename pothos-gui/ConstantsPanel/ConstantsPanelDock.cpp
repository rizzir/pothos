// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosGuiUtils.hpp" //make tool button
#include "ConstantsPanelDock.hpp"
#include <QTableWidget>
#include <QToolButton>
#include <QLineEdit>

ConstantsPanelDock::ConstantsPanelDock(QWidget *parent):
    QDockWidget(parent),
    _table(nullptr),
    _addButton(nullptr)
{
    this->setObjectName("ConstantsPanelDock");
    this->setWindowTitle(tr("Constants Panel"));
    this->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->setWidget(new QWidget(this));

    //create table
    _table = new QTableWidget(this->widget());
    _table->setColumnCount(2);
    _table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Name")));
    _table->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Value")));

    //create entry widgets
    _addButton = makeToolButton(this->widget(), "list-add");
    connect(_addButton, SIGNAL(clicked(void)), this, SLOT(handleAdd(void)));

    //create the data entry row
    _table->setRowCount(1);
    _table->setCellWidget(0, 0, _addButton);
    _table->setSpan(0, 0, 1, 2);

    _table->resizeColumnsToContents();
}

void ConstantsPanelDock::reload(const std::vector<std::pair<QString, QString>> &constants)
{
    
    _table->resizeColumnsToContents();
}

void ConstantsPanelDock::handleAdd(void)
{
    
}
