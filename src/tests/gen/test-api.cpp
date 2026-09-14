#include "../catch.h"
#include <QAudioFormat>
#include <QBuffer>
#include <QElapsedTimer>
#include <QAudioSink>
#include <iostream>
#include <QCoreApplication>
#include <QTextToSpeech>
#include <QEventLoop>
#include <QMetaEnum>
#include <QTimer>

namespace
{

void qSleep(int milliseconds)
{
	QEventLoop loop;
	QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
	loop.exec();
}

void qPlayBuffer(const QByteArray& data, const QAudioFormat& format, QObject* parent = nullptr)
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

bool engineAvailable(const QStringView& name)
{
	auto engines = QTextToSpeech::availableEngines();
	qInfo() << "Available Speech Engines:" << engines;
	INFO("Available Speech Engines:  " << engines.join(", ").toStdString());
	return engines.indexOf(name) >= 0;
}

}// namespace

using namespace std::chrono_literals;

constexpr QStringView engine_name(u"Piper");

TEST_CASE("tts-piper-1", "[say]")
{
	if (!engineAvailable(engine_name))
		SKIP(QString("Skipping because no engine '%1' is not available.").arg(engine_name).toStdString());

	// Used as parent for QObject derived instances.
	QSharedPointer<QObject> parent(new QObject);

	SECTION("Speak")
	{
		// ReSharper disable once CppDFAMemoryLeak
		QTextToSpeech _tts(parent.get());
		_tts.setEngine(QString(engine_name));
		auto tts = &_tts;// new QTextToSpeech(QString(engine_name), parent.get());
		if (tts->state() == QTextToSpeech::State::Error)
			SKIP("Skipping because the engine is reporting an error!");
		// Check if Piper is selected.
		REQUIRE(tts->engine() == engine_name);
		// Attach a state signal handler.
		QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
			qDebug() << "State Change Signal: " << state;
		});
		// Check for appropriate engine capabilities.
		if (!(tts->engineCapabilities() & QTextToSpeech::Capability::Speak))
			SKIP("Skipping because feature is not supported on by this engine.");
		// Some engines initialize asynchronously.
		if (tts->state() != QTextToSpeech::Ready)
		{
			QObject::connect(tts, &QTextToSpeech::stateChanged, [tts] {
				qDebug() << QMetaEnum::fromType<QTextToSpeech::State>().valueToKey(tts->state());
				REQUIRE(tts->state() == QTextToSpeech::Ready);
			});
			// Process events and return after 100 ms.
			qSleep(100);
		}
		else
			REQUIRE(tts->state() == QTextToSpeech::Ready);

		// Check if voice models are available.
		REQUIRE(tts->findVoices().size() > 0);

		// Set a locale language of which models are not available.
		QLocale locale(QLocale::Language::German, QLocale::Territory::Germany);
		tts->setLocale(locale);
		// Check if the locale was not able to set correctly.
		REQUIRE(tts->locale() != locale);

		// By default, English from the US is available using the 'models.py' script.
		locale = QLocale(QLocale::Language::English, QLocale::Territory::UnitedStates);
		tts->setLocale(locale);
		// Check if the locale was set correctly.
		REQUIRE(tts->locale() == locale);

		// Get the list of available voices from the last set locale.
		auto voices = tts->availableVoices();
		int voice_num = 1;
		// Check if the voice index is in range before setting it.
		REQUIRE(voices.size() > voice_num);
		tts->setVoice(voices[voice_num]);
		// Check if the voice was set correctly.
		REQUIRE(tts->voice() == voices[voice_num]);
		// Timer to time the duration of processing and speaking.
		QElapsedTimer stopwatch;
		stopwatch.start();
		// A long text duration test.
		/*
		tts->say(
			"Hello Everybody, this is an example text in English.\n"
			"QtSpeech is a library that makes text to speech easy with Qt.\n"
			"This is the end, the end of the speech example.\n"
			"Thank you for listening."
		);
		*/
		tts->say(
			"The is first line synthesized. "
			"An this is the second line.\n"
			"Continuing with the third line. "
			"Finalizing with The fourth line."
		);
		// tts->say("Hello Everybody, this is an example text in English.");
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		while (tts->state() == QTextToSpeech::Speaking && stopwatch.durationElapsed() < 30s)
		{
			qSleep(500);
		}
		// When shorter then 5 seconds it did not process all lines.
		REQUIRE(stopwatch.durationElapsed() > 8s);
		// Say a test message and enqueue another.
		tts->say("Hello, world!");
		tts->enqueue("This is a long text for speaking.");
		tts->enqueue("If you here this the previous one was not interrupted.");
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		// Wait 2 seconds before interrupting.
		qSleep(2000);
		// Check if still speaking.
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		// Interrupt the speech after 2 seconds.
		tts->say("Interrupted!");
		// Process events until not speaking and maximum 10 seconds.
		for (int i = 0; i < 200 && tts->state() == QTextToSpeech::Speaking; i++)
			qSleep(50);
		// State must be Ready after this.
		REQUIRE(tts->state() == QTextToSpeech::Ready);
	}
}

TEST_CASE("tts-piper-2", "[synth]")
{
	if (!engineAvailable(engine_name))
		SKIP(QString("Skipping because no engine '%1' is not available.").arg(engine_name).toStdString());

	// Used as parent for QObject derived instances.
	QSharedPointer<QObject> parent(new QObject);

	SECTION("Synthesize")
	{
		// ReSharper disable once CppDFAMemoryLeak
		auto tts = new QTextToSpeech(QString(engine_name), parent.get());
		if (tts->state() == QTextToSpeech::State::Error)
			SKIP("Skipping because the engine is reporting an error!");
		// Attach a state signal handler.
		QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
			qDebug() << "State Change Signal: " << state;
		});
		// Check for appropriate engine capabilities.
		if (!(tts->engineCapabilities() & QTextToSpeech::Capability::Synthesize))
			SKIP(QString("Skipping because feature is not supported on by engine '%1'.").arg(tts->engine()).toStdString());
		// Some engines initialize asynchronously.
		if (tts->state() != QTextToSpeech::Ready)
		{
			QObject::connect(tts, &QTextToSpeech::stateChanged, [tts] {
				qDebug() << QMetaEnum::fromType<QTextToSpeech::State>().valueToKey(tts->state());
				REQUIRE(tts->state() == QTextToSpeech::Ready);
			});
			// Process events and return after 100 ms.
			qSleep(100);
		}
		else
			REQUIRE(tts->state() == QTextToSpeech::Ready);
		// Use the synthesize function with a callback lambda function.
		tts->synthesize(
			"This text is played externally. This is the second sentence using a separate process step.",
			[parent](const QAudioFormat& format, const QByteArray& buffer) {
				REQUIRE(format.isValid());
				REQUIRE(!buffer.isEmpty());
				qDebug() << "Synthesized data" << format << buffer.size();
				qPlayBuffer(buffer, format, parent.get());
			}
		);
		REQUIRE(tts->state() == QTextToSpeech::Synthesizing);
		// Wait for synthesis to complete, up to 10 seconds.
		for (int i = 0; i < 30; i++)
		{
			qSleep(500);
			auto state = tts->state();
			qInfo() << "State: " << state;
			if (state != QTextToSpeech::Synthesizing)
				break;
		}
		auto state = tts->state();
		qInfo() << "State: " << state;
		REQUIRE(state == QTextToSpeech::Ready);
		while (parent->findChildren<QAudioSink*>().size() > 0)
			qSleep(100);
	}
}
