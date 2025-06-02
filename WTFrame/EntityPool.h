#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <cassert>
#include <type_traits>
namespace WT {

	// ǰ������
	template<typename T> class EntityPool;

	// ��ϣ��Ϻ���
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

		// Ĭ�Ϲ��캯��
		pool_ptr() noexcept = default;

		// �ڲ����캯��
		pool_ptr(EntityPool<T>* pool, size_t index) noexcept
			: m_pool(pool), m_index(index) {}

		// ��������͸�ֵ
		pool_ptr(const pool_ptr& other) noexcept = default;
		pool_ptr& operator=(const pool_ptr& other) noexcept = default;

		// �ƶ�����͸�ֵ
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

		// ��������
		~pool_ptr() = default;

		// �����ò�����
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

		// ������
		EntityPool<T>* pool() const noexcept { return m_pool; }
		size_t index() const noexcept { return m_index; }

		// ״̬����
		void clear() noexcept {
			m_pool = nullptr;
			m_index = invalid_index;
		}

		void reset() noexcept {
			clear();
		}

		// ���ն��󵽳���
		void recycle() {
			if (m_pool && is_valid()) {
				m_pool->recycle(*this);
				clear();
			}
		}

		// ״̬���
		bool is_valid() const noexcept {
			return m_pool && m_pool->contains(*this);
		}

		explicit operator bool() const noexcept {
			return is_valid();
		}

		// �Ƚϲ�����
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

		// ����
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
		// ����״̬
		enum class ObjectState : uint8_t {
			Active,     // ��Ծ״̬������ʹ��
			Recycled,   // �ѻ��գ�������
			Destroyed   // ������
		};

		// �����λ
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

		// ���ÿ������ƶ�
		EntityPool(const EntityPool&) = delete;
		EntityPool(EntityPool&&) = delete;
		EntityPool& operator=(const EntityPool&) = delete;
		EntityPool& operator=(EntityPool&&) = delete;

		struct private_tag {};

	public:
		using value_type = T;
		using pointer = pool_ptr<T>;

		// ���������
		static std::shared_ptr<EntityPool> create() {
			return std::make_shared<EntityPool>(private_tag{});
		}

		// ���캯��������make_sharedʹ�ã�
		explicit EntityPool(const private_tag&) = default;

		// ��������
		~EntityPool() {
			clear();
		}

		// Ԥ������
		void reserve(size_t capacity) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_slots.reserve(capacity);
		}

		// ��ȡ��ǰ��С
		size_t size() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_slots.size();
		}

		// ��ȡ��Ծ��������
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

		// ��ȡ�ɻ��ն�������
		size_t recycled_count() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_recycled_indices.size();
		}

		// �����¶���
		template<typename... Args>
		pool_ptr<T> spawn(Args&&... args) {
			std::lock_guard<std::mutex> lock(m_mutex);

			size_t index;
			Slot* slot;

			// ����ʹ�û��յĲ�λ
			if (!m_recycled_indices.empty()) {
				index = m_recycled_indices.back();
				m_recycled_indices.pop_back();
				slot = &m_slots[index];

				// ��������Ƿ�ƽ�����ͣ���Ҫ���¹���
				if constexpr (!std::is_trivially_constructible_v<T, Args...>) {
					if (slot->state == ObjectState::Recycled) {
						// �����ɶ���
						slot->get_object()->~T();
					}
				}
			}
			else {
				// �����²�λ
				index = m_slots.size();
				m_slots.emplace_back();
				slot = &m_slots[index];
			}

			// �����¶���
			new (slot->storage) T(std::forward<Args>(args)...);
			slot->state = ObjectState::Active;
			slot->ref_count.store(1);

			return pool_ptr<T>(this, index);
		}

		// ���ն���
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

			// �������ü���
			size_t old_count = slot.ref_count.fetch_sub(1);
			if (old_count == 1) {
				// ���һ�����ã����Ի���
				slot.state = ObjectState::Recycled;

				// ���ڷ�ƽ�����������ͣ�������������
				if constexpr (!std::is_trivially_destructible_v<T>) {
					// ע�⣺���ﲻ��������������������ʱ����
					// �������Ա���Ƶ���Ĺ���/����
				}

				m_recycled_indices.push_back(index);
			}
		}

		// ���ָ���Ƿ����ڴ˳�
		bool contains(const pool_ptr<T>& ptr) const noexcept {
			if (ptr.m_pool != this) {
				return false;
			}

			std::lock_guard<std::mutex> lock(m_mutex);
			return ptr.m_index < m_slots.size() &&
				m_slots[ptr.m_index].state != ObjectState::Destroyed;
		}

		// ��ճ�
		void clear() {
			std::lock_guard<std::mutex> lock(m_mutex);

			// �������л�Ծ����
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

		// ѹ���أ��Ƴ�δʹ�õĲ�λ��
		void shrink_to_fit() {
			std::lock_guard<std::mutex> lock(m_mutex);

			// �������л��յĶ���
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

			// �Ƴ�ĩβ�������ٲ�λ
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

	// ȫ�ֽ�������
	template<typename T>
	void swap(pool_ptr<T>& a, pool_ptr<T>& b) noexcept {
		a.swap(b);
	}

	// ������������
	template<typename T, typename... Args>
	pool_ptr<T> make_pooled(std::shared_ptr<EntityPool<T>>& pool, Args&&... args) {
		if (!pool) {
			pool = EntityPool<T>::create();
		}
		return pool->spawn(std::forward<Args>(args)...);
	}

} // namespace tntn

// std::hash �ػ�
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