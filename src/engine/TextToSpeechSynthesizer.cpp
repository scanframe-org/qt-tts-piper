#include "TextToSpeechSynthesizer.h"
#include "helpers.h"
#include <QAudioSink>
#include <QDebug>

namespace sf
{

using namespace Qt::StringLiterals;

TextToSpeechSynthesizer::~TextToSpeechSynthesizer()
{
	{
		QMutexLocker locker(&_mutexCondition);
		// Signal the loop to stop.
		_abort = true;
		// Wake the thread if it's waiting.
		_condition.wakeOne();
	}
	// Wait for the actual system thread to finish.
	// This blocks the main thread briefly until run() returns
	wait();
}

void TextToSpeechSynthesizer::setEspeakNgDataDirectory(const QString& dir)
{
	// Only call this before the thread is running and piper is initialized at the thread's start.
	Q_ASSERT(!isRunning());
	_piperEspeakNgData = dir.toStdString();
}

void TextToSpeechSynthesizer::setVoice(const QString& data_file, const QString& json_file, int speaker_id)
{
	QMutexLocker locker(&_mutexData);
	// Changing the speaker is no problem.
	if (_voiceSpeakerId != speaker_id)
	{
		_voiceSpeakerId = speaker_id;
		// Signal setting only the speaker.
		_voiceReload = -1;
	}
	//
	if (_voiceDataFile != data_file || _voiceJsonFile != json_file)
	{
		_voiceDataFile = data_file;
		_voiceJsonFile = json_file;
		// Signal reloading of piper voice model files.
		_voiceReload = 1;
	}
}

void TextToSpeechSynthesizer::setLengthScaleFactor(float scale_factor)
{
	QMutexLocker locker(&_mutexData);
	// Assign the new value.
	_lengthScaleFactor = scale_factor;
	// Apply the setting at the next available opportunity.
	_voiceReload = -1;
}

void TextToSpeechSynthesizer::run()
{
	// Process loop.
	while (true)
	{
		_mutexCondition.lock();
		// Wait while there is no work and we aren't stopping
		while (_queueText.isEmpty() && !_abort)
			_condition.wait(&_mutexCondition);
		// Check if the loop should be broken after being woken up.
		if (_abort)
		{
			_mutexCondition.unlock();
			// Gracefully exit the run() function.
			break;
		}
		//
		_mutexCondition.unlock();
		// Perform the heavy lifting outside the lock
		synthesize();
	}
	// Clean up.
	piper_free(_piper);
	_piper = nullptr;
}

void TextToSpeechSynthesizer::trigger()
{
	QMutexLocker locker(&_mutexCondition);
	_condition.wakeOne();
}

void TextToSpeechSynthesizer::add(const QStringList& lines)
{
	QMutexLocker locker(&_mutexData);
	// Append the lines at the end of the worker list.
	for (auto& line: lines)
		if (!line.isEmpty())
			_queueText.append(line);
	// Trigger the thread when it is waiting for work.
	trigger();
}

void TextToSpeechSynthesizer::clear()
{
	QMutexLocker locker(&_mutexData);
	// Clear any work in the queue to be yet synthesized.
	_queueText.clear();
	// Clear the already synthesized buffers.
	_buffers.clear();
}

void TextToSpeechSynthesizer::synthesize()
{
	QMutexLocker locker(&_mutexData);
	// When a load or reload is required.
	if (_voiceReload)
	{
			// Load the voice when larger than zero.
			if (_voiceReload > 0 || !_piper)
			{
				// Sanity check.
				if (!_voiceDataFile.isEmpty() && !_piperEspeakNgData.empty())
				{
					// Initialize the Piper engine.
					try
					{
						qDebug() << "Piper model:" << _voiceDataFile;
						_piper = piper_create(_voiceDataFile.toStdString().data(), _voiceJsonFile.toStdString().data(), _piperEspeakNgData.string().data());
						// Assign the default options.
						_options = piper_default_synthesize_options(_piper);
						_lengthScale = _options.length_scale;
						// Make the deviations from the default be applied.
						_voiceReload = -1;
					}
					catch (const std::exception& e)
					{
						qWarning() << "Piper initialize error:" << e.what();
						piper_free(_piper);
						_piper = nullptr;
					}
				}
			}
			if (_voiceReload < 0)
			{
				// Apply the deviations from the default.
				_options.speaker_id = _voiceSpeakerId;
				// Apply the rate to the default length scale.
				_options.length_scale = _lengthScale * _lengthScaleFactor;
			}
			// Clear the reload flag.
			_voiceReload = 0;
	}
	//
	while (!_queueText.isEmpty())
	{
		// Get an entry from the queue to synthesize.
		if (!locker.isLocked())
			locker.relock();
		// Get the
		auto text = _queueText.dequeue();
		// Allow access to the queue again.
		locker.unlock();
		//
		qDebug() << "Synthesizing:" << text;
		// Start synthesizing.
		int result = piper_synthesize_start(_piper, text.toStdString().c_str(), &_options);
		if (result == PIPER_ERR_GENERIC)
		{
			qWarning() << "Piper synthesize error";
			break;
		}
		piper_audio_chunk chunk;
		while ((result = piper_synthesize_next(_piper, &chunk)) != PIPER_DONE)
		{
			if (result == PIPER_ERR_GENERIC)
			{
				qWarning() << "Synthesis next failed!";
				break;
			}
			// Prevent access to the data again.
			locker.relock();
			// Play the data by adding it to the buffer.
			_buffers.enqueue(buffer_t(chunk.samples, chunk.samples + chunk.num_samples));
			// Notify main thread.
			emit synthesisQueued();
			// Allow access to the data again.
			locker.unlock();
		}
	}
}

bool TextToSpeechSynthesizer::handleQueued(const callback_t& callback)
{
	QMutexLocker locker(&_mutexData);
	if (!_buffers.isEmpty())
	{
		callback(_buffers.dequeue());
		return true;
	}
	return false;
}

bool TextToSpeechSynthesizer::isQueueEmpty()
{
	QMutexLocker locker(&_mutexData);
	return _buffers.isEmpty();
}
}// namespace sf
