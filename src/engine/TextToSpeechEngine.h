#pragma once
#include "TextToSpeechSynthesizer.h"
#include <QAudioSink>
#include <QBuffer>
#include <QSettings>
#include <QtTextToSpeech/QTextToSpeechEngine>

namespace sf
{

class TextToSpeechEngine : public QTextToSpeechEngine
{
		Q_OBJECT

	public:
		explicit TextToSpeechEngine(const QVariantMap& parameters, QObject* parent = nullptr);

		/// @brief Overridden from base clas.
		~TextToSpeechEngine() override;

		/// @brief Overridden from base clas.
		QList<QLocale> availableLocales() const override;

		/// @brief Overridden from base clas.
		QList<QVoice> availableVoices() const override;

		/// @brief Overridden from base clas.
		void say(const QString& text) override;

		/// @brief Overridden from base clas.
		void synthesize(const QString& text) override;

		/// @brief Overridden from base clas.
		void stop(QTextToSpeech::BoundaryHint boundaryHint) override;

		/// @brief Overridden from base clas.
		void pause(QTextToSpeech::BoundaryHint boundaryHint) override;

		/// @brief Overridden from base clas.
		void resume() override;

		/// @brief Overridden from base clas.
		double rate() const override;

		/// @brief Overridden from base clas.
		bool setRate(double rate) override;

		/// @brief Overridden from base clas.
		double pitch() const override;

		/// @brief Overridden from base clas.
		bool setPitch(double pitch) override;

		/// @brief Overridden from base clas.
		QLocale locale() const override;

		/// @brief Overridden from base clas.
		bool setLocale(const QLocale& locale) override;

		/// @brief Overridden from base clas.
		double volume() const override;

		/// @brief Overridden from base clas.
		bool setVolume(double volume) override;

		/// @brief Overridden from base clas.
		QVoice voice() const override;

		/// @brief Overridden from base clas.
		bool setVoice(const QVoice& voice) override;

		/// @brief Overridden from base clas.
		QTextToSpeech::State state() const override;

		/// @brief Overridden from base clas.
		QTextToSpeech::ErrorReason errorReason() const override;

		/// @brief Overridden from base clas.
		QString errorString() const override;

		/// @brief Overridden from base clas.
		QTextToSpeech::Capabilities capabilities() const override;

		void playQueued();

	private:
		/// @brief triggers a status update signal when the state actually changes.
		void updateState(QTextToSpeech::State newState);

		/// @brief Gets the audio sink instance pointer.
		QAudioSink* getAudioSink();

		/// @brief plays the raw buffer to the default audio device.
		void playBuffer(const TextToSpeechSynthesizer::buffer_t& audio_data);

		/// @brief Handles the state changes of the audio sink and starts a queued buffer to play.
		void AudioSinkStateChange(QAudio::State state);

		/// @brief Gets the model and gender from the file name.
		QPair<QString, QVoice::Gender> getModelGenderFromFilename(const QString& filename, int speaker_id);

		/**
		 * @brief Converts a Qt rate (-1.0 to 1.0) to a speaking speed factor.
		 * @param rate The normalized rate from QTextToSpeech.
		 * @param minFactor The multiplier for the slowest speed (default 0.5x).
		 * @param maxFactor The multiplier for the fastest speed (default 2.0x).
		 * @return A double representing the speed multiplier.
		 */
		static float convertRateToFactor(double rate, double minFactor = 0.5, double maxFactor = 2.0);

		/// @brief Optional model settings for adding speaker gender info from a model filename and speaker index.
		QSettings _settings;

		/// Current state of the engine.
		QTextToSpeech::State _state;
		/// Last error reason.
		QTextToSpeech::ErrorReason _errorReason;
		/// Last error reason string.
		QString _errorString;
		/// Holds the directory of this plugin.
		QDir _dirPlugin;
		/// Holds the directory of models.
		QDir _dirModels;
		/// Holds the directory location of 'espeak_ng_data'.
		QDir _dirEspeakNgData;
		/// Holds the selected locale, voice, volume, and audio format.
		struct
		{
			/// Holds the selected locale.
			QLocale locale;
			/// Holds the selected voice index where -1 is not selected.
			int voice{-1};
			/// Holds the audio format.
			QAudioFormat audioFormat;
			/// Holds the volume set.
			double volume;
		} _selected;
		/// Holds the list of available locales.
		QList<QLocale> _locales;
		/// Holds the list of available voices.
		QList<QVoice> _voices;
		/// Holds the rate of speaking in a range from -1.0 to 1.0.
		double _rate;
		/// Type to pass to the voice.
		typedef QPair<QString, int> FileSpeaker;
		/// Instance to play the synthesized audio buffer.
		QAudioSink* _audioSink{nullptr};
		/// Buffer instance to pass to the QAudioSink instance.
		QBuffer* _buffer{nullptr};
		///
		bool _playSentry{false};
		//
		TextToSpeechSynthesizer _worker;
};

}// namespace sf
