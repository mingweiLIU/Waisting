#ifndef WTTRANSCODEOPTIONS_H
#define WTTRANSCODEOPTIONS_H
#include "WTTranscoderDefines.h"

#include <string>
#include <functional>
#include <unordered_map>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

//定义参数
struct TranscodeOption
{
	std::string name;//参数名字
	std::string description;//参数说明
	std::function<void(const std::string&)>deserializer;//不同参数值的处理方式
};

//参数对象
class TranscodeOptions
{
public:
	TranscodeOptions() = default;
	virtual ~TranscodeOptions() = default;
	void Parse(const std::unordered_map<std::string, std::string>& inputOptions);
	std::string GenerateHelp()const;

protected:
	void AddOption(TranscodeOption&& option);
private:
	std::unordered_map<std::string, TranscodeOption>m_options;
};


template<typename TTranscodeOptions>
TTranscodeOptions ParseOptions(const std::unordered_map<std::string, std::string>& optionValues) {
	TTranscodeOptions options;
	options.Parse(optionValues);
	return options;
}

template<typename TTranscodeOptions>
std::string GetHelp() {
	TTranscodeOptions options;
	return options.GenerateHelp();
}

WTNAMESPACEEND
TRANSCODERNAMESPACEEND
#endif // TRANSCODEOPTIONS_H
