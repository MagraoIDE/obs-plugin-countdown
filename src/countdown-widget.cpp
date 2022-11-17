#include "countdown-widget.hpp"

CountdownDockWidget::CountdownDockWidget(QWidget *parent)
	: QDockWidget("Countdown Timer", parent), ui(new Ui::CountdownTimer)
{
	countdownTimerData = new CountdownWidgetStruct;
	// countdownTimerData->countdownTimerUI = new QWidget();
	// countdownTimerData->countdownTimerUI->setLayout(SetupCountdownWidgetUI(countdownTimerData));

	// setWidget(countdownTimerData->countdownTimerUI);
	// setFeatures(QDockWidget::DockWidgetClosable |
	// 	    QDockWidget::DockWidgetMovable |
	// 	    QDockWidget::DockWidgetFloatable);

	ui->setupUi(this);

	SetupCountdownWidgetUI(countdownTimerData);

	setVisible(false);
	setFloating(true);
	resize(300, 380);

	obs_frontend_add_event_callback(OBSFrontendEventHandler,
					ui);

	ConnectUISignalHandlers();

	ConnectObsSignalHandlers();

	InitialiseTimerTime(countdownTimerData);

	RegisterHotkeys(countdownTimerData);
}

CountdownDockWidget::~CountdownDockWidget()
{
	SaveSettings();
	UnregisterHotkeys();
}

void CountdownDockWidget::SetupCountdownWidgetUI(
	CountdownWidgetStruct *countdownStruct)
{

	CountdownWidgetStruct *context = countdownStruct;
	ui->timeDisplay->display("00:00:00");

	ui->hoursCheckBox->setText(obs_module_text("HoursCheckboxLabel"));
	ui->hoursCheckBox->setCheckState(Qt::Checked);
	ui->hoursCheckBox->setToolTip(obs_module_text("HoursCheckBoxTip"));
	ui->timerHours->setMaxLength(2);
	ui->timerHours->setValidator(new QRegularExpressionValidator(
		QRegularExpression("^[0-2]?[0-3]"), this));

	ui->minutesCheckBox->setText(obs_module_text("MinutesCheckboxLabel"));
	ui->minutesCheckBox->setCheckState(Qt::Checked);
	ui->minutesCheckBox->setToolTip(
		obs_module_text("MinutesCheckBoxTip"));
	ui->timerMinutes->setMaxLength(2);
	ui->timerMinutes->setValidator(new QRegularExpressionValidator(
		QRegularExpression("^[1-5]?[0-9]"), this));

	ui->secondsCheckBox->setText(obs_module_text("SecondsCheckboxLabel"));
	ui->secondsCheckBox->setCheckState(Qt::Checked);
	ui->secondsCheckBox->setToolTip(
		obs_module_text("SecondsCheckBoxTip"));
	ui->timerSeconds->setAlignment(Qt::AlignCenter);
	ui->timerSeconds->setMaxLength(2);
	ui->timerSeconds->setValidator(new QRegularExpressionValidator(
		QRegularExpression("^[1-5]?[0-9]"), this));

	ui->countdownTypeTabWidget->setTabText(0,  obs_module_text("SetPeriodTabLabel"));
	ui->countdownTypeTabWidget->setTabText(1,  obs_module_text("SetTimeTabLabel"));
	ui->countdownTypeTabWidget->setToolTip(obs_module_text("SetCountdownTypeTip"));

	ui->textSourceDropdownList->setToolTip(
		obs_module_text("TextSourceDropdownTip"));
	ui->textSourceDropdownLabel->setText(obs_module_text("TextSourceLabel"));

	ui->endMessageCheckBox->setCheckState(Qt::Unchecked);
	ui->endMessageCheckBox->setToolTip(
		obs_module_text("EndMessageCheckBoxTip"));
	ui->endMessageCheckBox->setText(obs_module_text("EndMessageLabel"));
	// ui->timerEndLabel->setEnabled(false);
	ui->endMessageLineEdit->setEnabled(false);
	ui->endMessageLineEdit->setToolTip(
		obs_module_text("EndMessageLineEditTip"));

	ui->switchSceneCheckBox->setCheckState(Qt::Unchecked);
	ui->switchSceneCheckBox->setToolTip(
		obs_module_text("SwitchSceneCheckBoxTip"));
	ui->switchSceneCheckBox->setText(obs_module_text("SwitchScene"));
	// ui->sceneSwitchLabel->setEnabled(false);
	ui->sceneSourceDropdownList->setEnabled(false);
	ui->sceneSourceDropdownList->setToolTip(
		obs_module_text("SceneSourceDropdownTip"));

	ui->playButton->setProperty("themeID", "playIcon");
	ui->playButton->setEnabled(true);
	ui->playButton->setToolTip(obs_module_text("PlayButtonTip"));
	ui->pauseButton->setProperty("themeID", "pauseIcon");
	ui->pauseButton->setEnabled(false);
	ui->pauseButton->setToolTip(obs_module_text("PauseButtonTip"));
	ui->resetButton->setProperty("themeID", "restartIcon");
	ui->resetButton->setToolTip(obs_module_text("ResetButtonTip"));

	context->isPlaying = false;
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

	QObject::connect(ui->hoursCheckBox, SIGNAL(stateChanged(int)),
			 SLOT(ToggleHoursCheckBoxSelected(int)));

	QObject::connect(ui->minutesCheckBox, SIGNAL(stateChanged(int)),
			 SLOT(ToggleMinutesCheckBoxSelected(int)));

	QObject::connect(ui->secondsCheckBox, SIGNAL(stateChanged(int)),
			 SLOT(ToggleSecondsCheckBoxSelected(int)));
}

