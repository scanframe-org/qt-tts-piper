#pragma once
#include <QJsonObject>
#include <QMetaEnum>
#include <filesystem>

namespace sf
{

/**
 * Gets the path to a dynamic library containing the given function.
 * @param func A pointer to a function in the library.
 * @return Path to the library.
 */
std::filesystem::path getLibraryPath(void* func = nullptr);

/**
 * Gets the path to the current executable.
 * @return Executable path.
 */
std::filesystem::path getExecutablePath();

/**
 * Gets a QDocument from a given json-file.
 * @param filepath Filepath to the json-file to read.
 * @return QJsonDocument
 * Example:
 * @code
 * auto sample_rate = readJsonFile("my-config.json").object()["audio"].toObject()["sample_rate"].toInt();
 * @endcode
 */
QJsonDocument readJsonFile(const QString& filepath);

/**
 * @brief Turns a QT enumerate type into a named key.
 * @tparam T Enumerate type.
 * @param value Enum value.
 * @return Key name of the enumeration value.
 */
template<typename T>
static const char* enumToKey(const T value)
{
	return QMetaEnum::fromType<T>().valueToKey(value);
}

/**
 * @brief Turns a QT enumerate typed key name into an enumeration value.
 * @tparam T Enumerate type.
 * @param key Key name
 * @param ok Flag set when no key was found.
 * @return Enumerate value.
 */
template<typename T>
static T keyToEnum(const char* key, bool* ok = nullptr)
{
	return static_cast<T>(QMetaEnum::fromType<T>().keyToValue(key, ok));
}

}// namespace sf
