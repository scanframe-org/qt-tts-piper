# Engine & Worker {#qt-tts-engine}

## General

The main plugin class is `::sf::TextToSpeechEngine` which contains a worker thread for text-to-speech synthesis.  
The worker thread `::sf::TextToSpeechSynthesizer` is responsible for processing text input and generating audio
output using the Piper1GPL engine.
The main thread is responsible for passing the synthesized audio to the audio output device.  
This all is event-driven.

## Sequences

This sequence diagram illustrates the interaction between the Qt TTS Engine and the synthesizer worker thread.

@startuml sequence-diagram
!include ./qt-tts-engine.puml
@enduml