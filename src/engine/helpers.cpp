#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#if WIN32
	#if __MINGW32__
		#include <windows.h>
	#else
		#include <Windows.h>
	#endif
#else
	#include <linux/limits.h>
	#include <unistd.h>
	#include <dlfcn.h>
#endif

namespace sf
{
std::filesystem::path getLibraryPath(void* func = nullptr)
{
	if (!func)
		func = reinterpret_cast<void*>(getLibraryPath);
#if WIN32
	HMODULE handle = nullptr;
	// Get the handle for the module containing this function address
	if (func && ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) func, &handle))
	{
		std::string rv(MAX_PATH, '\0');
		rv.resize(::GetModuleFileNameA(handle, rv.data(), rv.capacity()));
		return rv;
	}
	return {};
#else
	Dl_info dl_info;
	// Pass the address of any function in the target library
	if (func && ::dladdr(func, &dl_info) && dl_info.dli_fname)
		return dl_info.dli_fname;
	return {};
#endif
}

std::filesystem::path getExecutablePath()
{
#if WIN32
	std::string rv(MAX_PATH, '\0');
	rv.resize(::GetModuleFileNameA(nullptr, rv.data(), rv.capacity()));
	return rv;
#else
	std::string rv(PATH_MAX, '\0');
	rv.resize(readlink("/proc/self/exe", rv.data(), rv.capacity()));
	return rv;
#endif
}

QJsonDocument readJsonFile(const QString& filepath)
{
	QFile file(filepath);
	// Open the file.
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Could not open file for reading";
		return {};
	}
	// Read all content
	QByteArray data = file.readAll();
	file.close();
	// Parse the document
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	// 4. Handle parsing errors
	if (doc.isNull())
	{
		qWarning() << "Failed to create JSON doc:" << error.errorString();
		return {};
	}
	return doc;
}
}// namespace sf