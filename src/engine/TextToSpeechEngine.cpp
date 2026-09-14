#include "TextToSpeechEngine.h"
#include "helpers.h"
#include "logging.h"
#include <QDebug>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>
#include <QString>
#include <QStringList>
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
	, _dirModels(getLibraryPath().parent_path().append("models"))
	, _dirEspeakNgData(getLibraryPath().parent_path().append("espeak-ng-data"))
	, _mediaDevices()
	, _audioDevice(_mediaDevices.defaultAudioOutput())
	, _audioSink(nullptr)
	, _stream(new AudioStream(this))
	, _playSentry(false)
{
	// Name the worker thread for debugging.
	_worker.setObjectName(staticMetaObject.className());
	// Check if a general parameter provides a different audio device by id.
	if (auto param_name = u"audioDeviceId"_s; parameters.contains(param_name))
	{
		auto ad_id = parameters.value(param_name).toString();
		for (auto& aod: _mediaDevices.audioOutputs())
			if (aod.id() == ad_id)
			{
				_audioDevice = aod;
				break;
			}
	}
	// Set the default locale to the system one not using the QLocale::system() directly since it does not compare well.
	QLocale sys_locale(QLocale::system().language(), QLocale::system().territory());
	// Log some information.
	qCInfo(logCategoryEngine) << "System locale:" << sys_locale;
	qCInfo(logCategoryEngine) << "Plugin directory:" << _dirPlugin.path();
	qCInfo(logCategoryEngine) << "Models directory:" << _dirModels.path();
	qCInfo(logCategoryEngine) << "Espeak directory:" << _dirEspeakNgData.path();
	qCInfo(logCategoryEngine) << "Audio Device:" << _audioDevice.description();
	// Initialize available locales and voices by sorting the json-files by name.
	for (auto& filename: QDir(_dirModels.path()).entryList({"*.json"}, QDir::Files, QDir::Name | QDir::IgnoreCase))
	{
		QFileInfo fi(_dirModels, filename);
		qCInfo(logCategoryEngine) << "Processing model:" << fi.baseName();
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
				model_name.append(u"_%1"_s.arg(i));
			auto voice = createVoice(
				model_name, loc, gender, QVoice::Age::Adult,//
				QVariant::fromValue(FileSpeaker(fi.completeBaseName(), i))
			);
			// Add a voice by speaker number.
			// Add the voice to the available list.
			_voices.append(voice);
		}
	}
	// Set the default locale to the system one and report when not available.
	if (!TextToSpeechEngine::setLocale(sys_locale))
		qCWarning(logCategoryEngine) << "Failed to set default locale to system locale!";
	// Assign the 'espeak_ng_data' directory location.
	_worker.setEspeakNgDataDirectory(_dirEspeakNgData.path());
	// Assign the worker class instance to the new thread.
	_worker.moveToThread(&_worker);
	// Connect the signal to play queued buffers.
	connect(&_worker, &TextToSpeechSynthesizer::synthesisQueued, this, &TextToSpeechEngine::handleQueued);
	// Start the worker thread.
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
	for (const auto& v: _voices)
		if (v.locale() == _selected.locale)
			rv.append(v);
	return rv;
}

void TextToSpeechEngine::say(const QString& text)
{
	// When an empty text is given or no voice is selected, bailout.
	if (text.isEmpty())
		return;
	// When The state is 'Ready' set it 'Speaking' instantly to reflect the actual state.
	if (_state == QTextToSpeech::Ready)
		updateState(QTextToSpeech::Speaking);
	// Split the spoken text into sentences.
	_worker.add(TextToSpeechSynthesizer::qfPlay, text);
}

void TextToSpeechEngine::synthesize(const QString& text)
{
	// Synthesize to audio data without playing.
	// When The state is 'Ready' set it 'Synthesizing' instantly to reflect the actual state.
	if (_state == QTextToSpeech::Ready)
		updateState(QTextToSpeech::Synthesizing);
	// Split the to be synthesized text into sentences.
	_worker.add(TextToSpeechSynthesizer::qfSend, text);
}

void TextToSpeechEngine::stop(QTextToSpeech::BoundaryHint boundaryHint)
{
	// Remove all entries from the queue.
	_worker.clear();
	// Clear the stream buffer.
	_stream->clear();
	// Allow played again when the sentry was set.
	_playSentry = false;
}

void TextToSpeechEngine::pause(QTextToSpeech::BoundaryHint boundaryHint)
{
	if (_state == QTextToSpeech::Speaking)
		if (auto as = getAudioSink())
			as->suspend();
}

