#ifndef GULACHEK_DYNAMIC_VARIANT_HPP
#define GULACHEK_DYNAMIC_VARIANT_HPP

#include <type_traits>
#include <variant>
#include <functional>
#include <any>
#include <tuple>
#include <gulachek/gtree/encoding.hpp>
#include <gulachek/gtree/decoding.hpp>
#include <gulachek/gtree/encoding/unsigned.hpp>

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

		static constexpr std::size_t ntypes = sizeof...(Ts);
		using tup_t = std::tuple<Ts...>;

		template <typename T, std::size_t index>
			requires (index == ntypes)
		static constexpr std::size_t index_of_type_impl()
		{
			return ntypes;
		}

		template <typename T, std::size_t index>
			requires (index < ntypes)
		static constexpr std::size_t index_of_type_impl()
		{
			if constexpr (std::is_same_v<T,
					typename std::tuple_element<index, tup_t>::type>)
			{
				return index;
			}
			else
			{
				return index_of_type_impl<T, index+1>();
			}
		}

		template <typename T>
		static constexpr std::size_t index_of_type()
		{
			return index_of_type_impl<T, 0>();
		}

		public:
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
				if (_i == index_of_type<U>())
					return std::any_cast<const U>(&_val);

				return nullptr;
			}

			template <typename U>
			std::add_pointer_t<U> match()
			{
				if (_i == index_of_type<U>())
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

			error gtree_decode(gtree::treeder &r)
			{
				std::size_t actual_index;
				if (auto err = gtree::decode_unsigned(r.value(), &actual_index))
				{
					return err.wrap() << "error reading dynamic_variant alt index"; 
				}

				if (!r.child_count())
				{
					return {"expected dynamic_variant encoding to have child"};
				}

				return decode_alt<0>(actual_index, r);
			}

			error gtree_encode(gtree::tree_writer &w) const
			{
				std::uint8_t index[sizeof(std::size_t)];
				auto nbytes = gtree::encode_unsigned(index, _i);
				w.value(index, nbytes);

				w.child_count(1);
				return encode_alt<0>(w);
			}

		private:
			std::size_t _i;
			std::any _val;

			template <std::size_t index>
				requires (index == ntypes)
			error decode_alt(std::size_t actual_index, gtree::treeder &r)
			{
				error err;
				err << "dynamic_variant index " << actual_index << " is "
					"too large for " << ntypes << " alts";
				return err;
			}

			template <std::size_t index>
				requires (index < ntypes)
			error decode_alt(std::size_t actual_index, gtree::treeder &r)
			{
				if (actual_index == index)
				{
					using alt_t =
						typename std::tuple_element<index, tup_t>::type;

					alt_t val;
					if (auto err = r.read(&val))
					{
						return err.wrap() << "error reading dynamic_variant alt " << index; 
					}

					_i = index;
					_val = std::move(val);
					return {};
				}

				return decode_alt<index+1>(actual_index, r);
			}

			template <std::size_t index>
				requires (index == ntypes)
			error encode_alt(gtree::tree_writer &w) const
			{
				throw std::logic_error{"encoding bad dynamic_variant"};
			}

			template <std::size_t index>
				requires (index < ntypes)
			error encode_alt(gtree::tree_writer &w) const
			{
				if (_i == index)
				{
					using alt_t =
						typename std::tuple_element<index, tup_t>::type;

					const auto *pval = std::any_cast<alt_t>(&_val);
					if (auto err = w.write(*pval))
					{
						return err.wrap() << "failed to encode dynamic_variant alt " << _i;
					}

					return {};
				}

				return encode_alt<index+1>(w);
			}
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

#endif
