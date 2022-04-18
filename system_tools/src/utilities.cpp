
#include "sdkconfig.h"
#ifdef ESP_PLATFORM
#include "esp32/rom/rtc.h"
#include "esp32/rom/spi_flash.h"
#include "esp_app_format.h"
#include "esp_heap_caps.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "multi_heap.h"
#include "soc/rtc.h"
#include <esp_partition.h>
#include <soc/efuse_reg.h>
#include <soc/soc.h>
#endif
#include "utilities.hpp"
#include "stdarg.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <sstream>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static int base64EncodedLength(size_t length)
{
	return (length + 2 - ((length + 2) % 3)) / 3 * 4;
} // base64EncodedLength

static int base64EncodedLength(const std::string& in)
{
	return base64EncodedLength(in.length());
} // base64EncodedLength

static void a3_to_a4(unsigned char* a4, unsigned char* a3)
{
	a4[0] = (a3[0] & 0xfc) >> 2;
	a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
	a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
	a4[3] = (a3[2] & 0x3f);
} // a3_to_a4

static void a4_to_a3(unsigned char* a3, unsigned char* a4)
{
	a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
	a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
	a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
} // a4_to_a3

tools::TimeDate_t tools::GetSystemTimeDate()
{
	time_t now;
	time(&now);
	tm timeinfo = {};
#ifdef ESP_PLATFORM
	localtime_r(&now, &timeinfo);
#endif
	TimeDate_t currentTime;
	currentTime.year = timeinfo.tm_year;
	currentTime.month = timeinfo.tm_mon;
	currentTime.day = timeinfo.tm_mday;
	currentTime.hour = timeinfo.tm_hour;
	currentTime.minute = timeinfo.tm_min;
	return currentTime;
}
/**
 * @brief Encode a string into base 64.
 * @param [in] in
 * @param [out] out
 */
bool tools::base64Encode(const std::string& in, std::string* out)
{
	int i = 0, j = 0;
	size_t enc_len = 0;
	unsigned char a3[3];
	unsigned char a4[4];

	out->resize(base64EncodedLength(in));

	int input_len = in.size();
	std::string::const_iterator input = in.begin();

	while (input_len--)
	{
		a3[i++] = *(input++);
		if (i == 3)
		{
			a3_to_a4(a4, a3);

			for (i = 0; i < 4; i++)
			{
				(*out)[enc_len++] = kBase64Alphabet[a4[i]];
			}

			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
		{
			a3[j] = '\0';
		}

		a3_to_a4(a4, a3);

		for (j = 0; j < i + 1; j++)
		{
			(*out)[enc_len++] = kBase64Alphabet[a4[j]];
		}

		while ((i++ < 3))
		{
			(*out)[enc_len++] = '=';
		}
	}

	return (enc_len == out->size());
} // base64Encode

/**
 * @brief Dump general info to the log.
 * Data includes:
 * * Amount of free RAM
 */
void tools::dumpInfo()
{
#ifdef ESP_PLATFORM
	size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	esp_chip_info_t chipInfo;
	esp_chip_info(&chipInfo);
	ESP_LOGI(TAG, "--- dumpInfo ---");
	ESP_LOGI(TAG, "Free heap: %d", freeHeap);
	ESP_LOGD(TAG, "Chip Info: Model: %d, cores: %d, revision: %d", chipInfo.model, chipInfo.cores, chipInfo.revision);
	ESP_LOGD(TAG, "ESP-IDF version: %s", esp_get_idf_version());
	ESP_LOGD(TAG, "---");
#endif
} // dumpInfo

multi_heap_info_t tools::GetHeapInfo(uint8_t type)
{

	multi_heap_info_t heapInfo;
#ifdef ESP_PLATFORM
	switch (type)
	{
	case 0:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);
		break;
	case 1:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_DEFAULT);
		break;
	case 3:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_SPIRAM);
		break;
	default:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_8BIT);
		break;
	}
