#include "IOAdapter.h"
#include "MemoryPool.h"
WT::IOFileInfo::~IOFileInfo()
{
	//删除数据 这里的char* 使用malloc分配的
	if (memoryPoolInstanceName == "")
	{
		free(data);
		data = nullptr;
	}
	else {
		MemoryPool::GetInstance(memoryPoolInstanceName)->deallocate(data, dataSize);
	}
}
