#include "IOAdapter.h"
#include "MemoryPool.h"
WT::IOFileInfo::~IOFileInfo()
{
	//ɾ������ �����char* ʹ��malloc�����
	if (memoryPoolInstanceName == "")
	{
		free(data);
		data = nullptr;
	}
	else {
		MemoryPool::GetInstance(memoryPoolInstanceName)->deallocate(data, dataSize);
	}
}
