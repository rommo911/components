/**
 * @file File_2.cpp
 * @author rami
 * @brief 
 * @version 0.2
 * @date 2021-01-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "debug_tools.h"

#if (DEBUG_STORAGE == VERBOS)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGI
#elif (DEBUG_STORAGE == INFO)
#define LOG_STORAGE ESP_LOGI
#define LOG_STORAGE_V ESP_LOGD
#else
#define LOG_STORAGE ESP_LOGD
#define LOG_STORAGE_V ESP_LOGV
#endif

#include "File_2.h"
#include <sys/stat.h>
#include <esp_log.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "debug_tools.h"
/**
 * @brief Construct a file.
 * @param [in] name The name of the file.
 * @param [in] type The type of the file (DT_REGULAR, DT_DIRECTORY or DT_UNKNOWN).
 */
File_2::File_2(std::string path, uint8_t type)
{
	m_path = path;
	m_type = type;
	m_length = 0;
	m_data = nullptr;
	pointer = nullptr;
	m_modified = false;
} // File_2
File_2::~File_2()
{
	Close();
	if (m_data != nullptr)
		free(m_data);
}
/**
 * @brief Retrieve the content of the file.
 * @param [in] base64Encode Should we base64 encode the content?
 * @return The content of the file.
 */
std::string File_2::GetContent(bool base64Encode)
{
	const uint8_t *pData = GetContentP();
	if (pData == nullptr)
	{
		return std::string();
	}
	std::string ret((char *)pData, Length());
	return ret;
} // getContent

/**
 * @brief Retrieve the content of the file.
 * @param [in] base64Encode Should we base64 encode the content?
 * @return The content of the file.
 */
const uint8_t *File_2::GetContentP()
{
	uint32_t size = Length();
	LOG_STORAGE(m_path.c_str(), "path=%s, length=%d", m_path.c_str(), size);
	if (size == 0)
		return nullptr;
	FILE *file = fopen(m_path.c_str(), "r");
	if (file != NULL)
	{
		if (m_data != nullptr)
		{
			free(m_data);
		}
		uint8_t *m_data = (uint8_t *)calloc(1, size);
		if (m_data == nullptr)
		{
			LOGE(m_path.c_str(), "getContent: Failed to allocate memory");
			fclose(file);
			return nullptr;
		}
		fread(m_data, size, 1, file);
		fclose(file);
		return m_data;
	}
	else
	{
		return nullptr;
	}
} // getContent

/**
 * @brief Retrieve the content of the file.
 * @param [in] offset The file offset to read from.
 * @param [in] readSize The number of bytes to read.
 * @return The content of the file.
 */
std::string File_2::GetContent(uint32_t offset, uint32_t readSize)
{
	uint32_t fileSize = Length();
	LOG_STORAGE(m_path.c_str(), "File_2:: getContent(), name=%s, fileSize=%d, offset=%d, readSize=%d",
				m_path.c_str(), fileSize, offset, readSize);
	if (fileSize == 0 || offset > fileSize)
		return std::string("N/A");
	uint8_t *pData = (uint8_t *)malloc(readSize);
	if (pData == nullptr)
	{
		LOGE(m_path.c_str(), "getContent: Failed to allocate memory");
		return std::string("N/A");
	}
	FILE *file = fopen(m_path.c_str(), "r");
	fseek(file, offset, SEEK_SET);
	size_t bytesRead = fread(pData, 1, readSize, file);
	fclose(file);
	std::string ret((char *)pData, bytesRead);
	free(pData);
	return ret;
} // getContent

std::string File_2::GetPath()
{
	return m_path;
}
/**
 * @brief Get the name of the file.
 * @return The name of the file.
 */
std::string File_2::GetName()
{
	size_t pos = m_path.find_last_of('/');
	if (pos == std::string::npos)
		return m_path;
	return m_path.substr(pos + 1);
} // getName

/**
 * @brief Get the type of the file.
 * The type of a file can be DT_REGULAR, DT_DIRECTORY or DT_UNKNOWN.
 * @return The type of the file.
 */
uint8_t File_2::GetType()
{
	return m_type;
} // getName

/**
 * @brief Get the length of the file in bytes.
 * @return The length of the file in bytes.
 */
uint32_t File_2::Length()
{
	if (m_length != 0 && !m_modified)
		return m_length;
	struct stat buf;
	int rc = stat(m_path.c_str(), &buf);
	if (rc != 0 || S_ISDIR(buf.st_mode))
	{
		LOGW(m_path.c_str(), "Failed to get file size");
		return 0;
	}
	m_length = (uint32_t)buf.st_size;
	return m_length;
} // length

/**
 * @brief Determine if the type of the file is a directory.
 * @return True if the file is a directory.
 */
bool File_2::IsDirectory()
{
	struct stat buf;
	int rc = stat(m_path.c_str(), &buf);
	if (rc != 0)
		return false;
	return S_ISDIR(buf.st_mode);
} // isDirectory

bool File_2::exists()
{
	struct stat buf;
	int rc = stat(m_path.c_str(), &buf);
	if (rc != 0)
		return false;
	return true;
}

FILE *File_2::Open(const char *mode)
{
	FILE *f = fopen(m_path.c_str(), mode);
	if (f == NULL)
	{
		LOGE(m_path.c_str(), "Failed to open file for %s ", mode);
		pointer = nullptr;
		return (NULL);
	}
	else
	{
		pointer = f;
		//lasOpenedTime = FreeRTOS::millis();
		return (f);
	}
}

esp_err_t IRAM_ATTR File_2::Close()
{
	LOG_STORAGE_V(m_path.c_str(), "closing ");
	if (this->pointer != nullptr) // an open file is found : close it !
	{
		//fflush(lastOpened);
		int err = fclose(pointer);
		if (err == 0)
		{
			m_modified = true;
			pointer = NULL;
			//lastClosedTime = FreeRTOS::millis();
			return (ESP_OK);
		}
		else
		{
			//something went wrong
			pointer = NULL;
			LOGE(m_path.c_str(), "Failed to close file or no changes were made to file %d ", err);
			return (ESP_FAIL);
		}
	}
	else
	{
		LOG_STORAGE_V(m_path.c_str(), "No recently opened file...");
		return (ESP_OK);
	}
}