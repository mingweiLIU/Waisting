#pragma once
#include <glm/glm.hpp>

class TilingScheme {
private:
	int numberOfLevelZeroTilesX;
	int numberOfLevelZeroTilesY;
public:
	//��ȡlevelʱX�������Ƭ��
	virtual int getNumberOfXTilesAtLevel(int level) = 0;

	//��ȡlevelʱY�������Ƭ��
	virtual int getNumberOfYTilesAtLevel(int level) = 0;

	//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
	virtual void rectangleToNativeRectangle(glm::vec4 rect, glm::vec4& nativeRect) = 0;

	//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
	virtual void tileXYToNativeRectangle(int tileX, int tileY, int level,glm::vec4& nativeRect) = 0;

	//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
	virtual void tileXYToRectangle(int tileX, int tileY, int level, glm::vec4& rect) = 0;

	//�������λ�����ĸ���Ƭ��
	virtual void positionToTileXY(float radX, float radY,int level, int& tileX, int& tileY) = 0;
};