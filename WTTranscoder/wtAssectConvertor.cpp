#include "wtAssectConvertor.h"
#include "assimp/Exporter.hpp"

USINGTRANSCODERNAMESPACE

wtAssectConvertor::wtAssectConvertor(aiScene* assets)
{
	Assimp::Exporter exporter;
	exporter.ExportToBlob()
	assets->ex
}
