#include <QLoggingCategory>
#include <QPointer>
#include <QThreadPool>
#include <QDir>
#include <QSettings>
#include <QCommandLineParser>
#include <QFile>
#include <QTimer>
#include <QTextToSpeech>
#include "test-AudioPlayer.h"

Q_LOGGING_CATEGORY(logApp, "qt.tts.app", QtMsgType::QtWarningMsg)

#if (defined(__MINGW32__) || defined(__MINGW64__)) && (__GNUC__ < 13 || (__GNUC__ == 13 && __GNUC_MINOR__ < 4))
	#include <windows.h>

void exitNow(int exitcode)
{
	qCInfo(logApp) << "Terminating abnormally due to bug GCC #116159";
	TerminateProcess(GetCurrentProcess(), exitcode);
}
#endif

#ifdef _MSC_VER
	#define PACKED_STRUCT(name) __pragma(pack(push, 1)) struct name
	#define PACKED_STRUCT_END __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
	#define PACKED_STRUCT(name) struct __attribute__((packed)) name
	#define PACKED_STRUCT_END
#else
	#define PACKED_STRUCT(name) struct name
	#define PACKED_STRUCT_END
#endif

PACKED_STRUCT(WavHeader)
{
	// RIFF Chunk.
	[[maybe_unused]] char riff[4] = {'R', 'I', 'F', 'F'};
	// Total size - 8.
	uint32_t fileSize;
	char wave[4] = {'W', 'A', 'V', 'E'};
	// Format Chunk.
	char fmt[4] = {'f', 'm', 't', ' '};
	// Size of fmt chunk
	[[maybe_unused]] uint32_t fmtSize = 16;
	// PCM = 1, 3 = WAVE_FORMAT_IEEE_FLOAT.
	uint16_t audioFormat = 3;
	// Mono.
	uint16_t numChannels = 1;
	uint32_t sampleRate;
	// sampleRate * 1 * 4.
	uint32_t byteRate;
	// channels * bytesPerSample (1 * 4).
	uint16_t blockAlign = 4;
	uint16_t bitsPerSample = 32;
	// Fact Chunk (Required for non-PCM formats)
	[[maybe_unused]] char fact[4] = {'f', 'a', 'c', 't'};
	// Number of samples (dataSize / blockAlign).
	[[maybe_unused]] uint32_t factSize = 4;
	uint32_t sampleLength;
	// Data Chunk.
	char data[4] = {'d', 'a', 't', 'a'};
	// Raw data size in bytes.
	uint32_t dataSize;
};

PACKED_STRUCT_END;

bool saveWav(const QString& fileName, const QAudioFormat& format, const QByteArray& rawData)
{
	WavHeader header;

	// Map Qt Format to WAV Specs
	bool isFloat = (format.sampleFormat() == QAudioFormat::Float);
	header.audioFormat = isFloat ? 3 : 1;
	header.numChannels = format.channelCount();
	header.sampleRate = format.sampleRate();
	header.bitsPerSample = format.bytesPerSample() * 8;
	// Calculations
	header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
	header.byteRate = header.sampleRate * header.blockAlign;
	header.dataSize = rawData.size();
	header.sampleLength = header.dataSize / header.blockAlign;
	// Total file size: Struct size (56) + Data size - 8 bytes (RIFF & Size fields).
	header.fileSize = sizeof(WavHeader) + header.dataSize - 8;
	//
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	// Write the struct and then the raw data.
	file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
	file.write(rawData);
	file.close();
	return true;
}

#define _STRINGIFY_(x) #x
#define _TOSTRING_(x) _STRINGIFY_(x)

QString getVersion()
{
	return QString(_TOSTRING_(SF_VERSION));
}

