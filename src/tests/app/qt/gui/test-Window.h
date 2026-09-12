#pragma once
#include <QAudioDevice>
#include <QSettings>
#include <QTextToSpeech>
#include <QMainWindow>

namespace sf
{

namespace Ui
{
// Forward definition of auto-generated class.
class MainWindow;
}// namespace Ui

class MainWindow : public QMainWindow
{
		Q_OBJECT

	public:
		/**
		 * @brief Constructor.
		 */
		explicit MainWindow(QWidget* parent = nullptr);

		/**
		 * @brief Virtual destructor.
		 */
		~MainWindow() override;

		void setSettings(QSettings* settings);
		void storeState(bool save, bool text_only = false);
		void setRate(int);
		void setPitch(int);
		void setVolume(int volume);
		void stateChanged(QTextToSpeech::State state);
		void selectAudioDevice(int index);
		void selectEngine(int index);
		void selectLanguage(int language);
		void selectVoice(int index);
		void localeChanged(const QLocale &locale);

	private:
		void onEngineReady();

		Ui::MainWindow* ui;
		QSettings* _settings;
		QTextToSpeech* _tts;
		QString _audioDeviceId;
		QList<QVoice> _voices;
};




}// namespace sf
