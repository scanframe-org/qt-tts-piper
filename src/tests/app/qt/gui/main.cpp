#include "test-Window.h"
#include <QApplication>
#include <QThreadPool>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSettings>
#if defined(__MINGW32__) || defined(__MINGW64__)
	#include <windows.h>

void exitNow(int exitcode)
{
	qInfo() << "Terminating abnormally due to bug GCC #116159";
	TerminateProcess(GetCurrentProcess(), exitcode);
}
#endif

Q_LOGGING_CATEGORY(logCategoryApp, "qt.tts.app", QtMsgType::QtWarningMsg)

#define _STRINGIFY_(x) #x
#define _TOSTRING_(x) _STRINGIFY_(x)

QString getVersion()
{
	return QString(_TOSTRING_(SF_VERSION));
}

int main(int argc, char* argv[])
{
	// Use the alternative env var since Wine does not propagate 'QT_' prefixed vars.
	if (qEnvironmentVariableIsSet("SF_QT_LOGGING_RULES"))
	{
		QString rules = qEnvironmentVariable("SF_QT_LOGGING_RULES");
		rules.replace(QLatin1Char(';'), QLatin1Char('\n'));
		QLoggingCategory::setFilterRules(rules);
	}
	// Use the alternative env var since Wine does not propagate 'QT_' prefixed vars.
	if (qEnvironmentVariableIsSet("SF_QT_LOGGING_RULES"))
	{
		qSetMessagePattern(qEnvironmentVariable("SF_QT_MESSAGE_PATTERN"));
	}
	// Set some application information.
	QCoreApplication::setOrganizationName("Scanframe");
	QCoreApplication::setOrganizationDomain("scanframe.nl");
	QCoreApplication::setApplicationName("Qt-Text-To-Speech");
	// Initialize the application.
	QApplication app(argc, argv);
#ifdef WIN32
	app.setStyle("Fusion");
#endif
	// Get the 'lib' directory relative to the application.
	auto path = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("lib");
	// Log and add the library path.
	qCInfo(logCategoryApp) << "Adding library path:" << path;
	QCoreApplication::addLibraryPath(path);
	// Log the total library paths.
	qCInfo(logCategoryApp) << "Library paths:" << QCoreApplication::libraryPaths();
	// Get the full application file path.
	QFileInfo file_info(QCoreApplication::applicationFilePath());
	// Form the expected settings file directory.
	QString file_dir = QFileInfo(file_info.absolutePath()).absolutePath() + QDir::separator() + "config";
	// Add the settings file path.
	file_info.setFile(file_dir, file_info.completeBaseName() + ".ini");
	// Declare a self destruct pointer for the settings.
	std::unique_ptr<QSettings> settings;
	// Check if the configuration directory exists?
	if (QFileInfo::exists(file_dir))
	{
		// Create an instance to handle settings using the .
		settings = std::make_unique<QSettings>(file_info.absoluteFilePath(), QSettings::IniFormat);
	}
	else
	{
		// Form the settings file path in the users home directory for Linux as well as for Windows.
		settings = std::make_unique<QSettings>(
			QSettings::IniFormat, QSettings::UserScope,//
			QCoreApplication::organizationName(), QCoreApplication::applicationName()
		);
	}
	qCInfo(logCategoryApp) << "Using settings from:" << settings->fileName();
	//
	auto win = std::make_unique<sf::MainWindow>();
	win->setWindowIcon(QIcon(":/icons/main"));
	win->setWindowTitle(win->windowTitle() + ' ' + getVersion());
	win->setSettings(settings.get());
	win->storeState(false);
	win->show();
	auto rv = app.exec();
	win->storeState(true);
	win.release();
	// Required, otherwise not all is stored.
	settings->sync();
	settings.release();
// MinGW GCC < 13.4 detection for thread_local destructor ordering bug GCC #116159"
#if (defined(__MINGW32__) || defined(__MINGW64__)) && (__GNUC__ < 13 || (__GNUC__ == 13 && __GNUC_MINOR__ < 4))
	QThreadPool::globalInstance()->waitForDone();
	exitNow(rv);
#endif
	return rv;
}
