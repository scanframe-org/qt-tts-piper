#pragma once
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <filesystem>
#include <piper.h>

namespace sf
{

/**
 * @brief Worker thread to synthesize the audio data from lines of text.
 */
class TextToSpeechSynthesizer : public QThread
{
		Q_OBJECT
	public:
		/// @brief Piper used buffer type.
		typedef float sample_t;

		/// @brief Queue flag for controlling the synthesis process.
		enum QueueFlag
		{
			/// Play the buffer on the audio device.
			qfPlay,
			/// Send the buffer to the caller as this is the not last one.
			qfSend,
			/// Send the buffer to the caller and signal that this is the last one.
			qfSendEnd
		};

		/// @brief Callback type for playing the buffer.
		typedef std::function<void(QueueFlag, const QByteArray&)> callback_t;

		/// Use the base class constructor.
		using QThread::QThread;

		/// @brief Overridden from base class.
		~TextToSpeechSynthesizer() override;

		/// @brief Sets the location of the 'espeak_ng_data' directory.
		void setEspeakNgDataDirectory(const QString& dir);

		/**
		 * Sets the voice parameters for text-to-speech synthesis.
		 * @param data_file Location of the voice data file.
		 * @param json_file Location of the voice JSON configuration file.
		 * @param speaker_id Identifier of the speaker within the voice configuration.
		 */
		void setVoice(const QString& data_file, const QString& json_file, int speaker_id);

		/**
		 * @brief Sets the factor to modify the length scale of the voice.
		 * @param scale_factor Value between 0.05 and 2.0 where other values are clipped.
		 */
		void setLengthScaleFactor(float scale_factor);

		/**
		 * @brief Signals the main thread that synthesis is queued.
		 */
		Q_SIGNAL void synthesisQueued();

		/**
		 * @brief Handles the next audio buffer queued buffer in a thread-safe way using a callback function.
		 * @param callback
		 * @return True when a buffer was handled and false when the queue was empty.
		 */
		bool handleQueued(const callback_t& callback);

		/**
		 * @brief Gets the buffer queue empty status in a thread safe way.
		 */
		bool isQueueEmpty();

	protected:
		/// @brief Overridden form base class.
		void run() override;

		/// @brief Triggers the thread when idle.
		void trigger();

		/// @brief Synthesizes the queued lines.
		void synthesize();

	public:
		/// @brief Add work to the queue to be synthesized.
		void add(QueueFlag flag, const QString& text);

		/// @brief Clear any work from being synthesized.
		void clear();

	private:
		/// Mutex for condition triggering.
		QMutex _mutexCondition;
		/// Mutex for shared resource locking.
		QRecursiveMutex _mutexData;
		/// Condition to trigger when there is work.
		QWaitCondition _condition;
		/// List of texts to synthesize and speak.
		QQueue<QPair<QueueFlag, QString>> _queueText;
		/// Buffers synthesized and ready to be played.
		QQueue<QPair<QueueFlag, QByteArray>> _buffers;
		/// Flag set when a (re)load of the voice is needed.
		int _voiceReload{0};
		/// Flag set when the thread should terminate.
		std::atomic<bool> _abort{false};
		/// Holds the piper engine configuration.
		piper_synthesizer* _piper{nullptr};
		/// Holds the piper synthesize options.
		piper_synthesize_options _options{0, 0, 0, 0};
		/// Holds espeak_ng_data directory location.
		std::filesystem::path _piperEspeakNgData;
		/// Holds the voice model data file.
		QString _voiceDataFile;
		/// Holds the voice JSON information file.
		QString _voiceJsonFile;
		/// Holds the selected speaker id.
		int _voiceSpeakerId{0};
		/// Holds the initial length scale of the selected model.
		float _lengthScale{1.0};
		/// Holds the rate to modify the length scale with.
		float _lengthScaleFactor{1.0};
};

}// namespace sf
