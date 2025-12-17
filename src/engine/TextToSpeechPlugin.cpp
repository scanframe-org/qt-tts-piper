#include "TextToSpeechPlugin.h"
#include "TextToSpeechEngine.h"

QTextToSpeechEngine* SfTextToSpeechPlugin::createTextToSpeechEngine(const QVariantMap& parameters, QObject* parent, QString*) const
{
	qDebug() << __FUNCTION__;
	return new sf::TextToSpeechEngine(parameters, parent);
}
