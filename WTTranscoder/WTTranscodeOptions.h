#ifndef WTTRANSCODEOPTIONS_H
#define WTTRANSCODEOPTIONS_H
#include "WTTranscoderDefines.h"

#include <string>
#include <functional>
#include <unordered_map>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

//�������
struct TranscodeOption
{
	std::string name;//��������
	std::string description;//����˵��
	std::function<void(const std::string&)>deserializer;//��ͬ����ֵ�Ĵ���ʽ
};

//��������
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
