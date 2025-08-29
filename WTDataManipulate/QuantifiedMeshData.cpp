#include "QuantifiedMeshData.h"
#include <filesystem>

#include <glm/gtx/norm.hpp>

#include "DelaunayTriangle.h"
#include "Rectangle.h"
#include "Cartographic.h"
#include"Ellipsoid.h"
#include "AABBox.h"
#include "BoundingSphere.h"

namespace WT {
	QuantifiedMeshData::QuantifiedMeshData(IOFileInfo* tileInfo):tileInfo(tileInfo)
	{
	}

	bool QuantifiedMeshData::writeFile(std::string& basePath)
	{
		//将IOFileInfo转为Terrain的数据结构 注意这里只转换 不输出
		int tileSize = sqrt(tileInfo->dataSize);
		TerraMesh terraMesh(tileSize, tileSize, tileInfo);
		terraMesh.greedyInsert(1.9);
		std::array<std::vector<glm::dvec3>, 4> boundaryPs = terraMesh.getBoundaryPoints();
		std::vector<unsigned int> indices; std::vector<glm::dvec3> pos;
		terraMesh.getMeshData(indices, pos);

		//创建输出文件
		const auto fullPath = std::filesystem::path(basePath) / tileInfo->filePath;
		std::string outputFilename = fullPath.string() + ".terrain";

		// 打开输出文件
		FILE* fp = fopen(outputFilename.c_str(), "wb");
		if (!fp) {
			std::cerr << "无法创建输出文件: " << outputFilename << std::endl;
			return false;
		}

		//创建header
		QuantizedMeshHeader header(pos);
		header.writeToFile(&fp);
		//创建顶点
		VertexData vertexData(pos, header.minPoint, header.maxPoint);
		vertexData.writeToFile(&fp);
		//创建索引
		IndexData indexData(indices, &fp);
		//创建边缘点索引

	}

	void QuantifiedMeshData::pixelCoord2XYZ(std::vector<glm::dvec3>& pos)
	{
		Rectangle* tileRect = static_cast<Rectangle*>(tileInfo->userData);
		Cartographic northWest = tileRect->getNorthwest();
		float lonWidth = tileRect->computeWidth();//经度跨度
		float latHeight = tileRect->computeHeight();//维度跨度

		int tilePixelCount = sqrt(tileInfo->dataSize);
		float lonPerPixel = lonWidth / tilePixelCount;
		float latPerPixel = latHeight / tilePixelCount;

		//先计算像素坐标回到经纬度
		//再计算经纬度高度转xyz
		Ellipsoid ellipsoid = Ellipsoid::WGS84;
		for (glm::dvec3& onePos:pos)
		{
			double xDu = northWest.longitude + lonPerPixel * onePos.x;
			double yDu= northWest.latitude - latPerPixel * onePos.y;
			Cartographic position(glm::radians(xDu), glm::radians(yDu), onePos.z);
			onePos = ellipsoid.cartographicToCartesian(position);		

			//下面是直接使用投影来做的 感觉不太对
			//onePos.x = northWest.longitude + lonPerPixel * onePos.x;
			//onePos.y = northWest.latitude - latPerPixel * onePos.y;
			//onePos=projector.project(Cartographic(onePos.x, onePos.y, onePos.z));
		}
	}

	QuantifiedMeshData::QuantizedMeshHeader::QuantizedMeshHeader(const std::vector<glm::dvec3>& pos)
	{
		//根据矩形来重采样平面经纬度位置 然后再转换为地心坐标
		AABBox aabb(pos);
		maxPoint = aabb.maximum;
		minPoint = aabb.minimum;
		//计算中心球
		BoundingSphere bs(aabb);
		glm::dvec3 horizonOccularPoint = computeHorizonOcclusionPoint(pos, bs);

		this->center = aabb.center;
		this->boundingSphere = glm::dvec4(bs.center, bs.radius);
		this->horizonOcclusionPoint = horizonOccularPoint;
		this->minMaxHeight = glm::dvec2(aabb.minimum.z, aabb.maximum.z);
	}

	bool QuantifiedMeshData::QuantizedMeshHeader::writeToFile(FILE** fp)
	{
		size_t doubleSize = sizeof(double);
		size_t floatSize = sizeof(float);

		//写入中心点
		fwrite(&center, sizeof(double), 3, *fp);
		//写入minmax
		fwrite(&minMaxHeight, sizeof(float), 2, *fp);
		//写入boundingbox
		fwrite(&boundingSphere, sizeof(double), 4, *fp);
		//写入水平线裁切点
		fwrite(&horizonOcclusionPoint, sizeof(double), 3, *fp);
	}

	double QuantifiedMeshData::QuantizedMeshHeader::computeMagnitude(const glm::dvec3& position, const glm::dvec3& scaledSpaceDirectionToPoint)
	{
		// 将位置转换到缩放空间
		glm::dvec3 scaledSpacePosition = Ellipsoid::WGS84.transformPositionToScaledSpace(position);

		// 计算缩放空间位置的长度平方
		double magnitudeSquared = glm::length2(scaledSpacePosition);
		// 计算长度（幅度）
		double magnitude = std::sqrt(magnitudeSquared);
		// 计算方向向量
		glm::dvec3 direction = scaledSpacePosition / magnitude;

		// 为了这个计算，椭球体下面的点被认为在其表面上。
		// 这通过将长度平方和长度的最小值限制为 1.0 来实现。
		magnitudeSquared = std::max(1.0, magnitudeSquared);
		magnitude = std::max(1.0, magnitude);

		// 计算余弦和正弦值
		double cosAlpha = glm::dot(direction, scaledSpaceDirectionToPoint);
		double sinAlpha = glm::length(glm::cross(direction, scaledSpaceDirectionToPoint));
		double cosBeta = 1.0 / magnitude;
		double sinBeta = std::sqrt(magnitudeSquared - 1.0) * cosBeta;

		// 返回最终的幅度
		return 1.0 / (cosAlpha * cosBeta - sinAlpha * sinBeta);
	}

