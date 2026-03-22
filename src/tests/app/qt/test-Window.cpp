#include "test-Window.h"
#include "ui_test-Window.h"
#include <QDir>
#include <QLoggingCategory>
#include <algorithm>

namespace sf
{

using namespace Qt::StringLiterals;

int indexFromComboBoxByData(const QComboBox* comboBox, const QVariant& value, int default_index)
{
	auto index = comboBox->findData(value);
	if (index >= 0)
			return index;
	return default_index;
}

int indexFromComboBoxByText(const QComboBox* comboBox, const QString& value, int default_index)
{
	auto index = comboBox->findText(value);
	if (index >= 0)
		return index;
	return default_index;
}

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _tts(nullptr)
#if WIN32
	, _engineProvider("sapi")
#else
	, _engineProvider("sf-piper")
#endif
{
	//QLoggingCategory::setFilterRules(u"qt.speech.tts=true \n qt.speech.tts.*=true"_s);
	ui->setupUi(this);
	// Populate the engine selection list
	ui->cbEngine->addItem(u"Default"_s, u"default"_s);
	// Populate the engine dropdown list.
	for (const QString& engine: QTextToSpeech::availableEngines())
		ui->cbEngine->addItem(engine, engine);
	ui->cbEngine->setCurrentIndex(0);
	// Select the first engine by default.
	selectEngine(0);
	//
	connect(ui->sPitch, &QSlider::valueChanged, this, &MainWindow::setPitch);
	connect(ui->sRate, &QSlider::valueChanged, this, &MainWindow::setRate);
	connect(ui->sVolume, &QSlider::valueChanged, this, &MainWindow::setVolume);
	connect(ui->cbEngine, &QComboBox::currentIndexChanged, this, &MainWindow::selectEngine);
	connect(ui->cbLanguage, &QComboBox::currentIndexChanged, this, &MainWindow::selectLanguage);
	connect(ui->cbVoice, &QComboBox::currentIndexChanged, this, &MainWindow::selectVoice);
}

MainWindow::~MainWindow()
{
	(void)_tts->disconnect(this);
	delete ui;
}

void MainWindow::setSettings(QSettings& settings)
{
	_settings = &settings;
}

void MainWindow::storeState(bool save, bool text_only)
{
	if (!_settings)
	{
		qWarning() << "Settings not set!";
		return;
	}
	auto keyState(u"State"_s);
	auto keyText(u"Text"_s);
	auto keyRate(u"Rate"_s);
	auto keyLanguage(u"Language"_s);
	auto keyVoice(u"Voice"_s);
#if WIN32
	constexpr QStringView keyProvider(u"ProviderWindows");
#else
	auto keyProvider(u"ProviderLinux"_s);
#endif
	//
	_settings->beginGroup(u"AppWindow"_s);
	if (save)
	{
		if (!text_only)
		{
			_settings->setValue(keyState, geometry());
			_settings->setValue(keyProvider, ui->cbEngine->currentData().toString());
			_settings->setValue(keyLanguage, ui->cbLanguage->currentData().value<QLocale>().name());
			_settings->setValue(keyVoice, ui->cbVoice->currentText());
			_settings->setValue(keyRate, ui->sRate->value());
		}
		_settings->beginGroup(_tts->locale().name());
		_settings->setValue(keyText, ui->plainTextEdit->toPlainText());
		_settings->endGroup();
	}
	else
	{
		if (!text_only)
		{
			setGeometry(_settings->value(keyState, geometry()).toRect());
			ui->cbLanguage->setCurrentIndex(indexFromComboBoxByData(ui->cbLanguage, QVariant(QLocale(_settings->value(keyLanguage, u"C"_s).toString())), 0));
			ui->cbEngine->setCurrentIndex(indexFromComboBoxByData(ui->cbEngine, _settings->value(keyProvider, u"default"_s), 0));
			ui->cbVoice->setCurrentIndex(indexFromComboBoxByText(ui->cbVoice, _settings->value(keyVoice, u"default"_s).toString(), 0));
			ui->sRate->setValue(_settings->value(keyRate, ui->sRate->value()).toInt());
		}
		_settings->beginGroup(_tts->locale().name());
		ui->plainTextEdit->setPlainText(_settings->value(keyText, ui->plainTextEdit->toPlainText()).toString());
		_settings->endGroup();
	}
	_settings->endGroup();
}

void MainWindow::setRate(int rate)
{
	_tts->setRate(rate / static_cast<double>(ui->sRate->maximum()));
	ui->sRate->setValue(_tts->rate() * ui->sRate->maximum());
}

void MainWindow::setPitch(int pitch)
{
	_tts->setPitch(pitch / 10.0);
	ui->sPitch->setValue(_tts->pitch());
}

void MainWindow::setVolume(int volume)
{
	_tts->setVolume(volume / 100.0);
}

void MainWindow::stateChanged(QTextToSpeech::State state)
{
	switch (state)
	{
		case QTextToSpeech::Speaking:
			ui->statusbar->showMessage(tr("Speech started..."));
			break;
		case QTextToSpeech::Ready:
			ui->statusbar->showMessage(tr("Speech stopped..."), 2000);
			break;
		case QTextToSpeech::Paused:
			ui->statusbar->showMessage(tr("Speech paused..."));
			break;
		default:
			ui->statusbar->showMessage(tr("Speech error!"));
			break;
	}
	ui->pbPause->setEnabled(state == QTextToSpeech::Speaking);
	ui->pbResume->setEnabled(state == QTextToSpeech::Paused);
	ui->pbStop->setEnabled(state == QTextToSpeech::Speaking || state == QTextToSpeech::Paused);
}

void MainWindow::selectEngine(int index)
{
	// Get the engine name from the index.
	const QString engine = ui->cbEngine->itemData(index).toString();
	// Check recreation of the engine is needed.
	if (_tts && _tts->engine() == engine)
		return;
	// Delete the previous TTS engine when it is present at all.
	delete _tts;
	// Create the selected engine.
	_tts = engine == u"default" ? new QTextToSpeech(this) : new QTextToSpeech(engine, this);
	// Some engines initialize asynchronously.
	if (_tts->state() == QTextToSpeech::Ready)
		onEngineReady();
	else
		connect(_tts, &QTextToSpeech::stateChanged, this, &MainWindow::onEngineReady, Qt::SingleShotConnection);
}

static bool localeLessThan(const QLocale& l1, const QLocale& l2)
{
	return l1.name().compare(l2.name()) < 0;
}

void MainWindow::onEngineReady()
{
	if (_tts->state() != QTextToSpeech::Ready)
	{
		stateChanged(_tts->state());
		return;
	}
	// Get the capabilities from the engine.
	const bool hasPauseResume = _tts->engineCapabilities() & QTextToSpeech::Capability::PauseResume;
	// Make the button visible when it is possible to pause and resume.
	ui->pbPause->setVisible(hasPauseResume);
	ui->pbResume->setVisible(hasPauseResume);
	// Block signals of the languages combobox while populating.
	QSignalBlocker blocker(ui->cbLanguage);
	// Clear the list of languages.
	ui->cbLanguage->clear();
	QList<QLocale> locales = _tts->availableLocales();
	locales.append({QLocale::Language::C, QLocale::Territory::AnyTerritory});
	// Sort the locale list.
	std::stable_sort(locales.begin(), locales.end(), localeLessThan);
	//
	QLocale current = _tts->locale();
	for (const QLocale& locale: std::as_const(locales))
	{
		ui->cbLanguage->addItem(u"%1 - %2"_s.arg(QLocale::languageToString(locale.language()), QLocale::territoryToString(locale.territory())), locale);
		if (locale.name() == current.name())
			current = locale;
	}
	setRate(ui->sRate->value());
	setPitch(ui->sPitch->value());
	setVolume(ui->sVolume->value());
	connect(ui->pbSpeak, &QPushButton::clicked, _tts, [this] {
		_tts->say(ui->plainTextEdit->toPlainText());
	});
	connect(ui->pbStop, &QPushButton::clicked, _tts, [this] {
		_tts->stop();
	});
	connect(ui->pbPause, &QPushButton::clicked, _tts, [this] {
		_tts->pause();
	});
	connect(ui->pbResume, &QPushButton::clicked, _tts, &QTextToSpeech::resume);
	connect(_tts, &QTextToSpeech::stateChanged, this, &MainWindow::stateChanged);
	connect(_tts, &QTextToSpeech::localeChanged, this, &MainWindow::localeChanged);
	//
	blocker.unblock();
	//
	localeChanged(current);
}

void MainWindow::selectLanguage(int language)
{
	storeState(true, true);
	_tts->setLocale(ui->cbLanguage->itemData(language).toLocale());
	storeState(false, true);
}

void MainWindow::selectVoice(int index)
{
	auto voice = _voices.at(index);
	_tts->setVoice(voice);
}

void MainWindow::localeChanged(const QLocale& locale)
{
	qDebug() << "Locale changed:" << locale;
	QVariant localeVariant(locale);
	ui->cbLanguage->setCurrentIndex(ui->cbLanguage->findData(localeVariant));
	QSignalBlocker blocker(ui->cbVoice);
	ui->cbVoice->clear();
	_voices = _tts->availableVoices();
	QVoice currentVoice = _tts->voice();
	for (auto& voice: std::as_const(_voices))
	{
		ui->cbVoice->addItem(u"%1 - %2 - %3"_s.arg(voice.name(), QVoice::genderName(voice.gender()), QVoice::ageName(voice.age())));
		if (voice.name() == currentVoice.name())
			ui->cbVoice->setCurrentIndex(ui->cbVoice->count() - 1);
	}
}

}// namespace sf
