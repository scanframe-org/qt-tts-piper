#include "AudioStream.h"

#include "logging.h"

#include <QAudioSink>
#include <qloggingcategory.h>

namespace sf
{

AudioStream::AudioStream(QObject* parent)
	: QIODevice(parent)
{
	// Let Qt know it's a read-only provider.
	QIODevice::open(ReadOnly);
}

void AudioStream::appendData(const QByteArray& chunk)
{
	// Scope to prevent dead lock when 'readyRead' is emitted and calls 'readData(...).
	{
		QMutexLocker locker(&_mutex);
		_buffer.append(chunk);
	}
	emit readyRead();
}

qint64 AudioStream::bytesAvailable() const
{
	QMutexLocker locker(&_mutex);
	return _buffer.size() + QIODevice::bytesAvailable();
}

bool AudioStream::isSequential() const
{
	return false;
}

bool AudioStream::clear()
{
	QMutexLocker locker(&_mutex);
	_buffer.clear();
	return true;
}

qint64 AudioStream::readData(char* data, qint64 maxSize)
{
	QMutexLocker locker(&_mutex);
	if (_buffer.isEmpty())
		// No data ready. Return 0 to tell sink we are waiting for more.
		return 0;
	// Pull out what the sink requested up to what we actually have.
	auto sz = qMin(_buffer.size(), maxSize);
	memcpy(data, _buffer.constData(), sz);
	// Flush read bytes out of buffer.
	_buffer.remove(0, sz);
	return sz;
}

qint64 AudioStream::writeData(const char*, qint64)
{
	return 0;
}
}// namespace sf