void playBuffer(const QByteArray& data, const QAudioFormat& format, QObject* parent = nullptr)
{
	// ReSharper disable once CppDFAMemoryLeak
	auto buffer = new QBuffer(parent);
	buffer->setData(data);
	buffer->open(QIODevice::ReadOnly);
	// ReSharper disable once CppDFAMemoryLeak
	auto sink = new QAudioSink(format, parent);
	QEventLoop loop;
	QMetaObject::Connection conn;
	// When the sink finishes (IdleState), clean up and unblock the event loop.
	conn = QObject::connect(sink, &QAudioSink::stateChanged, [&loop, &conn, sink, buffer](QAudio::State state) {
		if (state == QAudio::IdleState)
		{
			QObject::disconnect(conn);// prevent re-entry from stop() emitting StoppedState
			sink->stop();
			buffer->close();
			sink->deleteLater();
			buffer->deleteLater();
			loop.quit();
		}
	});
	// Start playing, then block until done or 10s timeout.
	sink->start(buffer);
	QTimer::singleShot(10000, &loop, &QEventLoop::quit);
	loop.exec();
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
	// The environment variable has precedence.
	else if (qEnvironmentVariableIsEmpty("QT_LOGGING_RULES"))
		QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg=false");
	//
	QPointer app(new QCoreApplication(argc, argv));
	QCoreApplication::setApplicationName("Text-To-Speech Qt-Plugin CLI Tester");
	QCoreApplication::setApplicationVersion(getVersion());
	// Initialize the parser.
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::applicationName() + ' ' + QCoreApplication::applicationVersion());
	// Add default help and version options (-h, --help and -v, --version).
	parser.addHelpOption();
	parser.addVersionOption();
	// Define a boolean flag (switch).
	QCommandLineOption opt_info({"i", "info"}, "Show engines and voices information.");
	QCommandLineOption opt_play({"p", "play"}, "Play the synthesized text.");
	// Define an option that takes the TTS engine.
	QCommandLineOption opt_engine({"e", "engine"}, "Engine to use.", "engine", "");
	// Define an option that takes the TTS locale.
	QCommandLineOption opt_locale({"l", "locale"}, "Locale to use.", "locale", "");
	// Option for selecting a voice.
	QCommandLineOption opt_voice({"o", "voice"}, "Voice to use.", "voice", "");
	// Option to set the speech rate.
	QCommandLineOption opt_rate({"r", "rate"}, "Rate of speech with a range of -1.0 to 1.0 and 0.0 as default value.", "rate", "0.0");
	// Define an option that takes the TTS engine.
	QCommandLineOption opt_filepath({"f", "filepath"}, "Filepath for output wav-files.", "filepath", "output");
	// Define an option that takes the text value.
	QCommandLineOption opt_text({"t", "text"}, "Text to synthesize.", "text", "");
	// Add the command line options to the parser.
	parser.addOptions({opt_info, opt_engine, opt_locale, opt_voice, opt_rate, opt_play, opt_filepath, opt_text});
	// Process the arguments passed to the application
	parser.process(*app);
	//
	auto isInfo = parser.isSet(opt_info);
	auto text = parser.value(opt_text);
	//
	QPointer<AudioPlayer> audioPlayer;
	if (parser.isSet(opt_play))
	{
		audioPlayer = new AudioPlayer(app.get());
	}
	//
	auto path = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("lib");
	qCInfo(logApp) << "Adding library path:" << path;
	QCoreApplication::addLibraryPath(path);
	qCInfo(logApp) << QCoreApplication::libraryPaths().join(QLatin1Char('\n'));
	// Create a default one when 'engine' is not specified or the specified named engine.
	// ReSharper disable once CppDFAMemoryLeak
	QPointer tts(new QTextToSpeech(parser.value(opt_engine), app.get()));
	// When a locale is passed,
	if (!parser.value(opt_locale).isEmpty())
	{
		QLocale locale(parser.value(opt_locale));
		tts->setLocale(locale);
		if (tts->locale() != locale)
			qWarning() << "Locale could not be set:" << tts->locale().name();
	}
	// Set the rate of speech.
	if (!parser.value(opt_rate).isEmpty())
		tts->setRate(parser.value(opt_rate).toDouble());
	// Set the voice when found.
	if (!parser.value(opt_voice).isEmpty())
	{
		for (auto& voice: tts->availableVoices())
		{
			auto voice_name = parser.value(opt_voice);
			if (voice.name() == voice_name)
				tts->setVoice(voice);
		}
		if (tts->voice().name() != parser.value(opt_voice))
			qWarning() << "Voice could not be set:" << tts->voice().name();
	}
	//
	if (isInfo)
	{
		qInfo().noquote() << "Available Engines:" << tts->availableEngines().join(", ");
		qInfo().noquote() << "Current Engine:" << tts->engine();
		QStringList locales;
		for (auto& locale: tts->availableLocales())
			locales << locale.name();
		qInfo().noquote() << QString("%1 Locales:").arg(tts->engine()) << locales.join(", ");
		for (auto& voice: tts->availableVoices())
			qInfo().noquote() << QString("%1 Voice:").arg(tts->engine()) << voice.name() << voice.locale().name() << QVoice::genderName(voice.gender())
												<< QVoice::ageName(voice.age());
		qInfo().noquote() << "Speech rate:" << tts->rate();
	}
	// Check for appropriate engine capabilities.
	if (!(tts->engineCapabilities() & QTextToSpeech::Capability::Synthesize))
	{
		qCritical() << "Engine does not support synthesis.";
		return 1;
	}
	// Just bailout when not text is to be synthesized.
	if (text.isEmpty())
	{
		qInfo() << "No text to synthesize.";
		return 0;
	}
	// Flag for when synthesis has been initiated.
	bool synthesized = false;
	// Counter for naming files.
	int file_counter = 0;
	// Performs synthesis exactly once, guarded by the synthesized flag.
	auto doSynthesize = [&]() {
		// Bailout when called.
		if (synthesized)
			return;
		// Set flag to prevent re-entry.
		synthesized = true;
		qCDebug(logApp) << "TTS Synthesize:" << text;
		tts->synthesize(text, [&](const QAudioFormat& format, const QByteArray& buffer) {
			if (format.isValid() && !buffer.isEmpty())
			{
				auto filepath = parser.value(opt_filepath) + QString("-%1.wav").arg(file_counter++);
				// Check if audio is to be played.
				if (audioPlayer)
				{
					// playBuffer(buffer, format, app.get());
					audioPlayer->play(buffer, format);
				}
				else
				{
					qCDebug(logApp) << "Synthesized data" << format << buffer.size() << "Saving to:" << filepath;
					saveWav(filepath, format, buffer);
				}
			}
		});
	};
	// Trigger synthesis on Ready; quit when Ready is signaled after synthesis completes.
	QObject::connect(tts, &QTextToSpeech::stateChanged, app.get(), [&](QTextToSpeech::State state) {
		static bool started = false;
		if (state == QTextToSpeech::Ready && !started)
		{
			if (!synthesized)
			{
				started = true;
				doSynthesize();
			}
			else
			{
				// When audio is played wait until it is finished play before quiting the application event loop.
				if (audioPlayer)
				{
					audioPlayer->waitUntilDone();
				}
				app->quit();
			}
		}
		else if (state == QTextToSpeech::Error)
		{
			qCritical() << "TTS error:" << tts->errorString();
			app->quit();
		}
	});
	// If engine is already Ready at startup, stateChanged won't fire — synthesize immediately.
	if (tts->state() == QTextToSpeech::Ready)
		doSynthesize();
	// Start the application event loop.
	auto rv = app->exec();
	//
	// MinGW GCC < 13.4 detection for thread_local destructor ordering bug GCC #116159"
#if (defined(__MINGW32__) || defined(__MINGW64__)) && (__GNUC__ < 13 || (__GNUC__ == 13 && __GNUC_MINOR__ < 4))
	QThreadPool::globalInstance()->waitForDone();
	exitNow(rv);
#endif
	return rv;
}
