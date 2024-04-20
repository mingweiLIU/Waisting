#ifndef WTASSECTCONVERTOR_H
#define WTASSECTCONVERTOR_H
#include "WTTranscoderDefines.h"
#include <assimp/scene.h>
#include "wtGLTFTranscodeOptions.h"
#include "WTFrame/wtCancelation.h"

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

class WTAPI wtAssectConvertor
{
public:
    wtAssectConvertor(aiScene* assets,const wtGLTFTranscodeOptions& options={}, WT::Frame::IWTCancellationTokenPtr cancellationToken=nullptr);
    
    static bool ConvertFilesInFolder(std::string folderPath, std::string formatID, std::string outputFolder);

    bool Write();
private:
    aiScene* mAssets;
    wtGLTFTranscodeOptions mOptions;
    WT::Frame::IWTCancellationTokenPtr mCancellationToken;
};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // WTASSECTCONVERTOR_H
