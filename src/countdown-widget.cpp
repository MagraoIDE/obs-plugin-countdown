#include "countdown-widget.hpp"

CountdownDockWidget::CountdownDockWidget(QWidget *parent)
	: QDockWidget("Countdown Timer", parent), ui(new Ui::CountdownTimer)
{
	countdownTimerData = new CountdownWidgetStruct;
	ui->setupUi(this);



	obs_frontend_add_event_callback(OBSFrontendEventHandler,
					ui);

	ConnectUISignalHandlers();

	ConnectObsSignalHandlers();

	InitialiseTimerTime();
	hide();
}

CountdownDockWidget::~CountdownDockWidget()
{
	SaveSettings();
}

void CountdownDockWidget::SetupCountdownWidgetUI()
{

	CountdownWidgetStruct *context = countdownTimerData;
	// context->ui->timerDisplay = new QLCDNumber(8);
	ui->timerDisplay->display("00:00:00");

	// ui->timerHours = new QLineEdit("0");
	ui->timerHours->setAlignment(Qt::AlignCenter);
	ui->timerHours->setMaxLength(2);
	ui->timerHours->setValidator(new QRegularExpressionValidator(
		QRegularExpression("[0-9]{1,2}"), this));

	// ui->timerMinutes = new QLineEdit("0");
	ui->timerMinutes->setAlignment(Qt::AlignCenter);
	ui->timerMinutes->setMaxLength(2);
	ui->timerMinutes->setValidator(new QRegularExpressionValidator(
		QRegularExpression("^[1-5]?[0-9]"), this));

	// ui->timerSeconds = new QLineEdit("0");
	ui->timerSeconds->setAlignment(Qt::AlignCenter);
	ui->timerSeconds->setMaxLength(2);
	ui->timerSeconds->setValidator(new QRegularExpressionValidator(
		QRegularExpression("^[1-5]?[0-9]"), this));

	// ui->textSourceDropdownList = new QComboBox();
	ui->textSourceDropdownList->setToolTip(obs_module_text("TextSourceDropdownTip"));

	// ui->switchSceneCheckBox = new QCheckBox();
	ui->switchSceneCheckBox->setCheckState(Qt::Unchecked);
	ui->switchSceneCheckBox->setToolTip(obs_module_text("SwitchSceneCheckBoxTip"));

	// ui->sceneSourceDropdownList = new QComboBox();
	ui->sceneSourceDropdownList->setEnabled(false);
	ui->sceneSourceDropdownList->setToolTip(obs_module_text("SceneSourceDropdownTip"));

	// ui->playButton = new QPushButton(this);
	ui->playButton->setProperty("themeID", "playIcon");
	ui->playButton->setEnabled(true);
	ui->playButton->setToolTip(obs_module_text("PlayButton"));

	// ui->pauseButton = new QPushButton(this);
	ui->pauseButton->setProperty("themeID", "pauseIcon");
	ui->pauseButton->setEnabled(false);
	ui->pauseButton->setToolTip(obs_module_text("PauseButton"));

	// ui->resetButton = new QPushButton(this);
	ui->resetButton->setProperty("themeID", "restartIcon");
	ui->resetButton->setToolTip(obs_module_text("ResetButton"));

	context->isPlaying = false;

	// QHBoxLayout *timerLayout = new QHBoxLayout();

	// timerLayout->addWidget(ui->timerHours);
	// timerLayout->addWidget(new QLabel("h"));

	// timerLayout->addWidget(ui->timerMinutes);
	// timerLayout->addWidget(new QLabel("m"));

	// timerLayout->addWidget(ui->timerSeconds);
	// timerLayout->addWidget(new QLabel("s"));

	// QHBoxLayout *buttonLayout = new QHBoxLayout();

	// buttonLayout->addWidget(ui->resetButton);
	// buttonLayout->addWidget(ui->pauseButton);
	// buttonLayout->addWidget(ui->playButton);

	// QHBoxLayout *sourceDropdownLayout = new QHBoxLayout();
	// sourceDropdownLayout->addWidget(new QLabel(obs_module_text("TextSourceLabel")));
	// sourceDropdownLayout->addWidget(ui->textSourceDropdownList);

	// QHBoxLayout *endMessageLayout = new QHBoxLayout();
	// ui->endMessageCheckBox = new QCheckBox();
	ui->endMessageCheckBox->setCheckState(Qt::Unchecked);
	ui->endMessageCheckBox->setToolTip(obs_module_text("EndMessageCheckBoxTip"));

	// ui->timerEndMessage = new QLineEdit();
	ui->timerEndMessage->setEnabled(false);
	ui->timerEndMessage->setToolTip(obs_module_text("EndMessageLineEditTip"));

	ui->timerEndLabel->setText(obs_module_text("EndMessageLabel"));
	ui->timerEndLabel->setEnabled(false);
	// endMessageLayout->addWidget(ui->endMessageCheckBox);
	// endMessageLayout->addWidget(ui->timerEndLabel);
	// endMessageLayout->addWidget(ui->timerEndMessage);

	// QHBoxLayout *sceneDropdownLayout = new QHBoxLayout();
	ui->sceneSwitchLabel->setText(obs_module_text("SwitchScene"));
	// sceneDropdownLayout->addWidget(ui->switchSceneCheckBox);
	// sceneDropdownLayout->addWidget(ui->sceneSwitchLabel);
	// sceneDropdownLayout->addWidget(ui->sceneSourceDropdownList);

	// QVBoxLayout *timeLayout = new QVBoxLayout();

	// timeLayout->addWidget(ui->timerDisplay);
	// timeLayout->addLayout(timerLayout);

	// QVBoxLayout *mainLayout = new QVBoxLayout();
	// mainLayout->addLayout(timeLayout);
	// mainLayout->addLayout(sourceDropdownLayout);
	// mainLayout->addLayout(endMessageLayout);
	// mainLayout->addLayout(sceneDropdownLayout);
	// mainLayout->addLayout(buttonLayout);

	// mainLayout->setGeometry(QRect(0,0,200,300));

	// return mainLayout;
}

