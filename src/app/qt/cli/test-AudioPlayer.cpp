#include "test-AudioPlayer.h"

#include <QEventLoop>
#include <QDebug>

AudioPlayer::AudioPlayer(QObject* parent)
	: QObject(parent)
	, _sink(nullptr)
{
	// Open the buffer in ReadOnly mode so QAudioSink can pull data safely
	_buffer.open(QIODevice::ReadOnly);
}

AudioPlayer::~AudioPlayer()
{
	if (_sink)
		_sink->stop();
	_buffer.close();
}

void AudioPlayer::play(const QByteArray& data, const QAudioFormat& format)
{
	if (!_sink || _format != format)
	{
		recreateSink(format);
	}

	const bool needsRestart = (_sink->state() == QAudio::IdleState || _sink->state() == QAudio::StoppedState);
	if (needsRestart)
	{
		_buffer.close();
		_buffer.setData(QByteArray());
		_buffer.open(QIODevice::ReadOnly);
	}

	// Append data directly to the underlying QByteArray without affecting
	// QIODevice's internal read position pointer.
	_buffer.buffer().append(data);

	if (needsRestart)
	{
		_buffer.seek(0);
		_sink->start(&_buffer);
	}
}

void AudioPlayer::waitUntilDone()
{
	if (!_sink || (_sink->state() == QAudio::IdleState && _buffer.atEnd()) || _sink->state() == QAudio::StoppedState)
		return;
	QEventLoop loop;
	QMetaObject::Connection conn;
	conn = connect(_sink, &QAudioSink::stateChanged, [&loop, &conn, this](QAudio::State state) {
		if ((state == QAudio::IdleState && _buffer.atEnd()) || (state == QAudio::StoppedState && _sink->error() != QAudio::NoError))
		{
			disconnect(conn);
			loop.quit();
		}
	});
	loop.exec();
}

void AudioPlayer::recreateSink(const QAudioFormat& format)
{
	if (_sink)
	{
		_sink->stop();
		_sink->deleteLater();
		_sink = nullptr;
	}
	_format = format;
	_buffer.close();
	_buffer.setData(QByteArray());
	_buffer.open(QIODevice::ReadOnly);
	_sink = new QAudioSink(_format, this);
}