#include "simplewidgets.h"
#include "filtermodels.h"

#include <QProcess>
#include <QFileDialog>
#include <QShortcut>
#include <QCalendarWidget>
#include <QKeyEvent>
#include <QAction>

#include "file.h"
#include "mainwindow.h"
#include "helpers.h"
#include "libdivecomputer/parser.h"
#include "divelistview.h"
#include "display.h"
#include "profile/profilewidget2.h"
#include "undocommands.h"

class MinMaxAvgWidgetPrivate {
public:
	QLabel *avgIco, *avgValue;
	QLabel *minIco, *minValue;
	QLabel *maxIco, *maxValue;

	MinMaxAvgWidgetPrivate(MinMaxAvgWidget *owner)
	{
		avgIco = new QLabel(owner);
		avgIco->setPixmap(QIcon(":/average").pixmap(16, 16));
		avgIco->setToolTip(QObject::tr("Average"));
		minIco = new QLabel(owner);
		minIco->setPixmap(QIcon(":/minimum").pixmap(16, 16));
		minIco->setToolTip(QObject::tr("Minimum"));
		maxIco = new QLabel(owner);
		maxIco->setPixmap(QIcon(":/maximum").pixmap(16, 16));
		maxIco->setToolTip(QObject::tr("Maximum"));
		avgValue = new QLabel(owner);
		minValue = new QLabel(owner);
		maxValue = new QLabel(owner);

		QGridLayout *formLayout = new QGridLayout();
		formLayout->addWidget(maxIco, 0, 0);
		formLayout->addWidget(maxValue, 0, 1);
		formLayout->addWidget(avgIco, 1, 0);
		formLayout->addWidget(avgValue, 1, 1);
		formLayout->addWidget(minIco, 2, 0);
		formLayout->addWidget(minValue, 2, 1);
		owner->setLayout(formLayout);
	}
};

double MinMaxAvgWidget::average() const
{
	return d->avgValue->text().toDouble();
}

double MinMaxAvgWidget::maximum() const
{
	return d->maxValue->text().toDouble();
}
double MinMaxAvgWidget::minimum() const
{
	return d->minValue->text().toDouble();
}

MinMaxAvgWidget::MinMaxAvgWidget(QWidget *parent) : d(new MinMaxAvgWidgetPrivate(this))
{
}

MinMaxAvgWidget::~MinMaxAvgWidget()
{
}

void MinMaxAvgWidget::clear()
{
	d->avgValue->setText(QString());
	d->maxValue->setText(QString());
	d->minValue->setText(QString());
}

void MinMaxAvgWidget::setAverage(double average)
{
	d->avgValue->setText(QString::number(average));
}

void MinMaxAvgWidget::setMaximum(double maximum)
{
	d->maxValue->setText(QString::number(maximum));
}
void MinMaxAvgWidget::setMinimum(double minimum)
{
	d->minValue->setText(QString::number(minimum));
}

void MinMaxAvgWidget::setAverage(const QString &average)
{
	d->avgValue->setText(average);
}

void MinMaxAvgWidget::setMaximum(const QString &maximum)
{
	d->maxValue->setText(maximum);
}

void MinMaxAvgWidget::setMinimum(const QString &minimum)
{
	d->minValue->setText(minimum);
}

void MinMaxAvgWidget::overrideMinToolTipText(const QString &newTip)
{
	d->minIco->setToolTip(newTip);
	d->minValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideAvgToolTipText(const QString &newTip)
{
	d->avgIco->setToolTip(newTip);
	d->avgValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideMaxToolTipText(const QString &newTip)
{
	d->maxIco->setToolTip(newTip);
	d->maxValue->setToolTip(newTip);
}

RenumberDialog *RenumberDialog::instance()
{
	static RenumberDialog *self = new RenumberDialog(MainWindow::instance());
	return self;
}

void RenumberDialog::renumberOnlySelected(bool selected)
{
	if (selected && amount_selected == 1)
		ui.groupBox->setTitle(tr("New number"));
	else
		ui.groupBox->setTitle(tr("New starting number"));
	selectedOnly = selected;
}

void RenumberDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		QMap<int,int> renumberedDives;
		int i;
		struct dive *dive = NULL;
		for_each_dive (i, dive) {
			if (!selectedOnly || dive->selected)
				renumberedDives.insert(dive->id, dive->number);
		}
		UndoRenumberDives *undoCommand = new UndoRenumberDives(renumberedDives, ui.spinBox->value());
		MainWindow::instance()->undoStack->push(undoCommand);
	}
}

