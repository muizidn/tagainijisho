/*
 *  Copyright (C) 2009  Alexandre Courbot
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

#include "core/TextTools.h"
#include "core/Database.h"
#include "core/RelativeDate.h"
#include "core/Entry.h"
#include "core/EntriesCache.h"
#include "core/jmdict/JMdictEntry.h"
#include "core/jmdict/JMdictPlugin.h"
#include "core/kanjidic2/Kanjidic2Entry.h"
#include "gui/TrainSettings.h"
#include "gui/EntryFormatter.h"
#include "gui/TemplateFiller.h"
#include "gui/ReadingTrainer.h"

#include <QtDebug>
#include <QMessageBox>
#include <QLabel>

PreferenceItem<QByteArray> ReadingTrainer::windowGeometry("readingTrainWindow", "geometry", "");
PreferenceItem<bool> ReadingTrainer::showMeaning("readingTrainWindow", "showMeaning", true);

ReadingTrainer::ReadingTrainer(QWidget *parent) : QFrame(parent), _goodCount(0), _wrongCount(0), _totalCount(0), query(Database::connection())
{
	ui.setupUi(this);
	setWindowTitle(tr("Reading practice"));
	setAttribute(Qt::WA_DeleteOnClose);
	connect(ui.okButton, SIGNAL(clicked()), this, SLOT(checkAnswer()));
	connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(train()));

	_showMeaning = new QCheckBox(tr("Show &meaning"), this);
	_showMeaning->setChecked(ReadingTrainer::showMeaning.value());
	connect(_showMeaning, SIGNAL(toggled(bool)), this, SLOT(onShowMeaningChecked(bool)));
	_showMeaningAction = ui.detailedView->toolBar()->addWidget(_showMeaning);

	restoreGeometry(windowGeometry.value());
}

ReadingTrainer::~ReadingTrainer()
{
	windowGeometry.set(saveGeometry());
}

void ReadingTrainer::newSession()
{
	updateStatusLabel();

	QMessageBox messageBox(this);
	messageBox.setWindowTitle(tr("Preparing training session"));
	messageBox.setInformativeText(tr("Preparing training session, please wait..."));
	messageBox.setStandardButtons(QMessageBox::NoButton);
	messageBox.setWindowModality(Qt::WindowModal);

	// TODO Why the fuck is all that stuff needed to display the message box correctly??
	messageBox.show();
	messageBox.repaint();
	QApplication::processEvents();
	QCoreApplication::sendPostedEvents();
	QApplication::processEvents();
	messageBox.repaint();
	QApplication::processEvents();
	QCoreApplication::sendPostedEvents();
	QApplication::processEvents();
	messageBox.repaint();
	QApplication::processEvents();
	QCoreApplication::sendPostedEvents();
	QApplication::processEvents();

	// Get the train settings and build the query string
	QString queryString(QString("select k1.id from training cross join jmdict.kanjiChar as k1 on training.type = %1 and training.id = k1.kanji cross join training as t2 on t2.type = %2 and t2.id = k1.id cross join jmdict.entries on entries.id = t2.id cross join jmdict.senses on senses.id = k1.id and senses.priority = 0 where k1.priority = 0 and (senses.misc0 & %3) = 0").arg(KANJIDIC2ENTRY_GLOBALID).arg(JMDICTENTRY_GLOBALID).arg(1 << JMdictPlugin::miscMap()["uk"].second));
	RelativeDate minDate(TrainSettings::minDatePref.value());
	if (minDate.isSet()) queryString += QString(" and (t2.dateLastTrain < %1 OR t2.dateLastTrain is null)").arg(QDateTime(minDate.date()).toTime_t());
	RelativeDate maxDate(TrainSettings::maxDatePref.value());
	if (maxDate.isSet()) queryString += QString(" and t2.dateLastTrain > %1").arg(QDateTime(maxDate.date()).toTime_t());
	int minScore(TrainSettings::minScorePref.value());
	if (minScore != TrainSettings::MINSCORE_DEFAULT) queryString += QString(" and t2.score >= %1").arg(minScore);
	int maxScore(TrainSettings::maxScorePref.value());
	if (maxScore != TrainSettings::MAXSCORE_DEFAULT) queryString += QString(" and t2.score <= %1").arg(maxScore);
	queryString += " group by k1.id having count(k1.kanji) = entries.kanjiCount ";
	queryString += TrainSettings::buildOrderString("t2.score");

	query.exec(queryString);
	messageBox.hide();
}

void ReadingTrainer::train()
{
	ui.nextButton->setVisible(false);
	ui.okButton->setVisible(true);
	ui.userInput->setVisible(true);
	ui.userInput->setFocus();

	ui.detailedView->detailedView()->clear();
	if (!_showMeaning->isChecked()) {
		ui.detailedView->detailedView()->display(entry);
	}

	if (query.next()) {
		entry = JMdictEntryRef(query.valueInt(0)).get();
		ui.writingLabel->setText(entry->writings()[0]);
		if (_showMeaning->isChecked()) {
			ui.detailedView->detailedView()->setKanjiClickable(false);
			onShowMeaningChecked(true);
		}
		ui.userInput->clear();
	} else {
		if (_totalCount == 0) QMessageBox::information(this, tr("No matching entries found"), tr("Unable to find any entry eligible for reading practice. Entries eligible for this training mode are studied vocabulary entries for which all kanji are also studied, and that match the train settings. Please add entries or modify the train settings accordingly if you want to practice this mode."));
		else QMessageBox::information(this, tr("No more entries to train"), tr("There are no more entries to train for the current train settings."));
		close();
	}
}

void ReadingTrainer::checkAnswer()
{
	bool correct(false);
	QString answer(ui.userInput->text());
	if (TextTools::isRomaji(answer))
		answer = TextTools::romajiToKana(answer);
	foreach (const QString &reading, entry->readings()) {
		if (TextTools::hiragana2Katakana(answer) == TextTools::hiragana2Katakana(reading)) {
			correct = true;
			break;
		}
	}
	if (correct) {
		ui.resultLabel->setText(tr("<font color=\"green\">Correct!</font>"));
		entry->train(true);
		_goodCount++;
	} else {
		ui.resultLabel->setText(tr("<font color=\"red\">Error!</font>"));
		entry->train(false);
		_wrongCount++;
		ui.nextButton->setVisible(true);
		ui.okButton->setVisible(false);
		ui.userInput->setVisible(false);
		ui.nextButton->setFocus();
		ui.detailedView->detailedView()->display(entry);
	}
	ui.detailedView->detailedView()->setKanjiClickable(true);
	_totalCount++;
	updateStatusLabel();
	if (correct) {
		train();
	}
}

void ReadingTrainer::onShowMeaningChecked(bool checked)
{
	ReadingTrainer::showMeaning.set(checked);
	// Do not update the detailed view if it is already displaying the answer to a mistaken entry.
	if (ui.nextButton->isVisible()) return;
	ui.detailedView->detailedView()->clear();
	if (checked) {
		QTextDocument *document(ui.detailedView->detailedView()->document());
		const EntryFormatter *formatter = EntryFormatter::getFormatter(entry);
		QStringList parts;
		TemplateFiller filler;
		parts << "back";
		document->setDefaultStyleSheet(formatter->CSS());
		QString html(filler.fill(filler.extract(formatter->htmlTemplate(), parts), formatter, entry));
		document->setHtml(html);	
	}
}

void ReadingTrainer::updateStatusLabel()
{
	ui.statusLabel->setText(tr("Correct: %1, Wrong: %2, Total: %3").arg(_goodCount).arg(_wrongCount).arg(_totalCount));
}