#else
	heapInfo = 0;
#endif

	return heapInfo;
}
/**
 * Dump the storage stats for the heap.
 */
std::string tools::dumpHeapInfo(uint8_t type)
{
	multi_heap_info_t heapInfo;

	char* buf;
	std::string strRet("");
#ifdef ESP_PLATFORM

	switch (type)
	{
	case 0:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);
		printf("         %10s %10s %10s %10s %13s %11s %12s\n", "Free", "Allocated", "Largest", "Minimum", "Alloc Blocks", "Free Blocks", "Total Blocks");
		asprintf(&buf, "INTERNAL %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += (buf);
		break;
	case 1:
		heap_caps_get_info(&heapInfo, MALLOC_CAP_DEFAULT);
		printf("         %10s %10s %10s %10s %13s %11s %12s\n", "Free", "Allocated", "Largest", "Minimum", "Alloc Blocks", "Free Blocks", "Total Blocks");
		asprintf(&buf, "DEFAULT  %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		break;
	default:
		printf("         %10s %10s %10s %10s %13s %11s %12s\n", "Free", "Allocated", "Largest", "Minimum", "Alloc Blocks", "Free Blocks", "Total Blocks");
		heap_caps_get_info(&heapInfo, MALLOC_CAP_EXEC);
		asprintf(&buf, "EXEC     %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		heap_caps_get_info(&heapInfo, MALLOC_CAP_32BIT);
		asprintf(&buf, "32BIT    %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		heap_caps_get_info(&heapInfo, MALLOC_CAP_8BIT);
		asprintf(&buf, "8BIT     %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		heap_caps_get_info(&heapInfo, MALLOC_CAP_DMA);
		asprintf(&buf, "DMA      %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		heap_caps_get_info(&heapInfo, MALLOC_CAP_SPIRAM);
		asprintf(&buf, "SPISRAM  %10d %10d %10d %10d %13d %11d %12d\n", heapInfo.total_free_bytes, heapInfo.total_allocated_bytes, heapInfo.largest_free_block, heapInfo.minimum_free_bytes, heapInfo.allocated_blocks, heapInfo.free_blocks, heapInfo.total_blocks);
		strRet += buf;
		break;
	}
	printf("%s", strRet.c_str());
	free(buf);
#endif
	return strRet;
}

size_t tools::getMinimumFreeHeapSize()
{
#ifdef ESP_PLATFORM
	return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
#else
	return 0;
#endif
} // getMinimumFreeHeapSize
/**
 * @brief Does the string end with a specific character?
 * @param [in] str The string to examine.
 * @param [in] c The character to look form.
 * @return True if the string ends with the given character.
 */
bool tools::endsWith(std::string str, char c)
{
	if (str.empty())
	{
		return false;
	}
	if (str.at(str.length() - 1) == c)
	{
		return true;
	}
	return false;
} // endsWidth

static int DecodedLength(const std::string& in)
{
	int numEq = 0;
	int n = (int)in.size();

	for (std::string::const_reverse_iterator it = in.rbegin(); *it == '='; ++it)
	{
		++numEq;
	}
	return ((6 * n) / 8) - numEq;
} // DecodedLength

static unsigned char b64_lookup(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 71;
	if (c >= '0' && c <= '9')
		return c + 4;
	if (c == '+')
		return 62;
	if (c == '/')
		return 63;
	return 255;
}; // b64_lookup

/**
 * @brief Decode a chunk of data that is base64 encoded.
 * @param [in] in The string to be decoded.
 * @param [out] out The resulting data.
 */
bool tools::base64Decode(const std::string& in, std::string* out)
{
	int i = 0, j = 0;
	size_t dec_len = 0;
	unsigned char a3[3];
	unsigned char a4[4];

	int input_len = in.size();
	std::string::const_iterator input = in.begin();

	out->resize(DecodedLength(in));

	while (input_len--)
	{
		if (*input == '=')
		{
			break;
		}

		a4[i++] = *(input++);
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				a4[i] = b64_lookup(a4[i]);
			}

			a4_to_a3(a3, a4);

			for (i = 0; i < 3; i++)
			{
				(*out)[dec_len++] = a3[i];
			}

			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
		{
			a4[j] = '\0';
		}

		for (j = 0; j < 4; j++)
		{
			a4[j] = b64_lookup(a4[j]);
		}

		a4_to_a3(a3, a4);

		for (j = 0; j < i - 1; j++)
		{
			(*out)[dec_len++] = a3[j];
		}
	}

	return (dec_len == out->size());
} // base64Decode

/**
 * @brief Convert an IP address to string.
 * @param ip The 4 byte IP address.
 * @return A string representation of the IP address.
 */
std::string tools::ipToString(uint8_t* ip)
{
	std::stringstream s;
	s << (int)ip[0] << '.' << (int)ip[1] << '.' << (int)ip[2] << '.' << (int)ip[3];
	return s.str();
} // ipToString

/**
 * @brief Convert an IP address to string.
 * @param ip The 4 byte IP address.
 * @return A string representation of the IP address.
 */
std::string tools::ipToString(const esp_ip4_addr_t& ipadd)
{
	return tools::stringf(IPSTR, IP2STR(&ipadd));
} // ipToString

/**
 * @brief Dump a representation of binary data to the console.
 *
 * @param [in] pData Pointer to the start of data to be logged.
 * @param [in] length Length of the data (in bytes) to be logged.
 * @return N/A.
 */
void tools::hexDump(const uint8_t* pData, uint32_t length)
{
	char ascii[80];
	char hex[80];
	char tempBuf[80];
	uint32_t lineNumber = 0;

	ESP_LOGI(TAG, "     00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");
	ESP_LOGI(TAG, "     -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --");
	strcpy(ascii, "");
	strcpy(hex, "");
	uint32_t index = 0;
	while (index < length)
	{
		sprintf(tempBuf, "%.2x ", pData[index]);
		strcat(hex, tempBuf);
		if (isprint(pData[index]))
		{
			sprintf(tempBuf, "%c", pData[index]);
		}
		else
		{
			sprintf(tempBuf, ".");
		}
		strcat(ascii, tempBuf);
		index++;
		if (index % 16 == 0)
		{
			ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
			strcpy(ascii, "");
			strcpy(hex, "");
			lineNumber++;
		}
	}
	if (index % 16 != 0)
	{
		while (index % 16 != 0)
		{
			strcat(hex, "   ");
			index++;
		}
		ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
	}
} // hexDump

/**
 * @brief Split a string into parts based on a delimiter.
 * @param [in] source The source string to split.
 * @param [in] delimiter The delimiter characters.
 * @return A vector of strings that are the split of the input.
 */
std::vector<std::string> tools::split(std::string source, char delimiter)
{
	// See also: https://stackoverflow.com/questions/5167625/splitting-a-c-stdstring-using-tokens-e-g
	std::vector<std::string> strings;
	std::istringstream iss(source);
	std::string s;
	while (std::getline(iss, s, delimiter))
	{
		strings.push_back(trim(s));
	}
	return strings;
} // split

/**
 * @brief
 *
 * @param str
 * @return true
 * @return false
 */
 ///static const std::string pattern = "https?://(www.)?[-a-zA-Z0-9@:%._+~#=]{2,256}.[a-z]{2,4}\b([-a-zA-Z0-9@:%_+.~#?&//=]*)";
//static std::regex url_regex(pattern);
bool tools::IsValidUrl(const std::string& str)
{
	// regex pattern
	// Construct regex object
	// An url-string for example
	// Check for match
	//bool match = (std::regex_match(str, url_regex) == true);
	//if (!match)
	//{
		// throw(Aexception("Assert", "IsValidUrl failed"));
	//}
	return true;
}

/**
 * @brief Convert a string to lower case.
 * @param [in] value The string to convert to lower case.
 * @return A lower case representation of the string.
 */
std::string tools::toLower(std::string& value)
{
	// Question: Could this be improved with a signature of:
	// std::string& tools::toLower(std::string& value)
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	return value;
} // toLower

std::string tools::trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
		return str;
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
} // trim

#ifdef ESP_PLATFORM
/**
 * @brief Remove white space from a string.
 */

uint32_t tools::getCpuFreqMHz()
{
	rtc_cpu_freq_config_t conf;
	rtc_clk_cpu_freq_get_config(&conf);
	return conf.freq_mhz;
}
void tools::deepSleep(uint32_t time_us)
{
	esp_deep_sleep(time_us);
}

void tools::restart(void)
{
	esp_restart();
}

uint32_t tools::getHeapSize(void)
{
	multi_heap_info_t info;
	heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
	return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t tools::getFreeHeap(void)
{
	return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t tools::getMinFreeHeap(void)
{
	return heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t tools::getMaxAllocHeap(void)
{
	return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
}

uint32_t tools::getPsramSize(void)
{
	multi_heap_info_t info;
	heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
	return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t tools::getFreePsram(void)
{
	return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

uint32_t tools::getMinFreePsram(void)
{
	return heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
}

uint32_t tools::getMaxAllocPsram(void)
{
	return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
}

uint32_t tools::sketchSize(tools::sketchSize_t response)
{
	esp_image_metadata_t data;
	const esp_partition_t* running = esp_ota_get_running_partition();
	if (!running)
		return 0;
	const esp_partition_pos_t running_pos = {
		.offset = running->address,
		.size = running->size,
	};
	data.start_addr = running_pos.offset;
	esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);
	if (response)
	{
		return running_pos.size - data.image_len;
	}
	else
	{
		return data.image_len;
	}
}

uint32_t tools::getSketchSize()
{
	return sketchSize(tools::SKETCH_SIZE_TOTAL);
}

std::string tools::getSketchMD5()
{
	static std::string result;
	if (result.length())
	{
		return result;
	}
	//uint32_t lengthLeft = getSketchSize();

	const esp_partition_t* running = esp_ota_get_running_partition();
	if (!running)
	{
		return std::string();
	}
	const size_t bufSize = SPI_FLASH_SEC_SIZE;
	std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
	//uint32_t offset = 0;
	if (!buf.get())
	{
		return std::string();
	}

	return result;
}

std::string tools::getOTASketchMD5()
{
	static std::string result;
	if (result.length())
	{
		return result;
	}
	//uint32_t lengthLeft = getSketchSize();

	const esp_partition_t* running = esp_ota_get_next_update_partition(NULL);
	if (!running)
	{
		return std::string();
	}
	const size_t bufSize = SPI_FLASH_SEC_SIZE;
	std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
	///uint32_t offset = 0;
	if (!buf.get())
	{
		return std::string();
	}

	return result;
}

uint32_t tools::getFreeSketchSpace()
{
	const esp_partition_t* _partition = esp_ota_get_next_update_partition(NULL);
	if (!_partition)
	{
		return 0;
	}

	return _partition->size;
}

uint8_t tools::getChipRevision(void)
{
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	return chip_info.revision;
}

const char* tools::getSdkVersion(void)
{
	return esp_get_idf_version();
}

uint32_t tools::getFlashChipSize(void)
{
	esp_image_header_t fhdr;
	if (flashRead(0x1000, (uint32_t*)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
	{
		return 0;
	}
	return magicFlashChipSize(fhdr.spi_size);
}

uint32_t tools::getFlashChipSpeed(void)
{
	esp_image_header_t fhdr;
	if (flashRead(0x1000, (uint32_t*)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
	{
		return 0;
	}
	return magicFlashChipSpeed(fhdr.spi_speed);
}

tools::FlashMode_t tools::getFlashChipMode(void)
{
	esp_image_header_t fhdr;
	if (flashRead(0x1000, (uint32_t*)&fhdr, sizeof(esp_image_header_t)) && fhdr.magic != ESP_IMAGE_HEADER_MAGIC)
	{
		return FM_UNKNOWN;
	}
	return magicFlashChipMode(fhdr.spi_mode);
}

uint32_t tools::magicFlashChipSize(uint8_t byte)
{
	switch (byte & 0x0F)
	{
	case 0x0: // 8 MBit (1MB)
		return (1_MB);
	case 0x1: // 16 MBit (2MB)
		return (2_MB);
	case 0x2: // 32 MBit (4MB)
		return (4_MB);
	case 0x3: // 64 MBit (8MB)
		return (8_MB);
	case 0x4: // 128 MBit (16MB)
		return (16_MB);
	default: // fail?
		return 0;
	}
}

uint32_t tools::magicFlashChipSpeed(uint8_t byte)
{
	switch (byte & 0x0F)
	{
	case 0x0: // 40 MHz
		return (40_MHz);
	case 0x1: // 26 MHz
		return (26_MHz);
	case 0x2: // 20 MHz
		return (20_MHz);
	case 0xf: // 80 MHz
		return (80_MHz);
	default: // fail?
		return 0;
	}
}

tools::FlashMode_t tools::magicFlashChipMode(uint8_t byte)
{
	FlashMode_t mode = (FlashMode_t)byte;
	if (mode > FM_SLOW_READ)
	{
		mode = FM_UNKNOWN;
	}
	return mode;
}

bool tools::flashEraseSector(uint32_t sector)
{
	return spi_flash_erase_sector(sector) == ESP_OK;
}

// Warning: These functions do not work with encrypted flash
bool tools::flashWrite(uint32_t offset, uint32_t* data, size_t size)
{
	return spi_flash_write(offset, (uint32_t*)data, size) == ESP_OK;
}

bool tools::flashRead(uint32_t offset, uint32_t* data, size_t size)
{
	return spi_flash_read(offset, (uint32_t*)data, size) == ESP_OK;
}

uint64_t tools::getEfuseMac(void)
{
	uint64_t _chipmacid = 0LL;
	esp_efuse_mac_get_default((uint8_t*)(&_chipmacid));
	return _chipmacid;
}

#endif // #ifdef ESP_PLATFORM

std::string tools::stringf(const std::string fmt_str, ...)
{
	int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while (1)
	{
		formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
		strcpy(&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}

bool tools::CompareBssid(const char bssid[18], const char bssid2[18])
{
	return (strcmp(bssid, bssid2) == 0);
}

void tools::randomSeed(unsigned long seed)
{
	if (seed != 0)
	{
		srand(seed);
	}
}

long tools::random(long howbig)
{
#ifdef ESP_PLATFORM
#define RANDOM esp_random()
#else
#define RANDOM std::rand()
#endif
	uint32_t x = RANDOM;
	uint64_t m = uint64_t(x) * uint64_t(howbig);
	uint32_t l = uint32_t(m);
	if (l < howbig)
	{
		uint32_t t = -howbig;
		if (t >= howbig)
		{
			t -= howbig;
			if (t >= howbig)
				t %= howbig;
		}
		while (l < t)
		{
			x = RANDOM;
			m = uint64_t(x) * uint64_t(howbig);
			l = uint32_t(m);
		}
	}
	return m >> 32;
}

long tools::random(long howsmall, long howbig)
{
	if (howsmall >= howbig)
	{
		return howsmall;
	}
	long diff = howbig - howsmall;
	return random(diff) + howsmall;
}

long tools::map(long x, long in_min, long in_max, long out_min, long out_max)
{
	long divisor = (in_max - in_min);
	if (divisor == 0)
	{
		return -1; // AVR returns -1, SAM returns 0
	}
	return (x - in_min) * (out_max - out_min) / divisor + out_min;
}

unsigned int tools::makeWord(unsigned char h, unsigned char l)
{
	return (h << 8) | l;
}

void tools::reverse(char* begin, char* end)
{
	char* is = begin;
	char* ie = end - 1;
	while (is < ie)
	{
		char tmp = *ie;
		*ie = *is;
		*is = tmp;
		++is;
		--ie;
	}
}

char* tools::ltoa(long value, char* result, int base)
{
	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;
	long quotient = abs(value);

	do
	{
		const long tmp = quotient / base;
		*out = "0123456789abcdef"[quotient - (tmp * base)];
		++out;
		quotient = tmp;
	} while (quotient);

	// Apply negative sign
	if (value < 0)
		*out++ = '-';

	reverse(result, out);
	*out = 0;
	return result;
}

char* tools::ultoa(unsigned long value, char* result, int base)
{
	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;
	unsigned long quotient = value;

	do
	{
		const unsigned long tmp = quotient / base;
		*out = "0123456789abcdef"[quotient - (tmp * base)];
		++out;
		quotient = tmp;
	} while (quotient);

	reverse(result, out);
	*out = 0;
	return result;
}

void tools::macstr_to_uint(const char* str, uint8_t bssid[6])
{
	{
		char* pEnd;
		bssid[0] = strtol(str, &pEnd, 16);
		bssid[1] = strtol(pEnd + 1, &pEnd, 16);
		bssid[2] = strtol(pEnd + 1, &pEnd, 16);
		bssid[3] = strtol(pEnd + 1, &pEnd, 16);
		bssid[4] = strtol(pEnd + 1, &pEnd, 16);
		bssid[5] = strtol(pEnd + 1, &pEnd, 16);
	}
}
std::string tools::MacToStr(const uint8_t mac[6], uint8_t count, bool format)
{
	{
		std::string bssidStr = "";
		for (uint8_t i = 6 - count; i < 6; i++)
		{
			bssidStr += tools::stringf("%02X", mac[i]);
			if (format && i < 5)
				bssidStr += ":";
		}
		return bssidStr;
	}
}

char* tools::dtostrf(double number, signed char width, unsigned char prec, char* s)
{
	bool negative = false;

	if (isnan(number))
	{
		strcpy(s, "nan");
		return s;
	}
	if (isinf(number))
	{
		strcpy(s, "inf");
		return s;
	}

	char* out = s;

	int fillme = width; // how many cells to fill for the integer part
	if (prec > 0)
	{
		fillme -= (prec + 1);
	}

	// Handle negative numbers
	if (number < 0.0)
	{
		negative = true;
		fillme--;
		number = -number;
	}

	// Round correctly so that print(1.999, 2) prints as "2.00"
	// I optimized out most of the divisions
	double rounding = 2.0;
	for (uint8_t i = 0; i < prec; ++i)
		rounding *= 10.0;
	rounding = 1.0 / rounding;

	number += rounding;

	// Figure out how big our number really is
	double tenpow = 1.0;
	int digitcount = 1;
	while (number >= 10.0 * tenpow)
	{
		tenpow *= 10.0;
		digitcount++;
	}

	number /= tenpow;
	fillme -= digitcount;

	// Pad unused cells with spaces
	while (fillme-- > 0)
	{
		*out++ = ' ';
	}

	// Handle negative sign
	if (negative)
		*out++ = '-';

	// Print the digits, and if necessary, the decimal point
	digitcount += prec;
	int8_t digit = 0;
	while (digitcount-- > 0)
	{
		digit = (int8_t)number;
		if (digit > 9)
			digit = 9; // insurance
		*out++ = (char)('0' | digit);
		if ((digitcount == prec) && (prec > 0))
		{
			*out++ = '.';
		}
		number -= digit;
		number *= 10.0;
	}

	// make sure the string is terminated
	*out = 0;
	return s;
}
