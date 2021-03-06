#ifndef _EXCOM_TEST_HELPER_H
# define _EXCOM_TEST_HELPER_H

# ifndef UTEST_NOCOLOR
#  define TEXT_COLOR_BLACK "\e[0;30m"
#  define TEXT_COLOR_BOLD_BLACK "\e[0;1;30m"
#  define TEXT_COLOR_RED "\e[0;31m"
#  define TEXT_COLOR_BOLD_RED "\e[0;1;31m"
#  define TEXT_COLOR_GREEN "\e[0;32m"
#  define TEXT_COLOR_BOLD_GREEN "\e[0;1;32m"
#  define TEXT_COLOR_YELLOW "\e[0;33m"
#  define TEXT_COLOR_BOLD_YELLOW "\e[0;1;33m"
#  define TEXT_COLOR_BLUE "\e[0;34m"
#  define TEXT_COLOR_BOLD_BLUE "\e[0;1;34m"
#  define TEXT_COLOR_MAGENTA "\e[0;35m"
#  define TEXT_COLOR_BOLD_MAGENTA "\e[0;1;35m"
#  define TEXT_COLOR_CYAN "\e[0;36m"
#  define TEXT_COLOR_BOLD_CYAN "\e[0;1;36m"
#  define TEXT_COLOR_WHITE "\e[0;37m"
#  define TEXT_COLOR_BOLD_WHITE "\e[0;1;37m"
#  define TEXT_COLOR_RESET "\e[0m"
# else
#  define TEXT_COLOR_BLACK ""
#  define TEXT_COLOR_BOLD_BLACK ""
#  define TEXT_COLOR_RED ""
#  define TEXT_COLOR_BOLD_RED ""
#  define TEXT_COLOR_GREEN ""
#  define TEXT_COLOR_BOLD_GREEN ""
#  define TEXT_COLOR_YELLOW ""
#  define TEXT_COLOR_BOLD_YELLOW ""
#  define TEXT_COLOR_BLUE ""
#  define TEXT_COLOR_BOLD_BLUE ""
#  define TEXT_COLOR_MAGENTA ""
#  define TEXT_COLOR_BOLD_MAGENTA ""
#  define TEXT_COLOR_CYAN ""
#  define TEXT_COLOR_BOLD_CYAN ""
#  define TEXT_COLOR_WHITE ""
#  define TEXT_COLOR_BOLD_WHITE ""
#  define TEXT_COLOR_RESET ""
# endif

# define output(format) printf(format TEXT_COLOR_RESET); fflush(stdout)
# define output2(format, ...) printf(format TEXT_COLOR_RESET, __VA_ARGS__); fflush(stdout)
# define error(format) fprintf(stderr, format TEXT_COLOR_RESET); fflush(stderr)
# define error2(format, ...) fprintf(stderr, format TEXT_COLOR_RESET, __VA_ARGS__); fflush(stderr)

# define TEST_RETURN (failed > 0)

# define uassert_empty(value) \
  uassert(((intmax_t) (value)) == 0)

# define uassert_equal(v1, v2) \
  uassert(v1 == v2)

# define uassert_nequal(v1, v2) \
  uassert(v1 != v2)

# define uassert_same(str1, str2, len) \
  uassert_empty(strncmp((char*) str1, (char*) str2, len))
# define status(body)              \
    output2("\n\t\t\t"             \
      TEXT_COLOR_BOLD_YELLOW "%s"  \
      TEXT_COLOR_RESET, body)


# ifndef VERBOSE
#   define uassert(body) do                             \
    {                                                   \
      if(!(body)) {                                     \
        test_success = 0;                               \
        failed++;                                       \
        output2("\n" TEXT_COLOR_RED                     \
          "\t\t\tASSERT FAILED "                        \
          "\"" TEXT_COLOR_BOLD_RED "%s" TEXT_COLOR_RED  \
          "\"\n\t\t\t" TEXT_COLOR_RED "LINE "           \
          TEXT_COLOR_BOLD_RED "%d"  "\n\t\t\t"          \
          TEXT_COLOR_RED "FILE \"" TEXT_COLOR_BOLD_RED  \
          "%s" TEXT_COLOR_RED  "\"\n", #body, __LINE__, \
          __FILE);                                      \
      }                                                 \
  } while(0)
# else
#   define uassert(body) do                             \
    {                                                   \
      output(TEXT_COLOR_MAGENTA "\n\t\t\tASSERT \""     \
        TEXT_COLOR_BOLD_MAGENTA #body TEXT_COLOR_MAGENTA\
        "\"");                                          \
      if(!(body)) {                                     \
        test_success = 0;                               \
        failed++;                                       \
        output2("\n" TEXT_COLOR_RED                     \
          "\t\t\tASSERT FAILED "                        \
          "\"" TEXT_COLOR_BOLD_RED "%s" TEXT_COLOR_RED  \
          "\"\n\t\t\t" TEXT_COLOR_RED "LINE "           \
          TEXT_COLOR_BOLD_RED  "%d"  "\n\t\t\t"         \
          TEXT_COLOR_RED "FILE \""  TEXT_COLOR_BOLD_RED \
          "%s" TEXT_COLOR_RED  "\"", #body, __LINE__,   \
          __FILE);                                      \
      }                                                 \
    } while(0)
# endif

#endif
