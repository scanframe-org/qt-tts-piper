#include "../catch.h"
#include <QAudioFormat>
#include <QBuffer>
#include <QElapsedTimer>
#include <QAudioSink>
#include <iostream>
#include <QCoreApplication>
#include <QTextToSpeech>
#include <QEventLoop>
#include <QMediaDevices>
#include <QMetaEnum>
#include <QTimer>
#include <filesystem>

namespace
{

bool isQemu()
{
#if !WIN32
	// QEMU user-mode (Docker with binfmt_misc): the host registers an emulator handler.
	if (std::filesystem::exists("/proc/sys/fs/binfmt_misc/qemu-aarch64"))
	{
		return true;
	}
	// QEMU system emulation (a full VM) with virtio devices.
	if (std::filesystem::exists("/sys/bus/virtio"))
	{
		return true;
	}
#endif
	return false;
}

// Returns the factor to multiply test timings with.
int timeFactor()
{
	static const int factor = []() -> int {
		// An explicit setting always wins, e.g. TEST_TIMEOUT_FACTOR=10.
		if (qEnvironmentVariableIsSet("TEST_TIMEOUT_FACTOR"))
		{
			return qMax(1, qEnvironmentVariableIntValue("TEST_TIMEOUT_FACTOR"));
		}
		return isQemu() ? 3 : 1;
	}();
	return factor;
}

void qSleep(const int milliseconds)
{
	QEventLoop loop;
	QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
	loop.exec();
}

// Process events until the predicate is true or the scaled timeout expires.
bool qWaitFor(const int timeout, const std::function<bool()>& pred, const int finalTimeout = 0)
{
	QElapsedTimer timer;
	timer.start();
	while (!pred())
	{
		// Check the predicate.
		if (timer.hasExpired(timeout * timeFactor()))
		{
			// Give just a bit of time to finish up.
			if (finalTimeout)
				QCoreApplication::processEvents(QEventLoop::AllEvents, finalTimeout);
			return false;
		}
		QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
	}
	return true;
}

void qPlayBuffer(const QByteArray& data, const QAudioFormat& format, QObject* parent = nullptr)
{
	// ReSharper disable once CppDFAMemoryLeak
	auto buffer = new QBuffer(parent);
	buffer->setData(data);
	buffer->open(QIODevice::ReadOnly);
	// ReSharper disable once CppDFAMemoryLeak
	auto sink = new QAudioSink(format, parent);
	if (!sink->format().isValid())
	{
		INFO("Audio sink format is NOT valid, bailing playing.");
		return;
	}
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

TEST_CASE("tts-piper-0")
{
	SECTION("OutputDevices")
	{
		QMediaDevices devices;
		if (devices.audioOutputs().size() == 0)
			WARN("No audio output devices are available.");
		REQUIRE(devices.audioOutputs().size() > 0);
	}
}

TEST_CASE("tts-piper-1", "[say]")
{
	if (!engineAvailable(engine_name))
		SKIP(QString("Skipping because engine '%1' is not available.").arg(engine_name).toStdString());

	// Used as parent for QObject derived instances.
	QSharedPointer<QObject> parent(new QObject);

	SECTION("Speak")
	{
		INFO("Creating text-to-speech engine.");
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
		QString text(
			"The is first line synthesized. "
			"And this is the second line.\n"
			"Continuing with the third line. "
			"Finalizing with this fourth line."
		);
		INFO(std::string("Speaking: ").append(text.toStdString()));
		tts->say(text);
		// tts->say("Hello Everybody, this is an example text in English.");
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		qWaitFor(30000, [tts]() -> bool {
			return tts->state() != QTextToSpeech::Speaking;
		});
		// When shorter then 5 seconds it did not process all lines.
		REQUIRE(stopwatch.durationElapsed() > 6s);
		// Say a test message and enqueue another.
		tts->say("Hello, world!");
		tts->enqueue("This is a long text for speaking and I want to be interrupted.");
		tts->enqueue("If you here this the previous one was not interrupted.");
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		// Wait 2 seconds before interrupting.
		qSleep(2000);
		// Check if still speaking.
		REQUIRE(tts->state() == QTextToSpeech::Speaking);
		// Interrupt the speech after 2 seconds.
		tts->say("I was successfully interrupted.");
		// Process events until not speaking and maximum 10 seconds.
		qWaitFor(
			10000,
			[tts]() -> bool {
				INFO("Waiting until speech finishes.");
				return tts->state() == QTextToSpeech::Ready;
			},
			2000
		);
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
		INFO("Creating text-to-speech engine.");
		// ReSharper disable once CppDFAMemoryLeak
		auto tts = new QTextToSpeech(QString(engine_name), parent.get());
		if (tts->state() == QTextToSpeech::State::Error)
			SKIP("Skipping because the engine is reporting an error!");
		// Attach a state signal handler.
		QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
			qDebug() << "State Change Signal: " << state;
		});
		// By default, English from the US is available using the 'models.py' script ans setting the locale will set a default voice as well.
		tts->setLocale(QLocale(QLocale::Language::English, QLocale::Territory::UnitedKingdom));
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
				CHECK(format.isValid());
				CHECK(!buffer.isEmpty());
				if (format.isValid() && !buffer.isEmpty())
				{
					qDebug() << "Synthesized data" << format << buffer.size();
					qPlayBuffer(buffer, format, parent.get());
				}
			}
		);
		REQUIRE(tts->state() == QTextToSpeech::Synthesizing);
		// Wait for synthesis to complete, up to 10 seconds.
		qInfo() << "Wait for synthesis to complete, up to 15 seconds.";
		//
		qWaitFor(15000, [tts]() -> bool {
			INFO("Waiting for Synthesis to finish.");
			auto state = tts->state();
			return state != QTextToSpeech::Synthesizing;
		});
		//
		auto state = tts->state();
		qInfo() << "State: " << state;
		REQUIRE(state == QTextToSpeech::Ready);
		//
		qWaitFor(100, [parent]() -> bool {
			auto sinks = parent->findChildren<QAudioSink*>();
			if (sinks.size() > 0 && !sinks[0]->format().isValid())
			{
				INFO("Audio sink format is not valid. Stopped waiting!");
				return true;
			}
			return false;
		});
	}
}
