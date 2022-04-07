/*
 * File_2.h
 *
 *  Created on: Jun 1, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_FILE_H_
#define COMPONENTS_CPP_UTILS_FILE_H_
#pragma once

#include <string>
#include "esp_err.h"

#ifdef ESP_PLATFORM
#include "esp_attr.h"
#endif // ESP_PLATFORM

/**
 * @brief A logical representation of a file.
 */
class File_2
{
protected:
	static constexpr char TAG[] = "File_2";

public:
	File_2(std::string name, uint8_t type = 0);
	~File_2();
	File_2(const File_2 &t)
	{
		m_path = t.m_path;
	}
	File_2 &operator=(const File_2 &t)
	{
		m_path = t.m_path;
		return *this;
	}

	std::string GetContent(bool base64Encode = false);
	const uint8_t *GetContentP();
	std::string GetContent(uint32_t offset, uint32_t size);
	std::string GetName();
	std::string GetPath();
	uint8_t GetType();
	bool IsDirectory();
	uint32_t Length();
	FILE *Open(const char *mode = "r");
	FILE *pointer;
	esp_err_t  Close();
	bool exists();

private:
	bool m_modified = false;
	std::string m_path;
	uint8_t m_type;
	uint32_t m_length = 0;
	uint8_t *m_data = nullptr;
	unsigned long lastClosedTime = 0;
	unsigned long lasOpenedTime = 0;
};

#endif /* COMPONENTS_CPP_UTILS_FILE_H_ */
