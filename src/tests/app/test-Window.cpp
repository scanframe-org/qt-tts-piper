#include "test-Window.h"
#include "ui_test-Window.h"
#include <QDir>

namespace sf
{

Window::Window(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::Window)
	, _tts(nullptr)
#if WIN32
	, _engineProvider("sapi")
#else
	, _engineProvider("sf-piper")
#endif
{
	ui->setupUi(this);
	connect(ui->pbCreate, &QPushButton::clicked, this, &Window::createTextToSpeech);
	connect(ui->pbSpeakText, &QPushButton::clicked, this, &Window::speakText);
	connect(ui->pbListInfo, &QPushButton::clicked, this, &Window::listInfo);
}

Window::~Window()
{
	delete ui;
}

void Window::storeState(QSettings& settings, bool save)
{
	constexpr QStringView keyState(u"State");
	constexpr QStringView keyText(u"Text");
#if WIN32
	constexpr QStringView keyProvider(u"ProviderWindows");
#else
	constexpr QStringView keyProvider(u"ProviderLinux");
#endif
	//
	settings.beginGroup(u"AppWindow");
	if (save)
	{
		settings.setValue(keyState, geometry());
		settings.setValue(keyText, ui->pteText->toPlainText());
		settings.setValue(keyProvider, _engineProvider);
	}
	else
	{
		setGeometry(settings.value(keyState, geometry()).toRect());
		ui->pteText->setPlainText(settings.value(keyText, ui->pteText->toPlainText()).toString());
		_engineProvider = settings.value(keyProvider, _engineProvider).toString();
	}
	settings.endGroup();
}

void Window::createTextToSpeech()
{
	if (!_tts)
	{
		qDebug() << "Library paths:" << QCoreApplication::libraryPaths();
		qDebug() << "Available TTS engines:" << QTextToSpeech::availableEngines();
		// _tts = new QTextToSpeech(this);
		// _tts->setEngine("piper");
		_tts = new QTextToSpeech(_engineProvider, this);
		for (auto& voice: _tts->availableVoices())
		{
			if (voice.gender() == QVoice::Female)
			{
				qDebug() << voice.name() << voice.age() << voice.ageName(voice.age()) << voice.genderName(voice.gender());
				_tts->setVoice(voice);
				_tts->setPitch(0.5);
				break;
			}
		}
	}
}

void Window::listInfo()
{
	if (_tts)
	{
		qDebug() << "Locales:" << _tts->availableLocales();
		qDebug() << "Voices:";
		for (auto& voice: _tts->availableVoices())
		{
			qDebug() << voice.name() << voice.age() << voice.ageName(voice.age()) << voice.genderName(voice.gender());
		}
	}
}

void Window::speakText()
{
	if (_tts)
	{
		qDebug() << ui->pteText->toPlainText();
		_tts->say(ui->pteText->toPlainText());
	}
}

}// namespace sf
