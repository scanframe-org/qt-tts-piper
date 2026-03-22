#include "test-Window.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QSettings>

int checkPlugin(const QString& path)
{
	QPluginLoader loader(path);
	qDebug() << "Meta data:" << loader.metaData();
	qDebug() << "Error:" << loader.errorString();
	return 0;
}

// return checkPlugin(fi.absolutePath() + QDir::separator() + "lib" + QDir::separator() + "libsf-tts-piper.so");

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	auto path = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("lib");
	qDebug() << "Adding:" << path;
	QCoreApplication::addLibraryPath(path);
	qDebug() << QCoreApplication::libraryPaths();
	// Initialize the base using the application file path.
	QFileInfo fi(QCoreApplication::applicationFilePath());
	// Set the instance to change the extension only.
	QString fp = QFileInfo(fi.absolutePath()).absolutePath() + QDir::separator() + "config";
	fi.setFile(fp, fi.completeBaseName() + ".ini");
	// Create an instance to handle settings.
	QSettings settings(fi.absoluteFilePath(), QSettings::IniFormat);
	//
	auto win = new sf::MainWindow(nullptr);
	win->setSettings(settings);
	win->storeState(false);
	win->show();
	auto rv = app.exec();
	win->storeState(true);
	delete win;
	//
	return rv;
}