void CountdownDockWidget::RegisterHotkeys(CountdownWidgetStruct *context)
{
	auto LoadHotkey = [](obs_data_t *s_data, obs_hotkey_id id,
			     const char *name) {
		OBSDataArrayAutoRelease array =
			obs_data_get_array(s_data, name);

		obs_hotkey_load(id, array);
		obs_data_array_release(array);
	};

	char *file = obs_module_config_path(CONFIG);
	obs_data_t *saved_data = nullptr;
	if (file) {
		saved_data = obs_data_create_from_json_file(file);
		bfree(file);
	}

#define HOTKEY_CALLBACK(pred, method, log_action)                              \
	[](void *incoming_data, obs_hotkey_id, obs_hotkey_t *, bool pressed) { \
			Ui::CountdownTimer &countdownUi =                         \
			*static_cast<Ui::CountdownTimer *>(incoming_data);  \
		if ((pred) && pressed) {                                       \
			blog(LOG_INFO, log_action " due to hotkey");           \
			method();                                              \
		}                                                              \
	}
	// Register Play Hotkey
	context->startCountdownHotkeyId = (int)obs_hotkey_register_frontend(
		"Ashmanix_Countdown_Timer_Start",
		obs_module_text("StartCountdownHotkeyDecription"),
		HOTKEY_CALLBACK(true, countdownUi.playButton->animateClick,
				"Play Button Pressed"),
		ui);
	if (saved_data)
		LoadHotkey(saved_data, context->startCountdownHotkeyId,
			   "Ashmanix_Countdown_Timer_Start");

	// Register Pause Hotkey
	context->pauseCountdownHotkeyId = (int)obs_hotkey_register_frontend(
		"Ashmanix_Countdown_Timer_Pause",
		obs_module_text("PauseCountdownHotkeyDecription"),
		HOTKEY_CALLBACK(true, countdownUi.pauseButton->animateClick,
				"Pause Button Pressed"),
		ui);
	if (saved_data)
		LoadHotkey(saved_data, context->pauseCountdownHotkeyId,
			   "Ashmanix_Countdown_Timer_Pause");

	// Register Reset Hotkey
	context->setCountdownHotkeyId = (int)obs_hotkey_register_frontend(
		"Ashmanix_Countdown_Timer_Set",
		obs_module_text("SetCountdownHotkeyDecription"),
		HOTKEY_CALLBACK(true, countdownUi.resetButton->animateClick,
				"Set Button Pressed"),
		ui);
	if (saved_data)
		LoadHotkey(saved_data, context->setCountdownHotkeyId,
			   "Ashmanix_Countdown_Timer_Set");

	obs_data_release(saved_data);
#undef HOTKEY_CALLBACK
}

