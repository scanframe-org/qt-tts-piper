#pragma once
#include <QtTextToSpeech/QTextToSpeechEngine>

namespace sf
{

class TextToSpeechEngine : public QTextToSpeechEngine
{
		Q_OBJECT

	public:
		explicit TextToSpeechEngine(const QVariantMap& parameters, QObject* parent = nullptr);

		~TextToSpeechEngine() override;

		QList<QLocale> availableLocales() const override;
		QList<QVoice> availableVoices() const override;
		void say(const QString& text) override;
		void synthesize(const QString& text) override;
		void stop(QTextToSpeech::BoundaryHint boundaryHint) override;
		void pause(QTextToSpeech::BoundaryHint boundaryHint) override;
		void resume() override;
		double rate() const override;
		bool setRate(double rate) override;
		double pitch() const override;
		bool setPitch(double pitch) override;
		QLocale locale() const override;
		bool setLocale(const QLocale& locale) override;
		double volume() const override;
		bool setVolume(double volume) override;
		QVoice voice() const override;
		bool setVoice(const QVoice& voice) override;
		QTextToSpeech::State state() const override;
		QTextToSpeech::ErrorReason errorReason() const override;
		QString errorString() const override;

	private:
		void initializeVoices();
		void updateState(QTextToSpeech::State newState);

		QTextToSpeech::State m_state;
		QTextToSpeech::ErrorReason m_errorReason;
		QString m_errorString;

		double m_rate;
		double m_pitch;
		double m_volume;

		QLocale m_locale;
		QVoice m_voice;
		QList<QLocale> m_locales;
		QList<QVoice> m_voices;
};

}// namespace sf