void CountdownDockWidget::ConnectUISignalHandlers()
{
	QObject::connect(ui->switchSceneCheckBox,
			 SIGNAL(stateChanged(int)),
			 SLOT(SceneSwitchCheckBoxSelected(int)));

	QObject::connect(ui->playButton, SIGNAL(clicked()),
			 SLOT(PlayButtonClicked()));

	QObject::connect(ui->pauseButton, SIGNAL(clicked()),
			 SLOT(PauseButtonClicked()));

	QObject::connect(ui->resetButton, SIGNAL(clicked()),
			 SLOT(ResetButtonClicked()));

	QObject::connect(ui->endMessageCheckBox, SIGNAL(stateChanged(int)),
			 SLOT(EndMessageCheckBoxSelected(int)));

	QObject::connect(ui->textSourceDropdownList,
			 SIGNAL(currentTextChanged(QString)),
			 SLOT(HandleTextSourceChange(QString)));

	QObject::connect(ui->sceneSourceDropdownList,
			 SIGNAL(currentTextChanged(QString)),
			 SLOT(HandleSceneSourceChange(QString)));
}

void CountdownDockWidget::PlayButtonClicked()
{
	if (IsSetTimeZero())
		return;

	ui->timerDisplay->display(
		ConvertTimeToDisplayString(countdownTimerData->time));
	StartTimerCounting();
}

void CountdownDockWidget::PauseButtonClicked()
{
	StopTimerCounting();
}

void CountdownDockWidget::ResetButtonClicked()
{
	int hours = ui->timerHours->text().toInt();
	int minutes = ui->timerMinutes->text().toInt();
	int seconds = ui->timerSeconds->text().toInt();

	StopTimerCounting();

	countdownTimerData->time->setHMS(hours, minutes, seconds, 0);

	UpdateTimeDisplay();
}

void CountdownDockWidget::StartTimerCounting()
{
	countdownTimerData->isPlaying = true;
	countdownTimerData->timer->start(COUNTDOWNPERIOD);
	ui->playButton->setEnabled(false);
	ui->pauseButton->setEnabled(true);
	ui->resetButton->setEnabled(false);

	ui->timerHours->setEnabled(false);
	ui->timerMinutes->setEnabled(false);
	ui->timerSeconds->setEnabled(false);

	ui->textSourceDropdownList->setEnabled(false);
	ui->timerEndMessage->setEnabled(false);
	ui->sceneSourceDropdownList->setEnabled(false);
	ui->endMessageCheckBox->setEnabled(false);
	ui->switchSceneCheckBox->setEnabled(false);
}

