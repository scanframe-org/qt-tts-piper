#pragma once

#include <QAudioSink>
#include <QBuffer>
#include <QAudioFormat>

/**
 * @brief A class to handle text-to-speech audio playback.
 *
 * This class is responsible for playing audio data with a specified format
 * and handles the management of the audio sink.
 */
class AudioPlayer : public QObject
{
		Q_OBJECT
	public:
		/**
		 * Qt default constructor.
		 */
		explicit AudioPlayer(QObject* parent = nullptr);

		/**
		 * Destructor.
		 */
		~AudioPlayer() override;

		/**
		 * @brief Plays the given audio data with the specified format.
		 * @param data The audio data to play.
		 * @param format The format of the audio data.
		 */
		void play(const QByteArray& data, const QAudioFormat& format);

		/**
		 * @brief Blocks until the audio playback is done.
		 */
		void waitUntilDone();

	private:
		/**
		 * @brief Recreates the audio sink with the given format.
		 * @param format The new format for the audio sink.
		 */
		void recreateSink(const QAudioFormat& format);
		/// @brief Holds the audio sink for playing.
		QAudioSink* _sink;
		/// @brief Holds the data buffer for playing. Note: QBuffer has a single
		/// shared cursor for both reading and writing - appends must save/restore
		/// that cursor around the write, see AudioPlayer::play().
		QBuffer _buffer;
		/// @brief Holds the current format.
		QAudioFormat _format;
};
