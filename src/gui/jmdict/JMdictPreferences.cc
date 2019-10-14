/*
 *  Copyright (C) 2008, 2009  Alexandre Courbot
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

#include "core/jmdict/JMdictEntrySearcher.h"
#include "core/jmdict/JMdictPlugin.h"
#include "gui/jmdict/JMdictEntryFormatter.h"
#include "gui/jmdict/JMdictPreferences.h"
#include "gui/jmdict/JMdictGUIPlugin.h"

JMdictPreferences::JMdictPreferences(QWidget *parent) : PreferencesWindowCategory(tr("Vocabulary entries"), parent)
{
	setupUi(this);

	foreach (const QString &idx, JMdictEntryFormatter::getExampleSentencesServices().keys())
		exampleSentencesServiceBox->addItem(idx);

	connect(printKanjis, SIGNAL(toggled(bool)), printOnlyStudiedKanjis, SLOT(setEnabled(bool)));
	connect(headerPrintSize, SIGNAL(valueChanged(int)), this, SLOT(updatePrintPreview()));
	connect(printKanjis, SIGNAL(toggled(bool)), this, SLOT(updatePrintPreview()));
	connect(printOnlyStudiedKanjis, SIGNAL(toggled(bool)), this, SLOT(updatePrintPreview()));
	connect(maxDefinitionsToPrint, SIGNAL(valueChanged(int)), this, SLOT(updatePrintPreview()));

	connect(filterButton, SIGNAL(clicked()), this, SLOT(onFilterButtonClicked()));
	connect(unFilterButton, SIGNAL(clicked()), this, SLOT(onUnFilterButtonClicked()));

	previewEntry = JMdictEntryRef(1415000).get().staticCast<const JMdictEntry>();

	previewLabel->installEventFilter(this);

	connect(printKanjis, SIGNAL(toggled(bool)), printOnlyStudiedKanjis, SLOT(setEnabled(bool)));

	previewLabel->setPicture(previewPic);
}

bool JMdictPreferences::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == previewLabel) {
		switch (event->type()) {
		case QEvent::Show:
		case QEvent::Resize:
			updatePrintPreview();
			return false;
		default:
			return false;
		}
	}
	return false;
}

void JMdictPreferences::onFilterButtonClicked()
{
	foreach (QListWidgetItem *item, displayedDefs->selectedItems()) {
		QListWidgetItem *newItem = new QListWidgetItem(item->text());
		newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
		filteredDefs->addItem(newItem);
		delete item;
	}
}

void JMdictPreferences::onUnFilterButtonClicked()
{
	foreach (QListWidgetItem *item, filteredDefs->selectedItems()) {
		QListWidgetItem *newItem = new QListWidgetItem(item->text());
		newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
		displayedDefs->addItem(newItem);
		delete item;
	}
}

void JMdictPreferences::refresh()
{
	showJLPT->setChecked(JMdictEntryFormatter::showJLPT.value());
	showKanjis->setChecked(JMdictEntryFormatter::showKanjis.value());
	showJMdictID->setChecked(JMdictEntryFormatter::showJMdictID.value());
	homophonesCount->setValue(JMdictEntryFormatter::maxHomophonesToDisplay.value());
	studiedHomophonesOnly->setChecked(JMdictEntryFormatter::displayStudiedHomophonesOnly.value());
	homographsCount->setValue(JMdictEntryFormatter::maxHomographsToDisplay.value());
	studiedHomographsOnly->setChecked(JMdictEntryFormatter::displayStudiedHomographsOnly.value());
	lookupVerbBuddy->setChecked(JMdictEntryFormatter::searchVerbBuddy.value());

	exampleSentencesServiceBox->setCurrentIndex(0);
	for (int i = 1; i < exampleSentencesServiceBox->count(); i++)
		if (exampleSentencesServiceBox->itemText(i) == JMdictEntryFormatter::exampleSentencesService.value()) {
			exampleSentencesServiceBox->setCurrentIndex(i);
			break;
		}

	headerPrintSize->setValue(JMdictEntryFormatter::headerPrintSize.value());
	printKanjis->setChecked(JMdictEntryFormatter::printKanjis.value());
	printOnlyStudiedKanjis->setChecked(JMdictEntryFormatter::printOnlyStudiedKanjis.value());
	maxDefinitionsToPrint->setValue(JMdictEntryFormatter::maxDefinitionsToPrint.value());

	filteredDefs->clear();
	displayedDefs->clear();
	const QStringList &filtered(JMdictEntrySearcher::miscPropertiesFilter.value().split(','));
	for (int i = 0; i < JMdictPlugin::miscShift().size(); i++) {
		QPair<QString, QString> entity(JMdictPlugin::miscMap()[JMdictPlugin::miscShift()[i]]);
		QString s(QCoreApplication::translate("JMdictLongDescs", entity.second.toLatin1()));
		s[0] = s[0].toUpper();
		QListWidgetItem *item = new QListWidgetItem(s);
		item->setData(Qt::UserRole, entity.first);
		if (filtered.contains(entity.first))
			filteredDefs->addItem(item);
		else
			displayedDefs->addItem(item);
	}
}

void JMdictPreferences::applySettings()
{
	JMdictEntryFormatter::showJLPT.set(showJLPT->isChecked());
	JMdictEntryFormatter::showKanjis.set(showKanjis->isChecked());
	JMdictEntryFormatter::showJMdictID.set(showJMdictID->isChecked());
	JMdictEntryFormatter::maxHomophonesToDisplay.set(homophonesCount->value());
	JMdictEntryFormatter::displayStudiedHomophonesOnly.set(studiedHomophonesOnly->isChecked());
	JMdictEntryFormatter::maxHomographsToDisplay.set(homographsCount->value());
	JMdictEntryFormatter::displayStudiedHomographsOnly.set(studiedHomographsOnly->isChecked());
	JMdictEntryFormatter::searchVerbBuddy.set(lookupVerbBuddy->isChecked());

	if (exampleSentencesServiceBox->currentIndex() == 0)
		JMdictEntryFormatter::exampleSentencesService.reset();
	else
		JMdictEntryFormatter::exampleSentencesService.set(exampleSentencesServiceBox->currentText());

	JMdictEntryFormatter::headerPrintSize.set(headerPrintSize->value());
	JMdictEntryFormatter::printKanjis.set(printKanjis->isChecked());
	JMdictEntryFormatter::printOnlyStudiedKanjis.set(printOnlyStudiedKanjis->isChecked());
	JMdictEntryFormatter::maxDefinitionsToPrint.set(maxDefinitionsToPrint->value());

	QStringList filtered, res;
	for (int i = 0; i < filteredDefs->model()->rowCount(); i++)
		filtered << filteredDefs->item(i)->data(Qt::UserRole).toString();
	for (int i = 0; i < JMdictPlugin::miscShift().size(); i++) {
		QPair<QString, QString> entity(JMdictPlugin::miscMap()[JMdictPlugin::miscShift()[i]]);
		QString s(entity.first);
		if (filtered.contains(s))
			res << entity.first;
	}
	JMdictEntrySearcher::miscPropertiesFilter.set(res.join(","));
}

void JMdictPreferences::updatePrintPreview()
{
	if (!isVisible()) return;

	const JMdictEntryFormatter *formatter = static_cast<const JMdictEntryFormatter*>(EntryFormatter::getFormatter(previewEntry));
	QPainter painter(&previewPic);
	QRectF usedSpace;
	formatter->drawCustom(previewEntry, painter, QRectF(0, 0, printPreviewScrollArea->viewport()->contentsRect().width() - 20, 300), usedSpace, QFont(), headerPrintSize->value(), printKanjis->isChecked(), printOnlyStudiedKanjis->isChecked(), maxDefinitionsToPrint->value());
	previewPic.setBoundingRect(usedSpace.toRect());
	previewLabel->clear();
	previewLabel->setPicture(previewPic);
}