void CountdownDockWidget::StopTimerCounting()
{
	countdownTimerData->isPlaying = false;
	countdownTimerData->timer->stop();
	ui->playButton->setEnabled(true);
	ui->pauseButton->setEnabled(false);
	ui->resetButton->setEnabled(true);

	ui->timerHours->setEnabled(true);
	ui->timerMinutes->setEnabled(true);
	ui->timerSeconds->setEnabled(true);

	ui->textSourceDropdownList->setEnabled(true);

	ui->endMessageCheckBox->setEnabled(true);
	if (ui->endMessageCheckBox->isChecked()) {
		ui->timerEndMessage->setEnabled(true);
	}
	ui->switchSceneCheckBox->setEnabled(true);
	if (ui->switchSceneCheckBox->isChecked()) {
		ui->sceneSourceDropdownList->setEnabled(true);
	}

}

void CountdownDockWidget::InitialiseTimerTime()
{
	countdownTimerData->timer = new QTimer();
	QObject::connect(countdownTimerData->timer, SIGNAL(timeout()),
			 SLOT(TimerDecrement()));
	countdownTimerData->time = new QTime(ui->timerHours->text().toInt(),
				  ui->timerMinutes->text().toInt(),
				  ui->timerSeconds->text().toInt());
}

void CountdownDockWidget::TimerDecrement()
{
	CountdownWidgetStruct *context = countdownTimerData;

	QTime *currentTime = context->time;

	currentTime->setHMS(currentTime->addMSecs(-COUNTDOWNPERIOD).hour(),
			    currentTime->addMSecs(-COUNTDOWNPERIOD).minute(),
			    currentTime->addMSecs(-COUNTDOWNPERIOD).second());

	UpdateTimeDisplay();

	if (currentTime->hour() == 0 && currentTime->minute() == 0 &&
	    currentTime->second() == 0) {
		QString endMessageText = ui->timerEndMessage->text();
		if (ui->endMessageCheckBox->isChecked()) {
			SetSourceText(endMessageText.toStdString().c_str());
		}
		if (ui->switchSceneCheckBox->isChecked()) {
			SetCurrentScene();
		}
		ui->timerDisplay->display("00:00:00");
		currentTime->setHMS(0, 0, 0, 0);
		StopTimerCounting();
		return;
	}
}

QString CountdownDockWidget::ConvertTimeToDisplayString(QTime *timeToConvert)
{
	return timeToConvert->toString("hh:mm:ss");
}

bool CountdownDockWidget::IsSetTimeZero()
{
	bool isZero = false;

	if (countdownTimerData->time->hour() == 0 && countdownTimerData->time->minute() == 0 &&
	    countdownTimerData->time->second() == 0) {
		isZero = true;
	} else if (ui->timerHours->text().toInt() == 0 &&
		   ui->timerMinutes->text().toInt() == 0 &&
		   ui->timerSeconds->text().toInt() == 0) {
		isZero = true;
	}

	return isZero;
}

void CountdownDockWidget::OBSFrontendEventHandler(enum obs_frontend_event event,
						  void *private_data)
{

	Ui::CountdownTimer *ui = (Ui::CountdownTimer *)private_data;

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING: {
		// CountdownDockWidget::ConnectUISignalHandlers(context);
		CountdownDockWidget::LoadSavedSettings(ui);
	} break;
	default:
		break;
	}
}

void CountdownDockWidget::ConnectObsSignalHandlers()
{
	// Source Signals
	signal_handler_connect(obs_get_signal_handler(), "source_create",
			       OBSSourceCreated, ui);

	signal_handler_connect(obs_get_signal_handler(), "source_destroy",
			       OBSSourceDeleted, ui);

	signal_handler_connect(obs_get_signal_handler(), "source_rename",
			       OBSSourceRenamed, ui);
}

void CountdownDockWidget::OBSSourceCreated(void *param, calldata_t *calldata)
{
	auto ui = static_cast<Ui::CountdownTimer *>(param);
	obs_source_t *source;
	calldata_get_ptr(calldata, "source", &source);

	if (!source)
		return;
	int sourceType = CheckSourceType(source);
	// If not sourceType we need;
	if (!sourceType)
		return;

	const char *name = obs_source_get_name(source);

	if (sourceType == TEXT_SOURCE) {
		ui->textSourceDropdownList->addItem(name);
	} else if (sourceType == SCENE_SOURCE) {
		ui->sceneSourceDropdownList->addItem(name);
	}
};

