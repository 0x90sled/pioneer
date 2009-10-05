#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <vector>
#include <stdio.h>

#ifdef _WIN32
# define PATH_SEP "\\"
#else
# define PATH_SEP "/"
#endif /* !_WIN32 */

// joinpath("data","models","some.def") = "data/models/some.def"
std::string join_path(const char *firstbit, ...);
std::string string_join(std::vector<std::string> &v, std::string sep);
std::string string_subst(const char *format, const unsigned int num_args, std::string args[]);
std::string format_date(double time);
std::string format_date_only(double time);
std::string format_distance(double dist);
std::string format_money(int money);
void strip_cr_lf(char *string);

GLuint util_load_tex_rgba(const char *filename);

FILE *fopen_or_die(const char *filename, const char *mode);

#ifndef __GNUC__
#define __attribute(x)
#endif /* __GNUC__ */
static inline std::string stringf(int maxlen, const char *format, ...)
		__attribute((format(printf,2,3)));

static inline std::string stringf(int maxlen, const char *format, ...)
{
	char *buf = (char*)alloca(maxlen);
	va_list argptr;
	va_start(argptr, format);
	vsnprintf(buf, maxlen, format, argptr);
	va_end(argptr);
	return std::string(buf);
}

struct Plane {
	double a, b, c, d;
	double DistanceToPoint(const vector3d &p) {
		return a*p.x + b*p.y + c*p.z + d;
	}
};

/* from current GL modelview*projection matrix */
void GetFrustum(Plane planes[6]);
std::string make_random_ship_registration();

#endif /* _UTILS_H */