RenumberDialog::RenumberDialog(QWidget *parent) : QDialog(parent), selectedOnly(false)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

SetpointDialog *SetpointDialog::instance()
{
	static SetpointDialog *self = new SetpointDialog(MainWindow::instance());
	return self;
}

void SetpointDialog::setpointData(struct divecomputer *divecomputer, int second)
{
	dc = divecomputer;
	time = second < 0 ? 0 : second;
}

void SetpointDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
		add_event(dc, time, SAMPLE_EVENT_PO2, 0, (int)(1000.0 * ui.spinbox->value()), "SP change");
	mark_divelist_changed(true);
	MainWindow::instance()->graphics()->replot();
}

SetpointDialog::SetpointDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

ShiftTimesDialog *ShiftTimesDialog::instance()
{
	static ShiftTimesDialog *self = new ShiftTimesDialog(MainWindow::instance());
	return self;
}

void ShiftTimesDialog::buttonClicked(QAbstractButton *button)
{
	int amount;

	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			amount *= -1;
		if (amount != 0) {
			// DANGER, DANGER - this could get our dive_table unsorted...
			int i;
			struct dive *dive;
			QList<int> affectedDives;
			for_each_dive (i, dive) {
				if (!dive->selected)
					continue;

				affectedDives.append(dive->id);
			}
			MainWindow::instance()->undoStack->push(new UndoShiftTime(affectedDives, amount));
			sort_table(&dive_table);
			mark_divelist_changed(true);
			MainWindow::instance()->dive_list()->rememberSelection();
			MainWindow::instance()->refreshDisplay();
			MainWindow::instance()->dive_list()->restoreSelection();
		}
	}
}

void ShiftTimesDialog::showEvent(QShowEvent *event)
{
	ui.timeEdit->setTime(QTime(0, 0, 0, 0));
	when = get_times(); //get time of first selected dive
	ui.currentTime->setText(get_dive_date_string(when));
	ui.shiftedTime->setText(get_dive_date_string(when));
}

void ShiftTimesDialog::changeTime()
{
	int amount;

	amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
	if (ui.backwards->isChecked())
		amount *= -1;

	ui.shiftedTime->setText(get_dive_date_string(amount + when));
}

ShiftTimesDialog::ShiftTimesDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(changeTime()));
	connect(ui.backwards, SIGNAL(toggled(bool)), this, SLOT(changeTime()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void ShiftImageTimesDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		m_amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			m_amount *= -1;
	}
}

void ShiftImageTimesDialog::syncCameraClicked()
{
	timestamp_t timestamp;
	QPixmap picture;
	QDateTime dcDateTime = QDateTime();
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Open image file"),
							      DiveListView::lastUsedImageDir(),
							      tr("Image files (*.jpg *.jpeg *.pnm *.tif *.tiff)"));
	if (fileNames.isEmpty())
		return;

	picture.load(fileNames.at(0));
	ui.displayDC->setEnabled(true);
	QGraphicsScene *scene = new QGraphicsScene(this);

	scene->addPixmap(picture.scaled(ui.DCImage->size()));
	ui.DCImage->setScene(scene);

	picture_get_timestamp(fileNames.at(0).toUtf8().data(), &timestamp);
	dcImageEpoch = timestamp;
	dcDateTime.setTime_t(dcImageEpoch);
	ui.dcTime->setDateTime(dcDateTime);
	connect(ui.dcTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(dcDateTimeChanged(const QDateTime &)));
}

void ShiftImageTimesDialog::dcDateTimeChanged(const QDateTime &newDateTime)
{
	if (!dcImageEpoch)
		return;
	setOffset(newDateTime.toTime_t() - dcImageEpoch);
}

