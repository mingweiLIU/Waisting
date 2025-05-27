#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <optional>
#include <vector>

namespace WT{
	class Ellipsoid {
	public:
		explicit Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0);
		explicit Ellipsoid(glm::dvec3 radiis);

		//计算从该椭球体中心指向给定笛卡尔坐标位置的单位向量
		std::optional<glm::dvec3> geocentricSurfaceNormal(const glm::dvec3& pos);

		//计算在给定位置处与椭球体表面相切的平面的法向量
		std::optional<glm::dvec3> geodeticSurfaceNormalCartographic(const glm::dvec3& posCartographic);

		/**
		 * @brief 计算椭球体表面在给定笛卡尔坐标处的切平面法向量
		 *
		 * @param cartesian 输入的笛卡尔坐标
		 * @param oneOverRadiiSquared 椭球体三个轴半径平方的倒数 (1/a², 1/b², 1/c²)
		 * @param result 可选的结果存储指针
		 * @return std::optional<glm::dvec3> 返回法向量，如果输入为零向量则返回std::nullopt
		 * @throws std::invalid_argument 如果输入包含NaN或无效参数
		 */
		std::optional<glm::dvec3> geodeticSurfaceNormal(const glm::dvec3& cartesian);

		/**
		 * @brief 将地理坐标转换为笛卡尔坐标
		 *
		 * @param cartographic 地理坐标 (经度, 纬度, 高度) 弧度制
		 * @return glm::dvec3 转换后的笛卡尔坐标
		 *
		 * @example
		 * // 创建地理坐标并转换为WGS84椭球上的笛卡尔坐标
		 * glm::dvec3 position(glm::radians(21.0), glm::radians(78.0), 5000.0);
		 * glm::dvec3 cartesian = cartographicToCartesian(position);
		 */
		glm::dvec3 cartographicToCartesian(const glm::dvec3& cartographic);

		/**
		 * @brief 将地理坐标数组转换为笛卡尔坐标数组
		 *
		 * @param cartographics 地理坐标数组 (经度, 纬度, 高度) 弧度制
		 * @return std::vector<glm::dvec3> 转换后的笛卡尔坐标数组
		 *
		 * @example
		 * // 转换地理坐标数组到WGS84椭球上的笛卡尔坐标
		 * std::vector<glm::dvec3> positions = {
		 *     glm::dvec3(glm::radians(21.0), glm::radians(78.0), 0.0),
		 *     glm::dvec3(glm::radians(21.321), glm::radians(78.123), 100.0),
		 *     glm::dvec3(glm::radians(21.645), glm::radians(78.456), 250.0)
		 * };
		 * std::vector<glm::dvec3> cartesians = cartographicArrayToCartesianArray(positions, radiiSquared);
		 */
		std::vector<glm::dvec3> cartographicArrayToCartesianArray(const std::vector<glm::dvec3>& cartographics);

		/**
		 * @brief 将笛卡尔坐标转换为地理坐标
		 *
		 * @param cartesian 笛卡尔坐标 (x, y, z)
		 * @return std::optional<Cartographic> 转换后的地理坐标，如果输入在椭球中心则返回std::nullopt
		 *
		 * @example
		 * // 创建笛卡尔坐标并转换为WGS84椭球上的地理坐标
		 * glm::dvec3 position(17832.12, 83234.52, 952313.73);
		 * auto cartographic = cartesianToCartographic(position);
		 */
		//std::optional<Cartographic> cartesianToCartographic(const glm::dvec3& cartesian);
		


		// Getter 函数（const 成员函数，保证不修改对象状态）
		const glm::dvec3& getRadii() const { return mRadii; }
		const glm::dvec3& getRadiiSquared() const { return mRadiiSquared; }
		const glm::dvec3& getRadiiToFourth() const { return mRadiiToFourth; }
		const glm::dvec3& getOneOverRadii() const { return mOneOverRadii; }
		const glm::dvec3& getOneOverRadiiSquared() const { return mOneOverRadiiSquared; }
		double getMinimumRadius() const { return mMinimumRadius; }
		double getMaximumRadius() const { return mMaximumRadius; }

	public:
		static Ellipsoid WGS84;
		static Ellipsoid UNIT_SPHERE;
		static Ellipsoid MOON;

	public:
		/**
		 * 将给定的笛卡尔坐标沿着椭球体法线方向缩放，使其位于椭球体表面上。
		 * 如果坐标位于椭球体中心，则返回std::nullopt。
		 *
		 * @param cartesian 要缩放的笛卡尔坐标
		 * @param oneOverRadii 椭球体半径的倒数
		 * @param oneOverRadiiSquared 椭球体半径平方的倒数
		 * @param centerToleranceSquared 接近中心的容差阈值
		 * @param result 存储结果的对象(可选)
		 * @return 如果提供了result则返回修改后的result，否则返回新的glm::dvec3。
		 *         如果坐标位于中心则返回std::nullopt
		 *
		 * @throws std::invalid_argument 如果任何必需参数未提供
		 */
		static std::optional<glm::dvec3> scaleToGeodeticSurface(
			const glm::dvec3& cartesian,
			const glm::dvec3& oneOverRadii,
			const glm::dvec3& oneOverRadiiSquared,
			double centerToleranceSquared);
		
	private:
		//三个轴向的半径
		glm::dvec3 mRadii;
		//三个轴向半径的平方
		glm::dvec3 mRadiiSquared;
		//三个轴向半径的四次方
		glm::dvec3 mRadiiToFourth;
		//三个轴向半径分之一
		glm::dvec3 mOneOverRadii;
		//三个轴向半径平方之一
		glm::dvec3 mOneOverRadiiSquared;
		//最小轴向半径
		double mMinimumRadius=0.0;
		//最大轴向半径
		double mMaximumRadius=0.0;
		//中心限差
		double mCenterTolernaceSquard = 0.1;
		//x方/z方
		double mSquaredXOverSquaredZ=0.0;
	};
}