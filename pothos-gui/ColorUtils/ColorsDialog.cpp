// Copyright (c) 2013-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "ColorUtils/ColorsDialog.hpp"
#include "ColorUtils/ColorUtils.hpp"
#include <QTreeWidget>
#include <QVBoxLayout>
#include <string>
#include <algorithm>

ColorsDialog::ColorsDialog(QWidget *parent):
    QDialog(parent)
{
    this->setWindowTitle(tr("Type color mapping"));

    auto layout = new QVBoxLayout(this);
    auto tree = new QTreeWidget(this);
    layout->addWidget(tree);
    tree->setAutoScroll(true);
    tree->setColumnCount(1);
    tree->setHeaderLabels(QStringList(tr("Color to type")));

    //query map and sort keys
    const auto typeStrToColorMap = getTypeStrToColorMap();
    std::vector<std::string> typeStrs;
    for (const auto &pair : typeStrToColorMap) typeStrs.push_back(pair.first);
    std::sort(typeStrs.begin(), typeStrs.end());

    //populate tree with colors
    for (const auto &typeStr : typeStrs)
    {
        auto item = new QTreeWidgetItem(tree, QStringList(QString::fromStdString(typeStr)));
        item->setIcon(0, colorToWidgetIcon(typeStrToColorMap.at(typeStr)));
        tree->addTopLevelItem(item);
    }
    tree->resizeColumnToContents(0);

    this->setMinimumSize(375, 500);
    this->show();
    this->adjustSize();
}
