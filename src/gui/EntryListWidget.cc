/*
 *  Copyright (C) 2010  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/EntryListWidget.h"

#include <QToolButton>

EntryListWidget::EntryListWidget(QWidget *parent) : SearchFilterWidget(parent)
{
	setupUi(this);
	
	QToolButton *menuButton = new QToolButton(this);
	menuButton->setIcon(QIcon(":/images/icons/list-add.png"));
	menuButton->setMenu(lists->helper()->entriesMenu());
	menuButton->setPopupMode(QToolButton::InstantPopup);
	menuButton->setAutoRaise(true);

	QToolBar *_toolBar = new QToolBar(this);
	_toolBar->setAttribute(Qt::WA_MacMiniSize);
	_toolBar->layout()->setContentsMargins(0, 0, 0, 0);
	_toolBar->setStyleSheet("QToolBar { background: none; border-style: none; border-width: 0px; margin: 0px; padding: 0px; }");	
	_toolBar->addWidget(menuButton);
	_toolBar->addAction(entryListView()->newListAction());
	_toolBar->addAction(entryListView()->deleteSelectionAction());
	
	vLayout->insertWidget(0, _toolBar);
}
