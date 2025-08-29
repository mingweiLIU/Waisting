#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "IOAdapter.h"

namespace WT {
	constexpr int QUANTIZED_COORDINATE_SIZE = 32767;

	class QuantifiedMeshData {
	private:
		IOFileInfo* tileInfo = nullptr;
	public:
		QuantifiedMeshData(IOFileInfo* tileInfo);
		bool writeFile(std::string& basePath);
	private:
		void pixelCoord2XYZ(std::vector<glm::dvec3>& pos);
	private:
		struct QuantizedMeshHeader
		{
		private:
			// The center of the tile in Earth-centered Fixed coordinates.
			//double CenterX;
			//double CenterY;
			//double CenterZ;
			glm::dvec3 center;

			// The minimum and maximum heights in the area covered by this tile.
			// The minimum may be lower and the maximum may be higher than
			// the height of any vertex in this tile in the case that the min/max vertex
			// was removed during mesh simplification, but these are the appropriate
			// values to use for analysis or visualization.
			//float MinimumHeight;
			//float MaximumHeight;
			glm::vec2 minMaxHeight;

			// The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
			// in Earth-centered Fixed coordinates, and the radius is in meters.
			//double BoundingSphereCenterX;
			//double BoundingSphereCenterY;
			//double BoundingSphereCenterZ;
			//double BoundingSphereRadius;
			glm::dvec4 boundingSphere;

			// The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
			// If this point is below the horizon, the entire tile is below the horizon.
			// See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
			//double HorizonOcclusionPointX;
			//double HorizonOcclusionPointY;
			//double HorizonOcclusionPointZ;
			glm::dvec3 horizonOcclusionPoint;

		public:
			QuantizedMeshHeader(const std::vector<glm::dvec3>& points);

			bool writeToFile(FILE** fp);
			glm::dvec3 maxPoint, minPoint;
		private:
			/**
			 * @brief 计算一个向量的幅度。
			 *
			 * @param position 笛卡尔空间中的位置。
			 * @param scaledSpaceDirectionToPoint 指向点的缩放空间方向。
			 * @return 计算出的幅度值。
			 */
			double computeMagnitude(const glm::dvec3& position, const glm::dvec3& scaledSpaceDirectionToPoint);

			//计算地形瓦片数据所需要的水平遮挡点
			glm::dvec3 computeHorizonOcclusionPoint(const std::vector<glm::dvec3>& points, const BoundingSphere& bs);

		};
		struct VertexData
		{
		public:
			VertexData(std::vector<glm::dvec3> positions, glm::dvec3 minPos, glm::dvec3 maxPos);
			bool writeToFile(FILE** fp);
		private:
			unsigned int vertexCount;
			std::vector<unsigned short> u;
			std::vector<unsigned short>v;// v[vertexCount];
			std::vector<unsigned short> height;// [vertexCount] ;
			//这里的value为代编码的值 minValue为最低值 delta为max-min值
			inline uint16_t zigZagEncode(int16_t i)
			{
				return ((i >> 15) ^ (i << 1));
			}
		};
		struct IndexData
		{
		public:
			IndexData(std::vector<unsigned int> indices, FILE** fp);
		};
		struct EdgeIndices
		{
		public:
			EdgeIndices(std::vector<unsigned int> westIndices
				, std::vector<unsigned int> southIndices
				, std::vector<unsigned int> eastIndices
				, std::vector<unsigned int> northIndices
				, FILE** fp);
		private:
			void writeEdgeIndex(std::vector<unsigned int> indices, FILE** fp);
		};		
	};
};