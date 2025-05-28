#pragma once
#include "Ellipsoid.h"
namespace WT {
	class Cartographic
	{
	public:
		Cartographic(double longitude = 0, double latitude = 0, double height = 0)
			:longitude(longitude), latitude(latitude), height(height) {}

		/**
		* 生成表示该地理坐标的字符串，格式为"(longitude, latitude, height)"
		* @return 格式化的坐标字符串
		*/
		std::string toString() const;

		/**
		 * @brief 从弧度值创建新的地理坐标
		 *
		 * @param longitude_rad 经度（单位：弧度，默认0.0）
		 * @param latitude_rad 纬度（单位：弧度，默认0.0）
		 * @param height_m 高度（单位：米，默认0.0）
		 *
		 * @note 此函数不进行单位转换，直接使用输入的弧度值
		 *
		 * @example
		 * // 创建北京位置的地理坐标（使用弧度值）
		 * const double lon_rad = 2.0316;  // ~116.4度转换的弧度
		 * const double lat_rad = 0.6964;  // ~39.9度转换的弧度
		 * auto beijing = fromRadians(lon_rad, lat_rad, 50.0);
		 */
		static Cartographic fromRadians(double longtitude = 0, double latitude = 0, double height = 0);

		/**
		 * @brief 从度分秒格式创建新的地理坐标(结果以弧度存储)
		 *
		 * @param longitude_deg 经度(度)
		 * @param latitude_deg 纬度(度)
		 * @param height_m 高度(米)，默认为0.0
		 * @return Cartographic 弧度制坐标
		 *
		 * @example
		 * // 创建北京位置的地理坐标
		 * auto beijing = Cartographic::fromDegrees(116.4, 39.9, 50.0);
		 * // 经度: 2.0316弧度(≈116.4°)
		 * // 纬度: 0.6964弧度(≈39.9°)
		 * // 高度: 50.0米
		 */
		static Cartographic fromDegrees(double longtitude, double latitude, double height = 0);

		/**
		 * 从笛卡尔坐标创建新的地理坐标实例。结果中的值将以弧度表示。
		 *
		 * @param cartesian 要转换为地理坐标表示的笛卡尔位置
		 * @param ellipsoid 位置所在的椭球体，默认为WGS84椭球体
		 * @param result 用于存储结果的对象
		 * @return 如果提供了result参数则返回修改后的result，否则返回新的Cartographic实例。
		 *         如果笛卡尔坐标位于椭球体中心则返回std::nullopt
		 */
		static std::optional<Cartographic> fromCartesian(const glm::dvec3& cartesian, const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * 从地理坐标(Cartographic)创建新的笛卡尔坐标(Cartesian3)实例。
		 * 输入对象中的值应为弧度制。
		 *
		 * @param cartographic 要转换为笛卡尔坐标的输入地理坐标
		 * @param ellipsoid 坐标所在的椭球体，默认为WGS84椭球体
		 * @param result 存储结果的对象(可选)
		 * @return 返回转换后的笛卡尔坐标位置
		 *
		 * @throws std::invalid_argument 如果cartographic参数无效
		 */
		static glm::dvec3 toCartesian(const Cartographic& cartographic, const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * 比较两个地理坐标的各个分量，如果完全相同则返回true，否则返回false
		 *
		 * @param left 第一个地理坐标
		 * @param right 第二个地理坐标
		 * @return 如果两个坐标完全相同返回true，否则返回false
		 */
		static bool equals(const Cartographic& left, const Cartographic& right);


		/**
		 * 比较两个地理坐标的各个分量，如果在给定的epsilon范围内则认为相等
		 *
		 * @param left 第一个地理坐标
		 * @param right 第二个地理坐标
		 * @param epsilon 比较时允许的误差范围，默认为0
		 * @return 如果两个坐标在epsilon范围内相等返回true，否则返回false
		 */
		static bool equalsEpsilon(const Cartographic& left, const Cartographic& right, double epsilon = 0.0);

	public:
		double longitude, latitude, height;
	}
};