void CountdownDockWidget::UnregisterHotkeys()
{
	if (countdownTimerData->startCountdownHotkeyId)
		obs_hotkey_unregister(
			countdownTimerData->startCountdownHotkeyId);
	if (countdownTimerData->pauseCountdownHotkeyId)
		obs_hotkey_unregister(
			countdownTimerData->pauseCountdownHotkeyId);
	if (countdownTimerData->setCountdownHotkeyId)
		obs_hotkey_unregister(countdownTimerData->setCountdownHotkeyId);
}

void CountdownDockWidget::PlayButtonClicked()
{
	CountdownWidgetStruct *context = countdownTimerData;
	if (IsSetTimeZero(context))
		return;

	ui->timeDisplay->display(context->time->toString("hh:mm:ss"));
	StartTimerCounting(context);
}

void CountdownDockWidget::PauseButtonClicked()
{
	CountdownWidgetStruct *context = countdownTimerData;
	StopTimerCounting(context);
}

void CountdownDockWidget::ResetButtonClicked()
{
	CountdownWidgetStruct *context = countdownTimerData;
	int hours = ui->timerHours->text().toInt();
	int minutes = ui->timerMinutes->text().toInt();
	int seconds = ui->timerSeconds->text().toInt();

	StopTimerCounting(context);

	context->time->setHMS(hours, minutes, seconds, 0);

	UpdateTimeDisplay(context->time);
}

void CountdownDockWidget::StartTimerCounting(CountdownWidgetStruct *context)
{
	context->isPlaying = true;
	context->timer->start(COUNTDOWNPERIOD);
	ui->playButton->setEnabled(false);
	ui->pauseButton->setEnabled(true);
	ui->resetButton->setEnabled(false);

	ui->timerHours->setEnabled(false);
	ui->hoursCheckBox->setEnabled(false);
	ui->timerMinutes->setEnabled(false);
	ui->minutesCheckBox->setEnabled(false);
	ui->timerSeconds->setEnabled(false);
	ui->secondsCheckBox->setEnabled(false);

	ui->textSourceDropdownList->setEnabled(false);
	ui->textSourceDropdownLabel->setEnabled(false);
	ui->endMessageLineEdit->setEnabled(false);
	ui->sceneSourceDropdownList->setEnabled(false);
	ui->endMessageCheckBox->setEnabled(false);
	ui->switchSceneCheckBox->setEnabled(false);

	ui->countdownTypeTabWidget->setEnabled(false);
}

void CountdownDockWidget::StopTimerCounting(CountdownWidgetStruct *context)
{
	context->isPlaying = false;
	context->timer->stop();
	ui->playButton->setEnabled(true);
	ui->pauseButton->setEnabled(false);
	ui->resetButton->setEnabled(true);

	ui->timerHours->setEnabled(true);
	ui->hoursCheckBox->setEnabled(true);
	ui->timerMinutes->setEnabled(true);
	ui->minutesCheckBox->setEnabled(true);
	ui->timerSeconds->setEnabled(true);
	ui->secondsCheckBox->setEnabled(true);

	ui->textSourceDropdownList->setEnabled(true);
	ui->textSourceDropdownLabel->setEnabled(true);

	ui->endMessageCheckBox->setEnabled(true);
	if (ui->endMessageCheckBox->isChecked()) {
		ui->endMessageLineEdit->setEnabled(true);
	}
	ui->switchSceneCheckBox->setEnabled(true);
	if (ui->switchSceneCheckBox->isChecked()) {
		ui->sceneSourceDropdownList->setEnabled(true);
	}

	ui->countdownTypeTabWidget->setEnabled(true);
}