ShiftImageTimesDialog::ShiftImageTimesDialog(QWidget *parent, QStringList fileNames) : QDialog(parent), m_amount(0), fileNames(fileNames)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.syncCamera, SIGNAL(clicked()), this, SLOT(syncCameraClicked()));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime &)), this, SLOT(timeEditChanged(const QTime &)));
	dcImageEpoch = (time_t)0;
}

time_t ShiftImageTimesDialog::amount() const
{
	return m_amount;
}

void ShiftImageTimesDialog::setOffset(time_t offset)
{
	if (offset >= 0) {
		ui.forward->setChecked(true);
	} else {
		ui.backwards->setChecked(true);
		offset *= -1;
	}
	ui.timeEdit->setTime(QTime(offset / 3600, (offset % 3600) / 60, offset % 60));
}

void ShiftImageTimesDialog::updateInvalid()
{
	timestamp_t timestamp;
	QDateTime time;
	bool allValid = true;
	ui.warningLabel->hide();
	ui.invalidLabel->hide();
	ui.invalidLabel->clear();

	Q_FOREACH (const QString &fileName, fileNames) {
		if (picture_check_valid(fileName.toUtf8().data(), m_amount))
			continue;

		// We've found invalid image
		picture_get_timestamp(fileName.toUtf8().data(), &timestamp);
		dcImageEpoch = timestamp;
		time.setTime_t(timestamp + m_amount);
		ui.invalidLabel->setText(ui.invalidLabel->text() + fileName + " " + time.toString() + "\n");
		allValid = false;
	}

	if (!allValid){
		ui.warningLabel->show();
		ui.invalidLabel->show();
	}
}

void ShiftImageTimesDialog::timeEditChanged(const QTime &time)
{
	m_amount = time.hour() * 3600 + time.minute() * 60;
	if (ui.backwards->isChecked())
			m_amount *= -1;
	updateInvalid();
}

bool isGnome3Session()
{
#if defined(QT_OS_WIW) || defined(QT_OS_MAC)
	return false;
#else
	if (qApp->style()->objectName() != "gtk+")
		return false;
	QProcess p;
	p.start("pidof", QStringList() << "gnome-shell");
	p.waitForFinished(-1);
	QString p_stdout = p.readAllStandardOutput();
	return !p_stdout.isEmpty();
#endif
}

DateWidget::DateWidget(QWidget *parent) : QWidget(parent),
	calendarWidget(new QCalendarWidget())
{
	setDate(QDate::currentDate());
	setMinimumSize(QSize(80, 64));
	setFocusPolicy(Qt::StrongFocus);
	calendarWidget->setWindowFlags(Qt::FramelessWindowHint);
	calendarWidget->setFirstDayOfWeek(getLocale().firstDayOfWeek());
	calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

	connect(calendarWidget, SIGNAL(activated(QDate)), calendarWidget, SLOT(hide()));
	connect(calendarWidget, SIGNAL(clicked(QDate)), calendarWidget, SLOT(hide()));
	connect(calendarWidget, SIGNAL(activated(QDate)), this, SLOT(setDate(QDate)));
	connect(calendarWidget, SIGNAL(clicked(QDate)), this, SLOT(setDate(QDate)));
	calendarWidget->installEventFilter(this);
}

bool DateWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::FocusOut) {
		calendarWidget->hide();
		return true;
	}
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *ev = static_cast<QKeyEvent *>(event);
		if (ev->key() == Qt::Key_Escape) {
			calendarWidget->hide();
		}
	}
	return QObject::eventFilter(object, event);
}


void DateWidget::setDate(const QDate &date)
{
	mDate = date;
	update();
	emit dateChanged(mDate);
}

QDate DateWidget::date() const
{
	return mDate;
}

void DateWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::EnabledChange) {
		update();
	}
}

