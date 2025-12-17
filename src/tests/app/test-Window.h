#pragma once
#include <QSettings>
#include <QTextToSpeech>
#include <QWidget>

namespace sf
{

namespace Ui
{
// Forward definition of auto-generated class.
class Window;
}// namespace Ui

class Window : public QWidget
{
		Q_OBJECT

	public:
		/**
		 * @brief Constructor.
		 */
		explicit Window(QWidget* parent = nullptr);

		/**
		 * @brief Virtual destructor.
		 */
		~Window() override;

		void storeState(QSettings& settings, bool save);
		void createTextToSpeech();
		void listInfo();
		void speakText();

	private:
		Ui::Window* ui;
		QTextToSpeech* _tts;
		QString _engineProvider;
};

}// namespace sf