void CountdownDockWidget::InitialiseTimerTime(CountdownWidgetStruct *context)
{
	context->timer = new QTimer();
	QObject::connect(context->timer, SIGNAL(timeout()),
			 SLOT(TimerDecrement()));
	context->time = new QTime(ui->timerHours->text().toInt(),
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

	UpdateTimeDisplay(currentTime);

	if (currentTime->hour() == 0 && currentTime->minute() == 0 &&
	    currentTime->second() == 0) {
		QString endMessageText = ui->endMessageLineEdit->text();
		if (ui->endMessageCheckBox->isChecked()) {
			SetSourceText(endMessageText.toStdString().c_str());
		}
		if (ui->switchSceneCheckBox->isChecked()) {
			SetCurrentScene();
		}
		ui->timeDisplay->display("00:00:00");
		currentTime->setHMS(0, 0, 0, 0);
		StopTimerCounting(context);
		return;
	}
}

QString CountdownDockWidget::ConvertTimeToDisplayString(QTime *timeToConvert)
{
	int hoursState = ui->hoursCheckBox->checkState();
	int minutesState = ui->minutesCheckBox->checkState();
	int secondsState = ui->secondsCheckBox->checkState();

	QString stringTime = "";

	if (hoursState && minutesState & secondsState) {
		stringTime = timeToConvert->toString("hh:mm:ss");
	} else if (!hoursState && minutesState && secondsState) {
		stringTime = timeToConvert->toString("mm:ss");
	} else if (!hoursState && !minutesState && secondsState) {
		stringTime = timeToConvert->toString("ss");
	} else if (!hoursState && minutesState && !secondsState) {
		stringTime = timeToConvert->toString("mm");
	} else if (hoursState && !minutesState && !secondsState) {
		stringTime = timeToConvert->toString("hh");
	} else if (hoursState && !minutesState && secondsState) {
		stringTime = timeToConvert->toString("hh:ss");
	} else if (hoursState && minutesState && !secondsState) {
		stringTime = timeToConvert->toString("hh:mm");
	} else if (!hoursState && !minutesState && !secondsState) {
		stringTime = "Nothing selected!";
	}

	return stringTime;
}

void CountdownDockWidget::UpdateTimeDisplay(QTime *time)
{
	ui->timeDisplay->display(time->toString("hh:mm:ss"));
	QString formattedDisplayTime = ConvertTimeToDisplayString(time);
	// const char *timeToShow = ConvertToConstChar(formattedDisplayTime);
	// blog(LOG_INFO, "Formatted time is: %s", timeToShow);
	SetSourceText(formattedDisplayTime);
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

bool CountdownDockWidget::IsSetTimeZero(CountdownWidgetStruct *context)
{
	bool isZero = false;

	if (context->time->hour() == 0 && context->time->minute() == 0 &&
	    context->time->second() == 0) {
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

void CountdownDockWidget::EndMessageCheckBoxSelected(int state)
{
	if (state) {
		ui->endMessageLineEdit->setEnabled(true);
		// ui->timerEndLabel->setEnabled(true);
	} else {
		ui->endMessageLineEdit->setEnabled(false);
		// ui->timerEndLabel->setEnabled(false);
	}
}

void CountdownDockWidget::SceneSwitchCheckBoxSelected(int state)
{
	if (state) {
		ui->sceneSourceDropdownList->setEnabled(true);
		// ui->sceneSwitchLabel->setEnabled(true);
	} else {
		ui->sceneSourceDropdownList->setEnabled(false);
		// ui->sceneSwitchLabel->setEnabled(false);
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
	char *file = obs_module_config_path(CONFIG);
	obs_data_t *data = nullptr;
	if (file) {
		data = obs_data_create_from_json_file(file);
		bfree(file);
	}
	if (data) {
		// Get Save Data

		// Time
		int hours = (int)obs_data_get_int(data, "hours");
		int hoursCheckBoxStatus =
			(int)obs_data_get_int(data, "hoursCheckBoxStatus");

		int minutes = (int)obs_data_get_int(data, "minutes");
		int minutesCheckBoxStatus =
			(int)obs_data_get_int(data, "minutesCheckBoxStatus");

		int seconds = (int)obs_data_get_int(data, "seconds");

		// Selections
		int secondsCheckBoxStatus =
			(int)obs_data_get_int(data, "secondsCheckBoxStatus");

		const char *selectedTextSource =
			obs_data_get_string(data, "selectedTextSource");

		int endMessageCheckBoxStatus =
			(int)obs_data_get_int(data, "endMessageCheckBoxStatus");

		const char *endMessageText =
			obs_data_get_string(data, "endMessageText");

		int switchSceneCheckBoxStatus = (int)obs_data_get_int(
			data, "switchSceneCheckBoxStatus");

		const char *selectedSceneSource =
			obs_data_get_string(data, "selectedSceneSource");

		UNUSED_PARAMETER(selectedTextSource);
		UNUSED_PARAMETER(selectedSceneSource);

		// Apply saved data to plugin
		ui->timerHours->setText(QString::number(hours));
		ui->hoursCheckBox->setCheckState(
			(Qt::CheckState)hoursCheckBoxStatus);

		ui->timerMinutes->setText(QString::number(minutes));
		ui->minutesCheckBox->setCheckState(
			(Qt::CheckState)minutesCheckBoxStatus);

		ui->timerSeconds->setText(QString::number(seconds));
		ui->secondsCheckBox->setCheckState(
			(Qt::CheckState)secondsCheckBoxStatus);

		ui->endMessageLineEdit->setText(endMessageText);

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
	CountdownWidgetStruct *context = countdownTimerData;

	obs_data_t *obsData = obs_data_create();

	int hours = ui->timerHours->text().toInt();
	obs_data_set_int(obsData, "hours", hours);
	int hoursCheckBoxStatus = ui->hoursCheckBox->checkState();
	obs_data_set_int(obsData, "hoursCheckBoxStatus", hoursCheckBoxStatus);

	int minutes = ui->timerMinutes->text().toInt();
	obs_data_set_int(obsData, "minutes", minutes);
	int minutesCheckBoxStatus = ui->minutesCheckBox->checkState();
	obs_data_set_int(obsData, "minutesCheckBoxStatus",
			 minutesCheckBoxStatus);

	int seconds = ui->timerSeconds->text().toInt();
	obs_data_set_int(obsData, "seconds", seconds);
	int secondsCheckBoxStatus = ui->secondsCheckBox->checkState();
	obs_data_set_int(obsData, "secondsCheckBoxStatus",
			 secondsCheckBoxStatus);

	obs_data_set_string(obsData, "selectedTextSource",
			    context->textSourceNameText.c_str());

	int endMessageCheckBoxStatus =
		ui->endMessageCheckBox->checkState();
	obs_data_set_int(obsData, "endMessageCheckBoxStatus",
			 endMessageCheckBoxStatus);

	QString endMessageLineEdit = ui->endMessageLineEdit->text();
	obs_data_set_string(obsData, "endMessageText",
			    endMessageLineEdit.toStdString().c_str());

	int switchSceneCheckBoxStatus =
		ui->switchSceneCheckBox->checkState();
	obs_data_set_int(obsData, "switchSceneCheckBoxStatus",
			 switchSceneCheckBoxStatus);

	obs_data_set_string(obsData, "selectedSceneSource",
			    context->sceneSourceNameText.c_str());

	// Hotkeys
	obs_data_array_t *start_countdown_hotkey_save_array =
		obs_hotkey_save(context->startCountdownHotkeyId);
	obs_data_set_array(obsData, "Ashmanix_Countdown_Timer_Start",
			   start_countdown_hotkey_save_array);
	obs_data_array_release(start_countdown_hotkey_save_array);

	obs_data_array_t *pause_countdown_hotkey_save_array =
		obs_hotkey_save(context->pauseCountdownHotkeyId);
	obs_data_set_array(obsData, "Ashmanix_Countdown_Timer_Pause",
			   pause_countdown_hotkey_save_array);
	obs_data_array_release(pause_countdown_hotkey_save_array);

	obs_data_array_t *set_countdown_hotkey_save_array =
		obs_hotkey_save(context->setCountdownHotkeyId);
	obs_data_set_array(obsData, "Ashmanix_Countdown_Timer_Set",
			   set_countdown_hotkey_save_array);
	obs_data_array_release(set_countdown_hotkey_save_array);

	char *file = obs_module_config_path(CONFIG);
	obs_data_save_json(obsData, file);
	obs_data_release(obsData);
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
