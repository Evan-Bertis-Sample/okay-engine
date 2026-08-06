#ifndef __EXPORT_HPP__
#define __EXPORT_HPP__

#if defined(_WIN32) || defined(__CYGWIN__)

#if defined(OKAY_STATIC)
#define OKAY_EXPORT
#define OKAY_IMPORT
#define OKAY_LOCAL
#else
#define OKAY_EXPORT __declspec(dllexport)
#define OKAY_IMPORT __declspec(dllimport)
#define OKAY_LOCAL
#endif

#else

#if defined(__GNUC__) && __GNUC__ >= 4
#define OKAY_EXPORT __attribute__((visibility("default")))
#define OKAY_IMPORT __attribute__((visibility("default")))
#define OKAY_LOCAL __attribute__((visibility("hidden")))
#else
#define OKAY_EXPORT
#define OKAY_IMPORT
#define OKAY_LOCAL
#endif

#endif

#endif  // __EXPORT_HPP__
