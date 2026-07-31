#pragma once
#include <QByteArray>
#include <QMutex>
#include <qaudiosink.h>

namespace sf
{

class AudioStream : public QIODevice
{
		Q_OBJECT

	public:
		/// @brief Constructor.
		explicit AudioStream(QObject* parent = nullptr);

		/// @brief Appends freshly arrived live data chunks into our safe queue.
		void appendData(const QByteArray& chunk);

		/// @brief Tell Qt how much unread data is sitting in memory.
		qint64 bytesAvailable() const override;

		/// @brief Identifies this as an unseekable live streaming queue.
		bool isSequential() const override;

		/// @brief Clears the stream.
		bool clear();

	protected:
		/// @brief QAudioSink automatically fires this when hardware needs samples.
		qint64 readData(char* data, qint64 maxSize) override;

		/// @brief Unused.
		qint64 writeData(const char*, qint64) override;

	private:
		QByteArray _buffer;
		mutable QMutex _mutex;
};

}// namespace sf
