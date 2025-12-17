#pragma once
#include <QtTextToSpeech/QTextToSpeechPlugin>

class SfTextToSpeechPlugin
	: public QObject
	, public QTextToSpeechPlugin
{
		Q_OBJECT
		Q_PLUGIN_METADATA(IID "org.qt-project.qt.speech.tts.plugin/6.0" FILE "plugin.json")
		Q_INTERFACES(QTextToSpeechPlugin)

	public:
		QTextToSpeechEngine* createTextToSpeechEngine(const QVariantMap& parameters, QObject* parent, QString* errorString) const override;
};
