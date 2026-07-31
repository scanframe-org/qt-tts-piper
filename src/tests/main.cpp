#include <QLoggingCategory>
#include <QThreadPool>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <iostream>
#include "catch.h"
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(_MSC_VER)
	#include <windows.h>

void exitNow(int exitcode)
{
	// #if defined(_MSC_VER)
	qInfo() << "Terminating abnormally due to bug GCC #116159";
	// #endif
	TerminateProcess(GetCurrentProcess(), exitcode);
}
#endif

int main(int argc, char* argv[])
{
	QSharedPointer<QCoreApplication> app(new QCoreApplication(argc, argv));
	auto path = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("lib");
	qInfo() << "Adding library path:" << path;
	QCoreApplication::addLibraryPath(path);
	qInfo() << QCoreApplication::libraryPaths();
	// Initialize the base using the application file path.
	QFileInfo fi(QCoreApplication::applicationFilePath());
	// Set the instance to change the extension only.
	QString fp = QFileInfo(fi.absolutePath()).absolutePath() + QDir::separator() + "config";
	fi.setFile(fp, fi.completeBaseName() + ".ini");
	// Create an instance to handle settings.
	QSettings settings(fi.absoluteFilePath(), QSettings::IniFormat);
	// There must be exactly one instance
	Catch::Session session;
	// Function calling catch command line processor.
	auto func = [&]() -> int {
		// Let Catch (using Clara) parse the command line
		int returnCode = session.applyCommandLine(argc, argv);
		if (returnCode != 0)
		{
			// Indicates a command line error
			QCoreApplication::instance()->exit(returnCode);
			return returnCode;
		}
		auto rv = session.run();
		QCoreApplication::instance()->exit(rv);
		return rv;
	};
	QTimer::singleShot(0, func);
	auto rv = QCoreApplication::instance()->exec();
// MinGW GCC < 13.4 detection for thread_local destructor ordering bug GCC #116159"
#if (defined(__MINGW32__) || defined(__MINGW64__)) && (__GNUC__ < 13 || (__GNUC__ == 13 && __GNUC_MINOR__ < 4))
	QThreadPool::globalInstance()->waitForDone();
	exitNow(rv);
#endif
	return rv;
}