	glm::dvec3 QuantifiedMeshData::QuantizedMeshHeader::computeHorizonOcclusionPoint(const std::vector<glm::dvec3>& points, const BoundingSphere& bs)
	{
		Ellipsoid wgs84 = Ellipsoid::WGS84;
		glm::dvec3 oneOverEllipsoidRadius = wgs84.getOneOverRadii();
		const double MIN = -std::numeric_limits<double>::infinity();
		double max_magnitude = MIN;
		glm::dvec3 scaledCenter = bs.center * oneOverEllipsoidRadius;// (center.x * wgs84., center.y * llh_ecef_rY, center.z * llh_ecef_rZ);

		for (int i = 0, icount = points.size(); i < icount; i++) {
			glm::dvec3 onePoint = points[i];
			glm::dvec3 scaledPoint = onePoint * oneOverEllipsoidRadius;

			double magnitude = computeMagnitude(scaledPoint, scaledCenter);
			if (magnitude > max_magnitude) max_magnitude = magnitude;
		}
		return scaledCenter * max_magnitude;
	}

	QuantifiedMeshData::VertexData::VertexData(std::vector<glm::dvec3> positions, glm::dvec3 minPos, glm::dvec3 maxPos)
	{
		double ratioX = QUANTIZED_COORDINATE_SIZE / (maxPos.x - minPos.x);
		double ratioY = QUANTIZED_COORDINATE_SIZE / (maxPos.y - minPos.y);
		double ratioZ = QUANTIZED_COORDINATE_SIZE / (maxPos.z - minPos.z);

		int preU = 0, preV = 0, preH = 0;
		for (glm::dvec3& onePos : positions)
		{
			int scaledX = static_cast<int> ((onePos.x - minPos.x) * ratioX);
			int scaledY = static_cast<int> ((onePos.y - minPos.y) * ratioY);
			int scaledZ = static_cast<int> ((onePos.z - minPos.z) * ratioZ);

			u.push_back(zigZagEncode(scaledX - preU));
			v.push_back(zigZagEncode(scaledY - preV));
			height.push_back(zigZagEncode(scaledZ - preH));

			preU = scaledX; preV = scaledY; preH = scaledZ;
		}
	}

	bool QuantifiedMeshData::VertexData::writeToFile(FILE** fp)
	{
		size_t doubleSize = sizeof(double);
		size_t floatSize = sizeof(float);

		//写入顶点数
		fwrite(&vertexCount, sizeof(unsigned int), 1, *fp);
		//写入u
		fwrite(u.data(), sizeof(unsigned short), vertexCount, *fp);
		//写入v
		fwrite(v.data(), sizeof(unsigned short), vertexCount, *fp);
		//写入height
		fwrite(height.data(), sizeof(unsigned short), vertexCount, *fp);
	}

	QuantifiedMeshData::EdgeIndices::EdgeIndices(std::vector<unsigned int> westIndices, std::vector<unsigned int> southIndices, std::vector<unsigned int> eastIndices, std::vector<unsigned int> northIndices, FILE** fp)
	{
		writeEdgeIndex(westIndices, fp);
		writeEdgeIndex(southIndices, fp);
		writeEdgeIndex(eastIndices, fp);
		writeEdgeIndex(northIndices, fp);
	}

	void QuantifiedMeshData::EdgeIndices::writeEdgeIndex(std::vector<unsigned int> indices, FILE** fp)
	{
		unsigned int triangleCount = indices.size();
		fwrite(&triangleCount, sizeof(unsigned int), 1, *fp);
		if (triangleCount < 65536)
		{
			std::vector<unsigned short> temp(indices.begin(), indices.end());
			fwrite(temp.data(), sizeof(unsigned short), temp.size(), *fp);
		}
		else
		{
			fwrite(indices.data(), sizeof(unsigned short), indices.size(), *fp);
		}
	}

	QuantifiedMeshData::IndexData::IndexData(std::vector<unsigned int> indices, FILE** fp)
	{
		unsigned int triangleCount = indices.size() / 3;
		fwrite(&triangleCount, sizeof(unsigned int), 1, *fp);
		if (indices.size() < 65536)
		{
			std::vector<unsigned short> temp(indices.begin(), indices.end());

			unsigned short waterMark = 0;
			for (unsigned short i = 0, iUP = indices.size(); i < iUP; ++i)
			{
				unsigned short index = indices[i];
				unsigned short delta = waterMark - index;
				temp[i] = delta;
				if (index == waterMark)
				{
					waterMark++;
				}
			}

			fwrite(temp.data(), sizeof(unsigned short), temp.size(), *fp);
		}
		else
		{
			unsigned int waterMark = 0;
			for (unsigned int i = 0, iUP = indices.size(); i < iUP; ++i)
			{
				unsigned int index = indices[i];
				unsigned int delta = waterMark - index;
				indices[i] = delta;
				if (index == waterMark)
				{
					waterMark++;
				}
			}
			fwrite(indices.data(), sizeof(unsigned short), indices.size(), *fp);
		}
	}
};