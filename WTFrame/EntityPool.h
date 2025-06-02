#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <cassert>
#include <type_traits>
namespace WT {

	// 前向声明
	template<typename T> class EntityPool;

	// 哈希组合函数
	template<typename T>
	inline void hash_combine(std::size_t& seed, const T& v) {
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<typename T>
	class pool_ptr {
	public:
		using element_type = T;
		using pointer = T*;
		using reference = T&;

		// 默认构造函数
		pool_ptr() noexcept = default;

		// 内部构造函数
		pool_ptr(EntityPool<T>* pool, size_t index) noexcept
			: m_pool(pool), m_index(index) {}

		// 拷贝构造和赋值
		pool_ptr(const pool_ptr& other) noexcept = default;
		pool_ptr& operator=(const pool_ptr& other) noexcept = default;

		// 移动构造和赋值
		pool_ptr(pool_ptr&& other) noexcept
			: m_pool(other.m_pool), m_index(other.m_index) {
			other.clear();
		}

		pool_ptr& operator=(pool_ptr&& other) noexcept {
			if (this != &other) {
				m_pool = other.m_pool;
				m_index = other.m_index;
				other.clear();
			}
			return *this;
		}

		// 析构函数
		~pool_ptr() = default;

		// 解引用操作符
		reference operator*() const {
			assert(is_valid() && "Dereferencing invalid pool_ptr");
			return *get();
		}

		pointer operator->() const {
			assert(is_valid() && "Accessing invalid pool_ptr");
			return get();
		}

		pointer get() const noexcept {
			return is_valid() ? m_pool->get_addr(m_index) : nullptr;
		}

		// 访问器
		EntityPool<T>* pool() const noexcept { return m_pool; }
		size_t index() const noexcept { return m_index; }

		// 状态管理
		void clear() noexcept {
			m_pool = nullptr;
			m_index = invalid_index;
		}

		void reset() noexcept {
			clear();
		}

		// 回收对象到池中
		void recycle() {
			if (m_pool && is_valid()) {
				m_pool->recycle(*this);
				clear();
			}
		}

		// 状态检查
		bool is_valid() const noexcept {
			return m_pool && m_pool->contains(*this);
		}

		explicit operator bool() const noexcept {
			return is_valid();
		}

		// 比较操作符
		bool operator==(const pool_ptr& other) const noexcept {
			return m_pool == other.m_pool && m_index == other.m_index;
		}

		bool operator!=(const pool_ptr& other) const noexcept {
			return !(*this == other);
		}

		bool operator<(const pool_ptr& other) const noexcept {
			if (m_pool < other.m_pool) return true;
			if (m_pool > other.m_pool) return false;
			return m_index < other.m_index;
		}

		// 交换
		void swap(pool_ptr& other) noexcept {
			std::swap(m_pool, other.m_pool);
			std::swap(m_index, other.m_index);
		}

	private:
		static constexpr size_t invalid_index = ~static_cast<size_t>(0);

		friend class EntityPool<T>;
		template<typename U> friend struct std::hash;

		EntityPool<T>* m_pool = nullptr;
		size_t m_index = invalid_index;
	};

	template<typename T>
	class EntityPool : public std::enable_shared_from_this<EntityPool<T>> {
	private:
		// 对象状态
		enum class ObjectState : uint8_t {
			Active,     // 活跃状态，正在使用
			Recycled,   // 已回收，可重用
			Destroyed   // 已销毁
		};

		// 对象槽位
		struct Slot {
			alignas(T) std::byte storage[sizeof(T)];
			ObjectState state = ObjectState::Destroyed;
			std::atomic<size_t> ref_count{0};

			T* get_object() noexcept {
				return reinterpret_cast<T*>(storage);
			}

			const T* get_object() const noexcept {
				return reinterpret_cast<const T*>(storage);
			}
		};

		// 禁用拷贝和移动
		EntityPool(const EntityPool&) = delete;
		EntityPool(EntityPool&&) = delete;
		EntityPool& operator=(const EntityPool&) = delete;
		EntityPool& operator=(EntityPool&&) = delete;

		struct private_tag {};

	public:
		using value_type = T;
		using pointer = pool_ptr<T>;

		// 创建对象池
		static std::shared_ptr<EntityPool> create() {
			return std::make_shared<EntityPool>(private_tag{});
		}

		// 构造函数（仅供make_shared使用）
		explicit EntityPool(const private_tag&) = default;

		// 析构函数
		~EntityPool() {
			clear();
		}

		// 预留容量
		void reserve(size_t capacity) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_slots.reserve(capacity);
		}

		// 获取当前大小
		size_t size() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_slots.size();
		}

