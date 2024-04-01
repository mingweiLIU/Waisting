#ifndef IWTOUTPUTSTREAMFACTORY_H
#define IWTOUTPUTSTREAMFACTORY_H
#include "WTTranscoderDefines.h"
#include <string>
#include <memory>
#include <GLTFSDK/IStreamWriter.h>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART
class IOutputStreamFactory {
public:
	virtual ~IOutputStreamFactory() = default;
	virtual std::shared_ptr<std::ostream> CreateStream(const std::string& streamName) = 0;
};

class IStreamWriterOSFAdapter:public Microsoft::glTF::IStreamWriter
{
public:
	IStreamWriterOSFAdapter(IOutputStreamFactory& outputStreamFactory)
		:mOutputStreamFactory(outputStreamFactory){}

	std::shared_ptr<std::ostream> GetOutputStream(const std::string& fileName) const override {
		return mOutputStreamFactory.CreateStream(fileName);
	}
private:
	IOutputStreamFactory& mOutputStreamFactory;
};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND


#endif // IWTOUTPUTSTREAMFACTORY_H