#define DATEWIDGETWIDTH 80
void DateWidget::paintEvent(QPaintEvent *event)
{
	static QPixmap pix = QPixmap(":/calendar").scaled(DATEWIDGETWIDTH, 64);

	QPainter painter(this);

	painter.drawPixmap(QPoint(0, 0), isEnabled() ? pix : QPixmap::fromImage(grayImage(pix.toImage())));

	QString month = mDate.toString("MMM");
	QString year = mDate.toString("yyyy");
	QString day = mDate.toString("dd");

	QFont font = QFont("monospace", 10);
	QFontMetrics metrics = QFontMetrics(font);
	painter.setFont(font);
	painter.setPen(QPen(QBrush(Qt::white), 0));
	painter.setBrush(QBrush(Qt::white));
	painter.drawText(QPoint(6, metrics.height() + 1), month);
	painter.drawText(QPoint(DATEWIDGETWIDTH - metrics.width(year) - 6, metrics.height() + 1), year);

	font.setPointSize(14);
	metrics = QFontMetrics(font);
	painter.setPen(QPen(QBrush(Qt::black), 0));
	painter.setBrush(Qt::black);
	painter.setFont(font);
	painter.drawText(QPoint(DATEWIDGETWIDTH / 2 - metrics.width(day) / 2, 45), day);

	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		option.backgroundColor = palette().color(QPalette::Background);
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
	}
}

void DateWidget::mousePressEvent(QMouseEvent *event)
{
	calendarWidget->setSelectedDate(mDate);
	calendarWidget->move(event->globalPos());
	calendarWidget->show();
	calendarWidget->raise();
	calendarWidget->setFocus();
}

void DateWidget::focusInEvent(QFocusEvent *event)
{
	setFocus();
	QWidget::focusInEvent(event);
}

void DateWidget::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);
}

void DateWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return ||
	    event->key() == Qt::Key_Enter ||
	    event->key() == Qt::Key_Space) {
		calendarWidget->move(mapToGlobal(QPoint(0, 64)));
		calendarWidget->show();
		event->setAccepted(true);
	} else {
		QWidget::keyPressEvent(event);
	}
}

#define COMPONENT_FROM_UI(_component) what->_component = ui._component->isChecked()
#define UI_FROM_COMPONENT(_component) ui._component->setChecked(what->_component)

DiveComponentSelection::DiveComponentSelection(QWidget *parent, struct dive *target, struct dive_components *_what) : targetDive(target)
{
	ui.setupUi(this);
	what = _what;
	UI_FROM_COMPONENT(divesite);
	UI_FROM_COMPONENT(divemaster);
	UI_FROM_COMPONENT(buddy);
	UI_FROM_COMPONENT(rating);
	UI_FROM_COMPONENT(visibility);
	UI_FROM_COMPONENT(notes);
	UI_FROM_COMPONENT(suit);
	UI_FROM_COMPONENT(tags);
	UI_FROM_COMPONENT(cylinders);
	UI_FROM_COMPONENT(weights);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DiveComponentSelection::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		COMPONENT_FROM_UI(divesite);
		COMPONENT_FROM_UI(divemaster);
		COMPONENT_FROM_UI(buddy);
		COMPONENT_FROM_UI(rating);
		COMPONENT_FROM_UI(visibility);
		COMPONENT_FROM_UI(notes);
		COMPONENT_FROM_UI(suit);
		COMPONENT_FROM_UI(tags);
		COMPONENT_FROM_UI(cylinders);
		COMPONENT_FROM_UI(weights);
		selective_copy_dive(&displayed_dive, targetDive, *what, true);
	}
}

TagFilter::TagFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Tags: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(TagFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void TagFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(TagFilterModel::instance());
	QWidget::showEvent(event);
}

void TagFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(TagFilterModel::instance());
	QWidget::hideEvent(event);
}

BuddyFilter::BuddyFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Person: "));
	ui.label->setToolTip(tr("Searches for buddies and divemasters"));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(BuddyFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void BuddyFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(BuddyFilterModel::instance());
	QWidget::showEvent(event);
}

void BuddyFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(BuddyFilterModel::instance());
	QWidget::hideEvent(event);
}

LocationFilter::LocationFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Location: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(LocationFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void LocationFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(LocationFilterModel::instance());
	QWidget::showEvent(event);
}

void LocationFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(LocationFilterModel::instance());
	QWidget::hideEvent(event);
}

SuitFilter::SuitFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Suits: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(SuitsFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void SuitFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(SuitsFilterModel::instance());
	QWidget::showEvent(event);
}

void SuitFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(SuitsFilterModel::instance());
	QWidget::hideEvent(event);
}

MultiFilter::MultiFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	QWidget *expandedWidget = new QWidget();
	QHBoxLayout *l = new QHBoxLayout();

	TagFilter *tagFilter = new TagFilter(this);
	int minimumHeight = tagFilter->ui.filterInternalList->height() +
			tagFilter->ui.verticalLayout->spacing() * tagFilter->ui.verticalLayout->count();

	QListView *dummyList = new QListView();
	QStringListModel *dummy = new QStringListModel(QStringList() << "Dummy Text");
	dummyList->setModel(dummy);

	connect(ui.close, SIGNAL(clicked(bool)), this, SLOT(closeFilter()));
	connect(ui.clear, SIGNAL(clicked(bool)), MultiFilterSortModel::instance(), SLOT(clearFilter()));
	connect(ui.maximize, SIGNAL(clicked(bool)), this, SLOT(adjustHeight()));

	l->addWidget(tagFilter);
	l->addWidget(new BuddyFilter());
	l->addWidget(new LocationFilter());
	l->addWidget(new SuitFilter());
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	expandedWidget->setLayout(l);

	ui.scrollArea->setWidget(expandedWidget);
	expandedWidget->resize(expandedWidget->width(), minimumHeight + dummyList->sizeHintForRow(0) * 5 );
	ui.scrollArea->setMinimumHeight(expandedWidget->height() + 5);

	connect(MultiFilterSortModel::instance(), SIGNAL(filterFinished()), this, SLOT(filterFinished()));
}

void MultiFilter::filterFinished()
{
	ui.filterText->setText(tr("Filter shows %1 (of %2) dives").arg(MultiFilterSortModel::instance()->divesDisplayed).arg(dive_table.nr));
}

void MultiFilter::adjustHeight()
{
	ui.scrollArea->setVisible(!ui.scrollArea->isVisible());
}

void MultiFilter::closeFilter()
{
	MultiFilterSortModel::instance()->clearFilter();
	hide();
}
#include <QDebug>
#include <QShowEvent>

LocationInformationWidget::LocationInformationWidget(QWidget *parent) : QGroupBox(parent), modified(false)
{
	ui.setupUi(this);
	ui.diveSiteMessage->setCloseButtonVisible(false);
	ui.diveSiteMessage->show();

	// create the three buttons and only show the close button for now
	closeAction = new QAction(tr("Close"), this);
	connect(closeAction, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));

	acceptAction = new QAction(tr("Apply changes"), this);
	connect(acceptAction, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));

	rejectAction = new QAction(tr("Discard changes"), this);
	connect(rejectAction, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(closeAction);
}

void LocationInformationWidget::setLocationId(uint32_t uuid)
{
	currentDs = get_dive_site_by_uuid(uuid);

	if (!currentDs) {
		currentDs = get_dive_site_by_uuid(create_dive_site(""));
		displayed_dive.dive_site_uuid = currentDs->uuid;
		ui.diveSiteName->clear();
		ui.diveSiteDescription->clear();
		ui.diveSiteNotes->clear();
		ui.diveSiteCoordinates->clear();
	}
	displayed_dive_site = *currentDs;
	if (displayed_dive_site.name)
		ui.diveSiteName->setText(displayed_dive_site.name);
	else
		ui.diveSiteName->clear();
	if (displayed_dive_site.description)
		ui.diveSiteDescription->setText(displayed_dive_site.description);
	else
		ui.diveSiteDescription->clear();
	if (displayed_dive_site.notes)
		ui.diveSiteNotes->setPlainText(displayed_dive_site.notes);
	else
		ui.diveSiteNotes->clear();
	if (displayed_dive_site.latitude.udeg || displayed_dive_site.longitude.udeg)
		ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
	else
		ui.diveSiteCoordinates->clear();
}

void LocationInformationWidget::updateGpsCoordinates()
{
	ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
	MainWindow::instance()->setApplicationState("EditDiveSite");
}

