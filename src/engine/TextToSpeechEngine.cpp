#include "TextToSpeechEngine.h"
#include "helpers.h"
#include <QDebug>
#include <QDirListing>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <iostream>

namespace sf
{

using namespace Qt::StringLiterals;

TextToSpeechEngine::TextToSpeechEngine(const QVariantMap& parameters, QObject* parent)
	: QTextToSpeechEngine(parent)
	, _settings(QString::fromStdString(getLibraryPath().parent_path().append("models").append("models.ini").string()), QSettings::IniFormat)
	, _state(QTextToSpeech::Ready)
	, _errorReason(QTextToSpeech::ErrorReason::NoError)
	, _dirPlugin(getLibraryPath().parent_path())
	, _rate(0.0)
	, _dirModels(getLibraryPath().parent_path().append("models"))
	, _dirEspeakNgData(getLibraryPath().parent_path().append("espeak-ng-data"))
{
	// Name the worker thread for debugging.
	_worker.setObjectName(staticMetaObject.className());
	// Set the default locale to the system one not using the QLocale::system() directly since it does not compare well.
	_selected.locale = QLocale(QLocale::system().language(), QLocale::system().territory());
	//
	connect(&_worker, &TextToSpeechSynthesizer::synthesisQueued, this, &TextToSpeechEngine::playQueued);
	// Log some information.
	qInfo() << "Default locale:" << _selected.locale;
	qInfo() << "Plugin directory:" << _dirPlugin.path();
	qInfo() << "Models directory:" << _dirModels.path();
	qInfo() << "Espeak directory:" << _dirEspeakNgData.path();
	// Initialize available locales and voices.
	for (const auto& entry: QDirListing(_dirModels.path(), {"*.json"}, QDirListing::IteratorFlag::FilesOnly))
	{
		QFileInfo fi(_dirModels, entry.fileName());
		qDebug() << "Processing model:" << fi.baseName();
		auto json = readJsonFile(fi.filePath()).object();
		auto language = json["language"].toObject();
		auto family = language["family"].toString();
		auto region = language["region"].toString();
		auto speakers = json["num_speakers"].toInt();
		// Create a locale from the model json-file values.
		QLocale loc(QLocale::codeToLanguage(family), QLocale::codeToTerritory(region));
		// Add the local to the list of locales available.
		if (!_locales.contains(loc))
			_locales.append(loc);
		// Initialize available voices using createVoice().
		for (int i = 0; i < speakers; i++)
		{
			auto [model_name, gender] = getModelGenderFromFilename(fi.completeBaseName(), i);
			if (speakers > 1)
				model_name.append(u" [%1]"_s.arg(i));
			// Add a voice by speaker number.
			_voices.append(createVoice(
				model_name, loc, gender, QVoice::Age::Adult,//
				QVariant::fromValue(FileSpeaker(fi.completeBaseName(), i))
			));
		}
	}
	// Assign the 'espeak_ng_data' directory location.
	_worker.setEspeakNgDataDirectory(_dirEspeakNgData.path());
	// Assign the worker class instance to the new thread.
	_worker.moveToThread(&_worker);
	//
	_worker.start();
}

TextToSpeechEngine::~TextToSpeechEngine()
{
	if (_audioSink)
		_audioSink->stop();
}

QList<QLocale> TextToSpeechEngine::availableLocales() const
{
	return _locales;
}

QList<QVoice> TextToSpeechEngine::availableVoices() const
{
	QList<QVoice> rv;
	rv.append(QVoice{});
	for (const auto& v: _voices)
		if (v.locale() == _selected.locale)
			rv.append(v);
	return rv;
}

void TextToSpeechEngine::say(const QString& text)
{
	// Regex matches '.' '!' or '?' followed by a space or end of string
	static QRegularExpression re("(?<=[.!?])\\s+");
	// When an empty text is given or no voice is selected, bailout.
	if (text.isEmpty())
		return;
	// Split the text into sentences.
	_worker.add(text.split(re, Qt::SkipEmptyParts));
}

void TextToSpeechEngine::synthesize(const QString& text)
{
	// Synthesize to audio data without playing.
	/*
	updateState(QTextToSpeech::Synthesizing);
	updateState(QTextToSpeech::Ready);
	*/
}

void TextToSpeechEngine::stop(QTextToSpeech::BoundaryHint boundaryHint)
{
	getAudioSink()->stop();
	// Remove all entries from the queue.
	_worker.clear();
	// Allow played again when the sentry was set.
	_playSentry = false;
}

void TextToSpeechEngine::pause(QTextToSpeech::BoundaryHint boundaryHint)
{
	if (_state == QTextToSpeech::Speaking)
		getAudioSink()->suspend();
}

void TextToSpeechEngine::resume()
{
	if (_state == QTextToSpeech::Paused)
		getAudioSink()->resume();
}

double TextToSpeechEngine::rate() const
{
	return _rate;
}

bool TextToSpeechEngine::setRate(double rate)
{
	if (rate >= -1.0 && rate <= 1.0)
	{
		_worker.setLengthScaleFactor(convertRateToFactor(_rate = rate));
		return true;
	}
	return false;
}

double TextToSpeechEngine::pitch() const
{
	return 0.0;
}

bool TextToSpeechEngine::setPitch(double pitch)
{
	if (pitch >= -1.0 && pitch <= 1.0)
		return true;
	return false;
}

QLocale TextToSpeechEngine::locale() const
{
	return _selected.locale;
}

bool TextToSpeechEngine::setLocale(const QLocale& locale)
{
	if (_selected.locale != locale)
	{
		// Deselect the voice setting an .
		_selected.voice = -1;
		// If the locale exists selected it.
		if (_locales.contains(locale))
		{
			_selected.locale = locale;
			return true;
		}
		return false;
	}
	return true;
}

double TextToSpeechEngine::volume() const
{
	return _selected.volume;
}

bool TextToSpeechEngine::setVolume(double volume)
{
	if (volume >= 0.0 && volume <= 1.0)
	{
		_selected.volume = volume;
		if (_audioSink)
			_audioSink->setVolume(_selected.volume);
		return true;
	}
	return false;
}

QVoice TextToSpeechEngine::voice() const
{
	// Check if the selected voice index is out of bounds.
	if (_selected.voice < 0 || _selected.voice >= _voices.size())
		return QVoice{};
	return _voices[_selected.voice];
}

bool TextToSpeechEngine::setVoice(const QVoice& voice)
{
	if (!_voices.contains(voice))
		return false;
	_selected.voice = _voices.indexOf(voice);
	// When the voice is not found in the list.
	if (_selected.voice < 0)
		// Signal failure to do so.
		return false;
	// Get the model filename and speaker id.
	auto [filename, speaker_id] = voiceData(voice).value<FileSpeaker>();
	// Get the filepath for both required files.
	QFileInfo fi_data(_dirModels, filename);
	QFileInfo fi_json(_dirModels, filename + ".json");
	// Check if both files exist.
	if (!fi_data.exists() || !fi_json.exists())
	{
		qWarning() << "Missing file:" << fi_data;
		return false;
	}
	// Set the sample rate from the json-file.
	_selected.audioFormat.setSampleRate(readJsonFile(fi_json.filePath()).object()["audio"].toObject()["sample_rate"].toInt());
	// 1 for Mono, 2 for Stereo.
	_selected.audioFormat.setChannelCount(1);
	// Specify 16-bit signed integers.
	_selected.audioFormat.setSampleFormat(QAudioFormat::Float);
	// Trigger a voice update/reload.
	_worker.setVoice(fi_data.absoluteFilePath(), fi_json.absoluteFilePath(), speaker_id);
	//
	return true;
}

QTextToSpeech::State TextToSpeechEngine::state() const
{
	return _state;
}

QTextToSpeech::ErrorReason TextToSpeechEngine::errorReason() const
{
	return _errorReason;
}

QString TextToSpeechEngine::errorString() const
{
	return _errorString;
}

QTextToSpeech::Capabilities TextToSpeechEngine::capabilities() const
{
	auto caps = QTextToSpeechEngine::capabilities();
	caps.setFlag(QTextToSpeech::Capability::PauseResume);
	caps.setFlag(QTextToSpeech::Capability::Synthesize);
	return caps;
}

void TextToSpeechEngine::playQueued()
{
	// The QAudio::State is not enough for detecting activity.
	if (_playSentry)
		return;
	_playSentry = true;
	// Get the configured audio sink device.
	if (auto as = getAudioSink())
	{
		// Get the current state.
		auto audio_state = as->state();
		// Only when idle or stopped state the next buffer can be played.
		if (audio_state == QAudio::State::IdleState || audio_state == QAudio::State::StoppedState)
		{
			// Play the next buffer queued.
			_worker.handleQueued([&](const TextToSpeechSynthesizer::buffer_t& buffer) {
				playBuffer(buffer);
			});
		}
	}
}

void TextToSpeechEngine::updateState(QTextToSpeech::State newState)
{
	if (_state != newState)
	{
		_state = newState;
		emit stateChanged(_state);
	}
}

QAudioSink* TextToSpeechEngine::getAudioSink()
{
	// Check if the format is supported.
	auto device = QMediaDevices::defaultAudioOutput();
	if (!device.isFormatSupported(_selected.audioFormat))
	{
		qWarning("Audio format not supported!");
		return nullptr;
	}
	// Create or recreate audio sink when the format changed.
	if (!_audioSink || _audioSink->format() != _selected.audioFormat)
	{
		// Delete the previous instance when it exists.
		delete _audioSink;
		_audioSink = new QAudioSink(device, _selected.audioFormat, this);
		connect(_audioSink, &QAudioSink::stateChanged, this, &TextToSpeechEngine::AudioSinkStateChange);
	}
	return _audioSink;
}

void TextToSpeechEngine::playBuffer(const TextToSpeechSynthesizer::buffer_t& audio_data)
{
	// Convert std::vector to QByteArray
	QByteArray byteArray(reinterpret_cast<const char*>(audio_data.data()), audio_data.size() * sizeof(TextToSpeechSynthesizer::buffer_t::value_type));
	// Create a buffer to hold the audio data when it has not been created.
	if (!_buffer)
		_buffer = new QBuffer(this);
	if (_buffer->isOpen())
		_buffer->close();
	// Add the audio data to the buffer.
	_buffer->setData(byteArray);
	// Open the buffer
	_buffer->open(QIODevice::ReadOnly);
	// Start playback.
	if (auto as = getAudioSink())
		as->start(_buffer);
}

void TextToSpeechEngine::AudioSinkStateChange(QAudio::State state)
{
	switch (state)
	{
		case QAudio::State::ActiveState:
			updateState(QTextToSpeech::Speaking);
			break;

		case QAudio::State::SuspendedState:
			updateState(QTextToSpeech::Paused);
			break;

		case QAudio::State::StoppedState:
			updateState(QTextToSpeech::Ready);
			break;

		case QAudio::State::IdleState:
			_playSentry = false;
			// When the queue is empty, set the ready state otherwise play the next queued buffer.
			if (_worker.isQueueEmpty())
				updateState(QTextToSpeech::Ready);
			else
			{
				// Play the next buffer queued.
				_worker.handleQueued([&](const TextToSpeechSynthesizer::buffer_t& buffer) {
					playBuffer(buffer);
				});
			}
			break;
	}
}

QPair<QString, QVoice::Gender> TextToSpeechEngine::getModelGenderFromFilename(const QString& filename, int speaker_id)
{
	// Split the string like: 'en_US-ljspeech-medium'.
	auto parts = filename.split("-");
	auto model_name = (parts.length() >= 2) ? parts[1] : QString("?");
	_settings.beginGroup(filename);
	auto gender_str = _settings.value(u"gender_%1"_s.arg(speaker_id), enumToKey(QVoice::Gender::Unknown)).toString();
	_settings.endGroup();
	// Find if the enumerated values for the gender in a case-insensitive way.
	for (auto gen: {QVoice::Gender::Male, QVoice::Gender::Female, QVoice::Gender::Unknown})
		if (gender_str.compare(enumToKey(gen), gender_str, Qt::CaseInsensitive) == 0)
			return {model_name, gen};
	// When not found, set gender to 'Unknown'.
	return {model_name, QVoice::Gender::Unknown};
}

float TextToSpeechEngine::convertRateToFactor(double rate, double minFactor, double maxFactor)
{
	// Clamp the input to ensure it stays within the [-1.0, 1.0] range.
	rate = std::max(-1.0, std::min(1.0, rate));
	if (rate < 0.0)
	{
		// Linearly map [-1.0, 0.0] to [minFactor, 1.0].
		return 1.0f / (1.0f + rate * (1.0 - minFactor));
	}
	// Linearly map [0.0, 1.0] to [1.0, maxFactor].
	return 1.0f / (1.0f + rate * (maxFactor - 1.0));
}

}// namespace sf
