#include "TextToSpeechSynthesizer.h"
#include "helpers.h"
#include "logging.h"
#include <QAudioSink>

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

void TextToSpeechSynthesizer::add(QueueFlag flag, const QString& text)
{
	QMutexLocker locker(&_mutexData);
	// When requesting synthesizing, clear the queued text and buffers.
	if (flag > qfSend)
	{
		_buffers.clear();
		_queueText.clear();
	}
	_queueText.append({flag, text});
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
	if (_voiceReload || !_piper)
	{
			// Load the voice when larger than zero.
			if (_voiceReload > 0 || !_piper)
			{
				// Sanity check on the required files.
				if (_voiceDataFile.isEmpty() || _piperEspeakNgData.empty())
				{
					qCWarning(logCategoryEngine) << "One of the required 'espeak-ng-data' files missing!";
					return;
				}
				// Initialize the Piper engine.
				try
				{
					_piper = piper_create(_voiceDataFile.toStdString().data(), _voiceJsonFile.toStdString().data(), _piperEspeakNgData.string().data());
					// Assign the default options.
					_options = piper_default_synthesize_options(_piper);
					_lengthScale = _options.length_scale;
					// Make the deviations from the default be applied.
					_voiceReload = -1;
				}
				catch (const std::exception& e)
				{
					qCWarning(logCategoryEngine) << "Piper initialize error:" << e.what();
					piper_free(_piper);
					_piper = nullptr;
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
	// Sanity check.
	if (!_piper)
	{
		qCWarning(logCategoryEngine) << "Piper not initialized!";
		return;
	}
	//
	while (!_queueText.isEmpty())
	{
		// Get an entry from the queue to synthesize.
		if (!locker.isLocked())
			locker.relock();
		// Get the next text from the queue.
		auto text = _queueText.dequeue();
		// Allow access to the queue again.
		locker.unlock();
		// Do not try to synthesize an empty string.
		if (text.second.isEmpty())
			continue;
		// Notify the start of synthesis.
		qCDebug(logCategoryEngine) << "To synthesize text:" << text;
		// Start synthesizing.
		int result = piper_synthesize_start(_piper, text.second.toStdString().c_str(), &_options);
		if (result == PIPER_ERR_GENERIC)
		{
			qCWarning(logCategoryEngine) << "Piper synthesize error!" << text;
			break;
		}
		piper_audio_chunk chunk;
		while ((result = piper_synthesize_next(_piper, &chunk)) != PIPER_DONE)
		{
			// Report the size of the synthesized chunk.
			qCDebug(logCategoryEngine) << "Synthesized chunk bytes:" << chunk.num_samples * sizeof(sample_t);
			if (result == PIPER_ERR_GENERIC)
			{
				qCWarning(logCategoryEngine) << "Synthesis next failed!";
				break;
			}
			// Prevent access to the data again.
			locker.relock();
			// Play the data by adding it to the buffer or return it as synthesized.
			_buffers.enqueue({text.first, {reinterpret_cast<const char*>(chunk.samples), static_cast<qsizetype>(chunk.num_samples * sizeof(sample_t))}});
			// Notify main thread when audio can be played.
			if (text.first == qfPlay)
				emit synthesisQueued();
			// Allow access to the data again.
			locker.unlock();
		}
		// Notify main thread when a buffer is synthesized to be send.
		if (text.first == qfSend)
			emit synthesisQueued();
	}
}

bool TextToSpeechSynthesizer::handleQueued(const callback_t& callback)
{
	QMutexLocker locker(&_mutexData);
	if (!_buffers.isEmpty())
	{
		// Run the loop while there are more buffers to process when not playing.
		do
		{
			auto [flag, buffer](_buffers.dequeue());
			qCDebug(logCategoryEngine) << "Handling buffer:" << _buffers.size() << "Flag:" << flag << ", Size:" << buffer.size();
			if (flag == qfPlay)
				callback(qfPlay, buffer);
			else
				callback(_buffers.isEmpty() ? qfSendEnd : qfSend, buffer);
		} while (!_buffers.isEmpty());
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