void CountdownDockWidget::OBSSourceDeleted(void *param, calldata_t *calldata)
{
	auto ui = static_cast<Ui::CountdownTimer *>(param);

	obs_source_t *source;

	calldata_get_ptr(calldata, "source", &source);

	if (!source)
		return;
	int sourceType = CheckSourceType(source);
	// If not sourceType we need;
	if (!sourceType)
		return;

	const char *name = obs_source_get_name(source);

	if (sourceType == TEXT_SOURCE) {
		int textIndexToRemove =
			ui->textSourceDropdownList->findText(name);
		ui->textSourceDropdownList->removeItem(textIndexToRemove);
	} else if (sourceType == SCENE_SOURCE) {
		int sceneIndexToRemove =
			ui->sceneSourceDropdownList->findText(name);
		ui->sceneSourceDropdownList->removeItem(
			sceneIndexToRemove);
	}

	obs_source_release(source);
};

void CountdownDockWidget::OBSSourceRenamed(void *param, calldata_t *calldata)
{
	auto ui = static_cast<Ui::CountdownTimer *>(param);

	obs_source_t *source;
	calldata_get_ptr(calldata, "source", &source);

	if (!source)
		return;
	int sourceType = CheckSourceType(source);
	// If not sourceType we need;
	if (!sourceType)
		return;

	const char *newName = calldata_string(calldata, "new_name");
	const char *oldName = calldata_string(calldata, "prev_name");

	if (sourceType == TEXT_SOURCE) {
		int textListIndex =
			ui->textSourceDropdownList->findText(oldName);
		if (textListIndex == -1)
			return;
		ui->textSourceDropdownList->setItemText(textListIndex,
							     newName);
	} else if (sourceType == SCENE_SOURCE) {
		int sceneListIndex =
			ui->sceneSourceDropdownList->findText(oldName);
		if (sceneListIndex == -1)
			return;
		ui->sceneSourceDropdownList->setItemText(sceneListIndex,
							      newName);
	}
};

int CountdownDockWidget::CheckSourceType(obs_source_t *source)
{
	const char *source_id = obs_source_get_unversioned_id(source);
	if (strcmp(source_id, "text_ft2_source") == 0 ||
	    strcmp(source_id, "text_gdiplus") == 0) {
		return TEXT_SOURCE;
	} else if (strcmp(source_id, "scene") == 0) {
		return SCENE_SOURCE;
	}
	return 0;
}

void CountdownDockWidget::UpdateTimeDisplay()
{

	QString formattedTime = countdownTimerData->time->toString("hh:mm:ss");
	ui->timerDisplay->display(formattedTime);
	SetSourceText(formattedTime);
}

void CountdownDockWidget::SetSourceText(QString newText)
{

	QString currentSourceNameString =
		ui->textSourceDropdownList->currentText();

	obs_source_t *selectedSource = obs_get_source_by_name(
		currentSourceNameString.toStdString().c_str());

	if (selectedSource != NULL) {
		obs_data_t *sourceSettings =
			obs_source_get_settings(selectedSource);
		obs_data_set_string(sourceSettings, "text",
				    newText.toStdString().c_str());
		obs_source_update(selectedSource, sourceSettings);
		obs_data_release(sourceSettings);
		obs_source_release(selectedSource);
	}
}

void CountdownDockWidget::EndMessageCheckBoxSelected(int state)
{
	if (state) {
		ui->timerEndMessage->setEnabled(true);
		ui->timerEndLabel->setEnabled(true);
	} else {
		ui->timerEndMessage->setEnabled(false);
		ui->timerEndLabel->setEnabled(false);
	}
}

void CountdownDockWidget::SceneSwitchCheckBoxSelected(int state)
{
	if (state) {
		ui->sceneSourceDropdownList->setEnabled(true);
		ui->sceneSwitchLabel->setEnabled(true);
	} else {
		ui->sceneSourceDropdownList->setEnabled(false);
		ui->sceneSwitchLabel->setEnabled(false);
	}
}

