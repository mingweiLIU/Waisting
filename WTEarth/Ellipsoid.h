#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <optional>
#include <vector>
#include <string>

namespace WT{
	class Cartographic;
	class Rectangle;
	class Ellipsoid {
	public:
		explicit Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0);
		explicit Ellipsoid(glm::dvec3 radiis);

		//计算从该椭球体中心指向给定笛卡尔坐标位置的单位向量
		std::optional<glm::dvec3> geocentricSurfaceNormal(const glm::dvec3& pos);

		//计算在给定位置处与椭球体表面相切的平面的法向量
		std::optional<glm::dvec3> geodeticSurfaceNormalCartographic(const Cartographic& posCartographic);

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
		glm::dvec3 cartographicToCartesian(const Cartographic& cartographic);

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
		std::vector<glm::dvec3> cartographicArrayToCartesianArray(const std::vector<Cartographic>& cartographics);

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
		std::optional<Cartographic> cartesianToCartographic(const glm::dvec3& cartesian);
		

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
		std::optional<glm::dvec3> scaleToGeodeticSurface(const glm::dvec3& cartesian);

		/**
		 * 将笛卡尔坐标数组转换为大地坐标数组
		 *
		 * @param cartesians 笛卡尔坐标数组（输入）
		 * @param result 可选参数，用于存储结果的预分配数组（将被重置大小）
		 * @return 返回转换后的大地坐标数组（若未提供result参数则返回新数组）
		 *
		 * @throws std::invalid_argument 当输入数组为空时抛出异常
		 *
		 * @example
		 * // 创建笛卡尔坐标数组并转换为WGS84椭球体下的大地坐标
		 * std::vector<glm::dvec3> positions = {
		 *     glm::dvec3(17832.12, 83234.52, 952313.73),
		 *     glm::dvec3(17832.13, 83234.53, 952313.73),
		 *     glm::dvec3(17832.14, 83234.54, 952313.73)
		 * };
		 * auto cartographics = Ellipsoid::WGS84().cartesiansToCartographics(positions);
		 */
		std::vector<Cartographic> Ellipsoid::cartesiansToCartographics(const std::vector<glm::dvec3 >& cartesians);

		
		/**
			* @brief 将笛卡尔坐标沿地心表面法线缩放至椭球体表面
			*
			* 通过计算缩放系数 beta，将输入坐标点投影到椭球体表面。
			*
			* @param cartesian 输入坐标点（必须有效）
			* @return glm::dvec3 
			*
			* @warning 输入坐标点不应为零向量，否则会导致除以零错误
			*/
		glm::dvec3 scaleToGeocentricSurface(const glm::dvec3& cartesian) const;

		/**
		 * @brief 将笛卡尔坐标转换到椭球体缩放空间
		 *
		 * 通过将各坐标分量乘以对应轴向半径的倒数，实现坐标空间变换。
		 *
		 * @param position 要转换的坐标位置
		 * @param result 结果 glm::dvec3 对象
		 *
		 */
		glm::dvec3 transformPositionToScaledSpace(const glm::dvec3& position) const;

		/**
		 * @brief 将坐标从椭球体缩放空间转换回标准笛卡尔空间
		 *
		 * 通过将各坐标分量乘以对应轴向半径，实现逆变换。
		 *
		 * @param position 要转换的坐标位置
		 * @param result 结果 glm::dvec3 对象
		 *
		 */
		glm::dvec3 transformPositionFromScaledSpace(const glm::dvec3& position) const;

		//判断相等
		bool equals(Ellipsoid& right);

		//转出string
		std::string toString();

		/**
		 * @brief 计算表面法线与Z轴的交点
		 *
		 * 计算椭球体表面某点处法线与Z轴的交点坐标。
		 *
		 * @param position 表面点的位置坐标（必须在椭球体表面上）
		 * @param buffer 缓冲距离（默认为0），用于调整椭球体有效边界
		 * @return std::optional<glm::dvec3> 交点坐标（如果存在），否则返回std::nullopt
		 *
		 * @throws std::invalid_argument 如果输入位置无效
		 * @throws std::logic_error 如果椭球体不是旋转椭球体（radii.x != radii.y）
		 * @throws std::logic_error 如果radii.z <= 0
		 *
		 * @note 对于WGS84椭球体，交点z坐标绝对值最大为±42841.31151331382（约占z轴的0.673%）
		 * @note 当长轴/旋转轴比率大于√2时，交点可能在椭球体外
		 */
		std::optional<glm::dvec3> getSurfaceNormalIntersectionWithZAxis(const glm::dvec3& position, double buffer = 0.0) const;

		/**
		 * @brief 计算椭球体表面某点的局部曲率（东向和北向）
		 *
		 * @param surfacePosition 椭球体表面点的位置坐标（必须在表面上）
		 * @return glm::dvec2 局部曲率，x分量表示东向曲率，y分量表示北向曲率
		 *
		 * @throws std::invalid_argument 如果输入位置无效
		 *
		 * @note 东向曲率 = 1 / 卯酉圈曲率半径
		 * @note 北向曲率 = 1 / 子午圈曲率半径
		 * @note 计算公式：
		 *       卯酉圈半径 = 表面点到法线与Z轴交点的距离
		 *       子午圈半径 = (b² * 卯酉圈半径³) / a⁴
		 */
		glm::dvec2 getLocalCurvature(const glm::dvec3& surfacePosition) const;

		/**
		 * 计算椭球体表面矩形区域的近似面积
		 * 使用10阶高斯-勒让德求积法进行数值积分
		 *
		 * @param rectangle 定义表面矩形区域的地理边界
		 * @return double 计算得到的表面积（平方米）
		 * @throws std::invalid_argument 如果矩形边界无效
		 */
		double surfaceArea(const Rectangle& rectangle) const;


		// Getter 函数（const 成员函数，保证不修改对象状态）
		glm::dvec3& getRadii()  { return mRadii; }
		glm::dvec3& getRadiiSquared()  { return mRadiiSquared; }
		glm::dvec3& getRadiiToFourth()  { return mRadiiToFourth; }
		glm::dvec3& getOneOverRadii()  { return mOneOverRadii; }
		glm::dvec3& getOneOverRadiiSquared()  { return mOneOverRadiiSquared; }
		double getCenterTolernaceSquard()const {return mCenterTolernaceSquard; }
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
		// 椭球体三个半径（a, b, c）
		glm::dvec3 mRadii;
		// 半径平方（a², b², c²）
		glm::dvec3 mRadiiSquared;
		// 半径四次方（a⁴, b⁴, c⁴）
		glm::dvec3 mRadiiToFourth;
		// 半径平方的倒数（1/a, 1/b, 1/c）
		glm::dvec3 mOneOverRadii;
		// 半径平方的倒数（1/a², 1/b², 1/c²）
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
};