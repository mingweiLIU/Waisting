#include "WTTranscodeOptions.h"
#include "WTUlits/wtStringUtils.h"
#include "WTFrame/wtLog.h"

#include <sstream>

USINGTRANSCODERNAMESPACE
std::string TranscodeOptions::GenerateHelp() const
{
	size_t maxOptionLength = 40;//����40���ַ�
	for (const auto&option:m_options)
	{
		maxOptionLength = std::max(maxOptionLength, option.second.name.length());
	}
	std::stringstream sstr;
	for (const auto& option:m_options)
	{
		sstr << option.second.name << std::string((maxOptionLength - option.second.name.length()) + 4, ' ') << option.second.description << "\n";
	}
	return sstr.str();
}

void TranscodeOptions::Parse(const std::unordered_map<std::string, std::string>& inputOptions)
{
	//�ڲ����б��е�ÿһ�������Ӧ�����Ѷ���Ĳ����е�һ��
	for (const auto& inputOption: inputOptions)
	{
		auto option = m_options.find(WT::Ulits::wtStringUtils::ToLower(inputOption.second));

		if (option!=m_options.end())
		{
			option->second.deserializer(WT::Ulits::wtStringUtils::ToLower(inputOption.second));
		}
		else {
			wtLOG_WARN("Unrecognized Transcode option: %s", inputOption.first.c_str());
		}
	}
}

void WT::Transcoder::TranscodeOptions::AddOption(TranscodeOption&& option)
{
	auto key = WT::Ulits::wtStringUtils::ToLower(option.name);
	m_options.emplace(std::move(key), std::move(option));
}
