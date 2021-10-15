#ifndef GULACHEK_DYNAMIC_VARIANT_HPP
#define GULACHEK_DYNAMIC_VARIANT_HPP

#include <type_traits>
#include <variant>
#include <functional>
#include <any>
#include <gulachek/gtree.hpp>
#include <gulachek/gtree/encoding/variant.hpp>

namespace gulachek
{
	// Helpers

	template <typename ...Ts>
	struct __overload_rev {};

	template <>
	struct __overload_rev<>
	{
		static void init() {};
	};

	template <typename Head, typename ...Tail>
	struct __overload_rev<Head, Tail...> :
		__overload_rev<Tail...>
	{
		using __overload_rev<Tail...>::init;

		static void init(
				Head h,
				std::size_t *reverse_i,
				std::any *val
				)
		{
			*val = std::make_any<Head>(std::move(h));
			*reverse_i = 1 + sizeof...(Tail);
		}
	};

	template <typename ...Ts>
	struct __overload
	{
		template <typename T>
		static void init(
				T &&t,
				std::size_t *i,
				std::any *val
				)
		{
			__overload_rev<Ts...>::init(
						std::forward<T>(t),
						i,
						val
						);

			*i = sizeof...(Ts) - *i;
		}
	};

	template <typename ...Ts>
	class dynamic_variant
	{
		template <typename T>
		using self_t = std::enable_if_t<
			std::is_base_of_v<dynamic_variant, std::decay_t<T>>,
			int
				>;

		template <typename T>
		using other_t = std::enable_if_t<
			!std::is_base_of_v<dynamic_variant, std::decay_t<T>>,
			int
				>;

		public:
			using types = gulachek::gtree::meta_cons<Ts...>;

			dynamic_variant() : _i{0}, _val{} {}

			template <
				typename T,
				other_t<T> = 0
					>
			dynamic_variant(T &&t)
			{
				__overload<Ts...>::init(
						t,
						&_i,
						&_val
						);
			}

			template <
				typename T,
				other_t<T> = 1
				>
			dynamic_variant& operator=(T &&t)
			{
				__overload<Ts...>::init(
						t,
						&_i,
						&_val
						);

				return *this;
			}

			template <
				typename T,
				self_t<T> = 2
					>
			dynamic_variant& operator=(T &&t)
			{
				_i = t._i;
				_val = t._val;

				return *this;
			}

			template <
				typename T,
				other_t<T> = 3
					>
			dynamic_variant& operator=(const T &t)
			{
				__overload<Ts...>::init(
						t,
						&_i,
						&_val
						);

				return *this;
			}

			template <
				typename T,
				self_t<T> = 4
					>
			dynamic_variant& operator=(const T &t)
			{
				_i = t._i;
				_val = t._val;

				return *this;
			}

			std::size_t index() const
			{
				return _i;
			}

			bool valueless_by_exception() const
			{
				return !_val.has_value();
			}

			template <typename U>
			std::add_pointer_t<const U> match() const
			{
				if (_i == gtree::index_of<types, U>())
					return std::any_cast<const U>(&_val);

				return nullptr;
			}

			template <typename U>
			std::add_pointer_t<U> match()
			{
				if (_i == gtree::index_of<types, U>())
					return std::any_cast<U>(&_val);

				return nullptr;
			}

			template <typename U>
			U& require()
			{
				auto pmatch = match<U>();
				if (!pmatch) throw std::bad_variant_access{};
				return *pmatch;
			}

			template <typename U>
			const U& require() const
			{
				auto pmatch = match<U>();
				if (!pmatch) throw std::bad_variant_access{};
				return *pmatch;
			}

		private:
			std::size_t _i;
			std::any _val;
	};
}

namespace std
{
	template <typename T, typename ...Ts>
	std::add_pointer_t<const T> get_if(
			const gulachek::dynamic_variant<Ts...> *pv
			)
	{
		return pv->template match<T>();
	}

	template <typename T, typename ...Ts>
	std::add_pointer_t<T> get_if(
			gulachek::dynamic_variant<Ts...> *pv
			)
	{
		return pv->template match<T>();
	}

	template <typename T, typename ...Ts>
	T&& get(
			gulachek::dynamic_variant<Ts...> &&v
			)
	{
		return std::move(v.template require<T>());
	}

	template <typename T, typename ...Ts>
	const T& get(
			const gulachek::dynamic_variant<Ts...> &v
			)
	{
		return v.template require<T>();
	}
}

namespace gulachek
{
	template <typename Visitor, typename ...Ts>
	struct __visit_deduce_return
	{
	};

	template <typename Visitor, typename Tail>
	struct __visit_deduce_return<Visitor, Tail>
	{
		using type = std::invoke_result_t<Visitor, Tail>;
	};

	template <typename Visitor, typename Head, typename ...Tail>
	struct __visit_deduce_return<Visitor, Head, Tail...>
	{
		using type = std::invoke_result_t<Visitor, Head>;
		static_assert(std::is_same_v<
				type,
				typename __visit_deduce_return<Visitor, Tail...>::type
				>,
				"Visitor must return same type for each alternative"
				);
	};

	template <typename Visitor, typename ...Ts>
	typename __visit_deduce_return<Visitor, Ts...>::type
	visit(
			Visitor &&vis,
			gulachek::dynamic_variant<Ts...> &&var
			)
	{
		typedef gulachek::dynamic_variant<Ts...> variant;
		typedef typename
			__visit_deduce_return<Visitor, Ts...>::type ret;

		std::function<ret()> fs[] = {
			[&](){ return std::invoke(
					std::forward<Visitor>(vis),
					std::get<Ts>(std::forward<variant>(var))
					); }
			...
		};

		return fs[var.index()]();
	}

	template <typename Visitor, typename ...Ts>
	typename __visit_deduce_return<Visitor, Ts...>::type
	visit(
			Visitor &&vis,
			const gulachek::dynamic_variant<Ts...> &var
			)
	{
		typedef gulachek::dynamic_variant<Ts...> variant;
		typedef typename
			__visit_deduce_return<Visitor, Ts...>::type ret;

		std::function<ret()> fs[] = {
			[&](){ return std::invoke(
					std::forward<Visitor>(vis),
					std::get<Ts>(std::forward<const variant>(var))
					); }
			...
		};

		return fs[var.index()]();
	}
}

namespace gulachek::gtree
{
	template <typename ...Ts>
	struct variant_encoding<gulachek::dynamic_variant<Ts...>>
	{
		using type = gulachek::dynamic_variant<Ts...>;
		using types = gulachek::gtree::meta_cons<Ts...>;

		template <typename Var>
		static std::size_t index(Var &&var)
		{ return var.index(); }

		template <typename T, typename Var>
		static auto get(Var &&var)
		{ return std::get<T>(std::forward<Var>(var)); }
	};
}

#endif