void TextToSpeechEngine::resume()
{
	if (_state == QTextToSpeech::Paused)
		if (auto as = getAudioSink())
			as->resume();
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
	qCDebug(logCategoryEngine) << "setLocale:" << locale;
	// Only perform this if the locale is different from the current one.
	if (_selected.locale != locale)
	{
		// If the locale exists selected it.
		if (_locales.contains(locale))
		{
			// Iterate through the voices to find the first matching locale.
			for (auto& voice: _voices)
			{
				if (voice.locale() == locale)
				{
					auto idx = _voices.indexOf(voice);
					// When the voice is successfully updated.
					if (updateVoice(_voices.indexOf(voice)))
					{
						// Set the selected voice index and locale.
						_selected.voice = idx;
						_selected.locale = locale;
					}
					break;
				}
			}
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
	// Update the voice settings.
	return updateVoice(_selected.voice);
}

bool TextToSpeechEngine::updateVoice(qsizetype idx)
{
	// Get the model filename and speaker id.
	auto [filename, speaker_id] = voiceData(_voices[idx]).value<FileSpeaker>();
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
	// Trigger a voice update/reload when something has changed.
	_worker.setVoice(fi_data.absoluteFilePath(), fi_json.absoluteFilePath(), speaker_id);
	// Signal success.
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
	return QTextToSpeech::Capability::PauseResume | QTextToSpeech::Capability::Speak | QTextToSpeech::Capability::Synthesize;
}

void TextToSpeechEngine::handleQueued()
{
	// The QAudio::State is not enough for detecting activity.
	if (_playSentry)
		return;
	_playSentry = true;
	// Play the next buffer queued.
	_worker.handleQueued([&](TextToSpeechSynthesizer::QueueFlag flag, const QByteArray& buffer) {
		qCDebug(logCategoryEngine) << "Process queued: " << flag << ", Size:" << buffer.size();
		if (flag == TextToSpeechSynthesizer::qfPlay)
			playBuffer(buffer);
		else
		{
			sendBuffer(buffer);
			if (flag == TextToSpeechSynthesizer::qfSendEnd)
				updateState(QTextToSpeech::Ready);
		}
	});
}

void TextToSpeechEngine::updateState(QTextToSpeech::State new_state)
{
	if (_state != new_state)
	{
		_state = new_state;
		emit stateChanged(_state);
	}
}

QAudioSink* TextToSpeechEngine::getAudioSink()
{
	// Check if the format is supported.
	if (!_audioDevice.isFormatSupported(_selected.audioFormat))
	{
		_audioSink = nullptr;
		qCWarning(logCategoryEngine, "Audio format not supported!");
	}
	else
	{
		// Create or recreate audio sink when the format changed.
		if (!_audioSink || _audioSink->format() != _selected.audioFormat)
		{
			qCInfo(logCategoryEngine) << "Audio Device id:" << _audioDevice.id();
			// Delete the previous instance when it exists.
			delete _audioSink;
			qCInfo(logCategoryEngine) << "Creating audio sink:" << _selected.audioFormat << "for device:" << _audioDevice.description();
			_audioSink = new QAudioSink(_audioDevice, _selected.audioFormat, this);
			_audioSink->start(_stream);
			connect(_audioSink, &QAudioSink::stateChanged, this, &TextToSpeechEngine::AudioSinkStateChange);
		}
	}
	return _audioSink;
}

void TextToSpeechEngine::sendBuffer(const QByteArray& audio_data)
{
	emit synthesized(_selected.audioFormat, audio_data);
}

void TextToSpeechEngine::playBuffer(const QByteArray& audio_data)
{
	if (auto as = getAudioSink())
	{
		// Start playback by add the buffer data to the audio stream and only when not stopped.
		if (auto state = as->state(); state != QAudio::State::StoppedState)
		{
			qCInfo(logCategoryEngine) << "Appending to audio stream, size:" << audio_data.size();
			_stream->appendData(audio_data);
		}
		else
		{
			qCDebug(logCategoryEngine) << "Unable to append to audio stream with sink state:" << state;
		}
	}
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
			updateState(QTextToSpeech::Error);
			break;

		case QAudio::State::IdleState:
			_playSentry = false;
			// When the queue is empty, set the ready state otherwise play the next queued buffer.
			if (_worker.isQueueEmpty())
				updateState(QTextToSpeech::Ready);
			else
			{
				// Play the next buffer queued.
				_worker.handleQueued([&](TextToSpeechSynthesizer::QueueFlag flag, const QByteArray& buffer) -> bool {
					if (flag == TextToSpeechSynthesizer::QueueFlag::qfPlay)
						playBuffer(buffer);
					return true;
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
