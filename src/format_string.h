
#include <string>
#include <sstream>

namespace wfx
{
	namespace Tools
	{
		class String;
		class SystemError;

		namespace details
		{
			struct typeformat_default_tag { };
			struct typeformat_string_tag { };
			struct typeformat_systemerror_tag { };

			template<typename T>
			struct typeformat
			{
				typedef typeformat_default_tag tag_type;
			};

			template<>
			struct typeformat<SystemError>
			{
				typedef typeformat_systemerror_tag tag_type;
			};

			template<>
			struct typeformat<String>
			{
				typedef typeformat_string_tag tag_type;
			};

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted(m_string_stream_t& stream, T&& t, typeformat_default_tag)
			{
				stream << t;
			}

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted_string(m_string_stream_t& stream, T&& t, char)
			{
				stream << t.c_str();
			}

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted_string(m_string_stream_t& stream, T&& t, wchar_t)
			{
				stream << t.c_strw();
			}

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted(m_string_stream_t& stream, T&& t, typeformat_string_tag)
			{
				string_stream_append_type_formatted_string(stream, t, m_string_stream_t::char_type());
			}

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted(m_string_stream_t& stream, T&& systemError, typeformat_systemerror_tag)
			{
				typedef typename std::basic_string< m_string_stream_t::char_type > stream_string_type;
				stream << stream_string_type(systemError);
			}

			template<
				typename m_string_stream_t,
				typename T
			>
			inline void
				string_stream_append_type_formatted(m_string_stream_t& stream, T&& t)
			{
				typedef typename typeformat
					<
					typename std::remove_cv<
					typename std::remove_reference<T>::type
					>::type
					>::tag_type tag_type;
				string_stream_append_type_formatted(stream, std::forward<T>(t), tag_type());
			};

			template<
				typename m_string_stream_t,
				auto spacing_char,
				typename T,
				typename... Ts
			>
			inline void constexpr string_stream_append(m_string_stream_t& stream, T&& t, Ts&&... ts)
			{
				typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type plain_type;

				static_assert(!std::is_pointer<plain_type>::value
											||
											std::is_same<
											typename std::remove_cv<typename std::remove_pointer<plain_type>::type>::type,
											typename m_string_stream_t::char_type
											>::value
											||
											std::is_void<typename std::remove_cv<typename std::remove_pointer<plain_type>::type>::type>::value,
											"Nonmatching pointer type in make_string or make_wstring. Please check wchar_t / char correctness. If you want to print pointer adresses please cast to void*."
											);
				static_assert(!std::is_array<plain_type>::value
											||
											std::is_same<
											typename std::remove_cv<typename std::remove_extent<plain_type>::type>::type,
											typename m_string_stream_t::char_type
											>::value,
											"Nonmatching array type in make_string or make_wstring. Please check wchar_t / char correctness."
											);

				string_stream_append_type_formatted(stream, std::forward<T>(t));

				if constexpr (sizeof...(Ts) > 0)
				{
					if constexpr (spacing_char != 0)
					{
						stream << spacing_char;
					}
					string_stream_append<m_string_stream_t, spacing_char>(stream, std::forward<Ts>(ts)...);
				}
			};

		}

		template<
			char spacing_char = 0,
			typename... Ts
		>
		inline std::string constexpr format_string(Ts&&... ts)
		{
			typedef std::stringstream m_string_stream_t;
			m_string_stream_t stream;
			details::string_stream_append<m_string_stream_t, spacing_char>(stream, std::forward<Ts>(ts)...);
			return stream.str();
		};

		template<
			wchar_t spacing_char = 0,
			typename... Ts
		>
		inline std::wstring constexpr format_wstring(Ts&&... ts)
		{
			typedef std::wstringstream m_string_stream_t;
			m_string_stream_t stream;
			details::string_stream_append<m_string_stream_t, spacing_char>(stream, std::forward<Ts>(ts)...);
			return stream.str();
		};

		template<
			typename string_type,
			typename string_type::value_type spacing_char = 0,
			typename... Ts
		>
		inline string_type constexpr format(Ts&&... ts)
		{
			typedef typename string_type::value_type char_type;

			typedef std::basic_stringstream< char_type, std::char_traits< char_type >, std::allocator< char_type > >
				m_string_stream_t;

			m_string_stream_t stream;
			details::string_stream_append<m_string_stream_t, spacing_char>(stream, std::forward<Ts>(ts)...);
			return stream.str();
		};

	}

}

template<
	wchar_t spacing_char = 0,
	typename... Ts
>
inline void debug_msg(Ts&&... ts)
{
#ifdef _DEBUG
	const auto msg = wfx::Tools::format_wstring<spacing_char>(ts..., L"\n");
	OutputDebugStringW(msg.c_str());
#endif
};

template<
	char spacing_char = 0,
	typename... Ts
>
inline void debug_msg_a(Ts&&... ts)
{
#ifdef _DEBUG
	const auto msg = wfx::Tools::format_string<spacing_char>(ts..., "\n");
	OutputDebugStringA(msg.c_str());
#endif
};