void CountdownDockWidget::SetCurrentScene()
{
	QString selectedScene =
		ui->sceneSourceDropdownList->currentText();
	if (selectedScene.length()) {
		obs_source_t *source = obs_get_source_by_name(
			selectedScene.toStdString().c_str());
		if (source != NULL) {
			obs_frontend_set_current_scene(source);
			obs_source_release(source);
		}
	}
}

void CountdownDockWidget::LoadSavedSettings(Ui::CountdownTimer *ui)
{
	char *file = obs_module_config_path("config.json");
	obs_data_t *data = nullptr;
	if (file) {
		data = obs_data_create_from_json_file(file);
		bfree(file);
	}
	if (data) {
		// Get Save Data
		int hours = obs_data_get_int(data, "hours");
		int minutes = obs_data_get_int(data, "minutes");
		int seconds = obs_data_get_int(data, "seconds");

		const char *selectedTextSource =
			obs_data_get_string(data, "selectedTextSource");

		int endMessageCheckBoxStatus =
			obs_data_get_int(data, "endMessageCheckBoxStatus");

		const char *endMessageText =
			obs_data_get_string(data, "endMessageText");

		int switchSceneCheckBoxStatus =
			obs_data_get_int(data, "switchSceneCheckBoxStatus");

		const char *selectedSceneSource =
			obs_data_get_string(data, "selectedSceneSource");

		UNUSED_PARAMETER(selectedTextSource);
		UNUSED_PARAMETER(selectedSceneSource);

		// Apply saved data to plugin
		ui->timerHours->setText(QString::number(hours));
		ui->timerMinutes->setText(QString::number(minutes));
		ui->timerSeconds->setText(QString::number(seconds));
		ui->timerEndMessage->setText(endMessageText);

		ui->endMessageCheckBox->setCheckState(
			(Qt::CheckState)endMessageCheckBoxStatus);

		ui->switchSceneCheckBox->setCheckState(
			(Qt::CheckState)switchSceneCheckBoxStatus);

		int textSelectIndex = ui->textSourceDropdownList->findText(
			selectedTextSource);
		if (textSelectIndex != -1)
			ui->textSourceDropdownList->setCurrentIndex(
				textSelectIndex);

		int sceneSelectIndex =
			ui->sceneSourceDropdownList->findText(
				selectedSceneSource);
		if (sceneSelectIndex != -1)
			ui->sceneSourceDropdownList->setCurrentIndex(
				sceneSelectIndex);

		obs_data_release(data);
	}
}

void CountdownDockWidget::SaveSettings()
{
	obs_data_t *data = obs_data_create();

	int hours = ui->timerHours->text().toInt();
	obs_data_set_int(data, "hours", hours);
	int minutes = ui->timerMinutes->text().toInt();
	obs_data_set_int(data, "minutes", minutes);
	int seconds = ui->timerSeconds->text().toInt();
	obs_data_set_int(data, "seconds", seconds);

	obs_data_set_string(data, "selectedTextSource",
			    countdownTimerData->textSourceNameText.c_str());

	int endMessageCheckBoxStatus =
		ui->endMessageCheckBox->checkState();
	obs_data_set_int(data, "endMessageCheckBoxStatus",
			 endMessageCheckBoxStatus);

	QString timerEndMessage = ui->timerEndMessage->text();
	obs_data_set_string(data, "endMessageText",
			    timerEndMessage.toStdString().c_str());

	int switchSceneCheckBoxStatus =
		ui->switchSceneCheckBox->checkState();
	obs_data_set_int(data, "switchSceneCheckBoxStatus",
			 switchSceneCheckBoxStatus);

	obs_data_set_string(data, "selectedSceneSource",
			    countdownTimerData->sceneSourceNameText.c_str());

	char *file = obs_module_config_path(CONFIG);
	obs_data_save_json(data, file);
	obs_data_release(data);
	bfree(file);

	deleteLater();
}

const char *CountdownDockWidget::ConvertToConstChar(QString value)
{
	QByteArray ba = value.toLocal8Bit();
	const char *cString = ba.data();
	return cString;
}

void CountdownDockWidget::HandleTextSourceChange(QString newText)
{
	std::string textSourceSelected = newText.toStdString();
	countdownTimerData->textSourceNameText = textSourceSelected;
}

void CountdownDockWidget::HandleSceneSourceChange(QString newText)
{
	std::string sceneSourceSelected = newText.toStdString();
	countdownTimerData->sceneSourceNameText = sceneSourceSelected;
}