void LocationInformationWidget::acceptChanges()
{
	char *uiString;
	currentDs->latitude = displayed_dive_site.latitude;
	currentDs->longitude = displayed_dive_site.longitude;
	uiString = ui.diveSiteName->text().toUtf8().data();
	if (!same_string(uiString, currentDs->name)) {
		free(currentDs->name);
		currentDs->name = copy_string(uiString);
	}
	uiString = ui.diveSiteDescription->text().toUtf8().data();
	if (!same_string(uiString, currentDs->description)) {
		free(currentDs->description);
		currentDs->description = copy_string(uiString);
	}
	uiString = ui.diveSiteNotes->document()->toPlainText().toUtf8().data();
	if (!same_string(uiString, currentDs->notes)) {
		free(currentDs->notes);
		currentDs->notes = copy_string(uiString);
	}
	if (dive_site_is_empty(currentDs)) {
		delete_dive_site(currentDs->uuid);
		displayed_dive.dive_site_uuid = 0;
		setLocationId(0);
	} else {
		setLocationId(currentDs->uuid);
	}
	mark_divelist_changed(true);
	resetState();
	emit informationManagementEnded();
}

void LocationInformationWidget::rejectChanges()
{
	Q_ASSERT(currentDs != NULL);
	if (dive_site_is_empty(currentDs)) {
		delete_dive_site(currentDs->uuid);
		displayed_dive.dive_site_uuid = 0;
		setLocationId(0);
	} else {
		setLocationId(currentDs->uuid);
	}
	resetState();
	emit informationManagementEnded();
}

void LocationInformationWidget::showEvent(QShowEvent *ev) {
	ui.diveSiteMessage->setCloseButtonVisible(false);
}

void LocationInformationWidget::markChangedWidget(QWidget *w)
{
	QPalette p;
	qreal h, s, l, a;
	if (!modified)
		enableEdition();
	qApp->palette().color(QPalette::Text).getHslF(&h, &s, &l, &a);
	p.setBrush(QPalette::Base, (l <= 0.3) ? QColor(Qt::yellow).lighter() : (l <= 0.6) ? QColor(Qt::yellow).light() : /* else */ QColor(Qt::yellow).darker(300));
	w->setPalette(p);
	modified = true;
}

void LocationInformationWidget::resetState()
{
	modified = false;
	resetPallete();
	MainWindow::instance()->dive_list()->setEnabled(true);
	MainWindow::instance()->setEnabledToolbar(true);
	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(closeAction);
	ui.diveSiteMessage->removeAction(acceptAction);
	ui.diveSiteMessage->removeAction(rejectAction);
	ui.diveSiteMessage->setCloseButtonVisible(false);
}

void LocationInformationWidget::enableEdition()
{
	MainWindow::instance()->dive_list()->setEnabled(false);
	MainWindow::instance()->setEnabledToolbar(false);
	ui.diveSiteMessage->setText(tr("You are editing a dive site"));
	ui.diveSiteMessage->removeAction(closeAction);
	ui.diveSiteMessage->addAction(acceptAction);
	ui.diveSiteMessage->addAction(rejectAction);
	ui.diveSiteMessage->setCloseButtonVisible(false);
}

void LocationInformationWidget::on_diveSiteCoordinates_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), printGPSCoords(currentDs->latitude.udeg, currentDs->longitude.udeg)))
		markChangedWidget(ui.diveSiteCoordinates);
}

void LocationInformationWidget::on_diveSiteDescription_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), currentDs->description))
		markChangedWidget(ui.diveSiteDescription);
}

void LocationInformationWidget::on_diveSiteName_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), currentDs->name))
		markChangedWidget(ui.diveSiteName);
}

void LocationInformationWidget::on_diveSiteNotes_textChanged()
{
	if (!same_string(qPrintable(ui.diveSiteNotes->toPlainText()),  currentDs->notes))
		markChangedWidget(ui.diveSiteNotes);
}

void LocationInformationWidget::resetPallete()
{
	QPalette p;
	ui.diveSiteCoordinates->setPalette(p);
	ui.diveSiteDescription->setPalette(p);
	ui.diveSiteName->setPalette(p);
	ui.diveSiteNotes->setPalette(p);
}