		// 获取活跃对象数量
		size_t active_count() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			size_t count = 0;
			for (const auto& slot : m_slots) {
				if (slot.state == ObjectState::Active) {
					++count;
				}
			}
			return count;
		}

		// 获取可回收对象数量
		size_t recycled_count() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_recycled_indices.size();
		}

		// 创建新对象
		template<typename... Args>
		pool_ptr<T> spawn(Args&&... args) {
			std::lock_guard<std::mutex> lock(m_mutex);

			size_t index;
			Slot* slot;

			// 优先使用回收的槽位
			if (!m_recycled_indices.empty()) {
				index = m_recycled_indices.back();
				m_recycled_indices.pop_back();
				slot = &m_slots[index];

				// 如果对象是非平凡类型，需要重新构造
				if constexpr (!std::is_trivially_constructible_v<T, Args...>) {
					if (slot->state == ObjectState::Recycled) {
						// 析构旧对象
						slot->get_object()->~T();
					}
				}
			}
			else {
				// 创建新槽位
				index = m_slots.size();
				m_slots.emplace_back();
				slot = &m_slots[index];
			}

			// 构造新对象
			new (slot->storage) T(std::forward<Args>(args)...);
			slot->state = ObjectState::Active;
			slot->ref_count.store(1);

			return pool_ptr<T>(this, index);
		}

		// 回收对象
		void recycle(const pool_ptr<T>& ptr) {
			if (!ptr.is_valid() || ptr.m_pool != this) {
				return;
			}

			std::lock_guard<std::mutex> lock(m_mutex);

			size_t index = ptr.m_index;
			if (index >= m_slots.size()) {
				return;
			}

			Slot& slot = m_slots[index];
			if (slot.state != ObjectState::Active) {
				return;
			}

			// 减少引用计数
			size_t old_count = slot.ref_count.fetch_sub(1);
			if (old_count == 1) {
				// 最后一个引用，可以回收
				slot.state = ObjectState::Recycled;

				// 对于非平凡析构的类型，调用析构函数
				if constexpr (!std::is_trivially_destructible_v<T>) {
					// 注意：这里不立即析构，而是在重用时析构
					// 这样可以避免频繁的构造/析构
				}

				m_recycled_indices.push_back(index);
			}
		}

		// 检查指针是否属于此池
		bool contains(const pool_ptr<T>& ptr) const noexcept {
			if (ptr.m_pool != this) {
				return false;
			}

			std::lock_guard<std::mutex> lock(m_mutex);
			return ptr.m_index < m_slots.size() &&
				m_slots[ptr.m_index].state != ObjectState::Destroyed;
		}

		// 清空池
		void clear() {
			std::lock_guard<std::mutex> lock(m_mutex);

			// 析构所有活跃对象
			for (auto& slot : m_slots) {
				if (slot.state == ObjectState::Active || slot.state == ObjectState::Recycled) {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						slot.get_object()->~T();
					}
					slot.state = ObjectState::Destroyed;
				}
			}

			m_slots.clear();
			m_recycled_indices.clear();
		}

		// 压缩池（移除未使用的槽位）
		void shrink_to_fit() {
			std::lock_guard<std::mutex> lock(m_mutex);

			// 析构所有回收的对象
			for (size_t index : m_recycled_indices) {
				if (index < m_slots.size()) {
					Slot& slot = m_slots[index];
					if (slot.state == ObjectState::Recycled) {
						if constexpr (!std::is_trivially_destructible_v<T>) {
							slot.get_object()->~T();
						}
						slot.state = ObjectState::Destroyed;
					}
				}
			}
			m_recycled_indices.clear();

			// 移除末尾的已销毁槽位
			while (!m_slots.empty() && m_slots.back().state == ObjectState::Destroyed) {
				m_slots.pop_back();
			}

			m_slots.shrink_to_fit();
		}

	private:
		friend class pool_ptr<T>;

		T* get_addr(size_t index) const {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (index >= m_slots.size() || m_slots[index].state == ObjectState::Destroyed) {
				return nullptr;
			}
			return m_slots[index].get_object();
		}

		mutable std::mutex m_mutex;
		std::vector<Slot> m_slots;
		std::vector<size_t> m_recycled_indices;
	};

	// 全局交换函数
	template<typename T>
	void swap(pool_ptr<T>& a, pool_ptr<T>& b) noexcept {
		a.swap(b);
	}

	// 创建辅助函数
	template<typename T, typename... Args>
	pool_ptr<T> make_pooled(std::shared_ptr<EntityPool<T>>& pool, Args&&... args) {
		if (!pool) {
			pool = EntityPool<T>::create();
		}
		return pool->spawn(std::forward<Args>(args)...);
	}

} // namespace tntn

// std::hash 特化
namespace std {
	template<typename T>
	struct hash<tntn::pool_ptr<T>> {
		std::size_t operator()(const tntn::pool_ptr<T>& ptr) const noexcept {
			std::size_t seed = 0;
			tntn::hash_combine(seed, reinterpret_cast<std::uintptr_t>(ptr.pool()));
			tntn::hash_combine(seed, ptr.index());
			return seed;
		}
	};
};