#ifndef __GLOBALVAR_H
#define __GLOBALVAR_H

#include <param.h>

#ifdef CONFIG_GLOBALVAR
int globalvar_add_simple(const char *name, const char *value);

int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags);
char *globalvar_get_match(const char *match, const char *separator);
void globalvar_set_match(const char *match, const char *val);
#else
static inline int globalvar_add_simple(const char *name, const char *value)
{
	return 0;
}

static inline int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	return 0;
}

static inline char *globalvar_get_match(const char *match, const char *separator)
{
	return NULL;
}

static inline void globalvar_set_match(const char *match, const char *val) {}
#endif

#endif /* __GLOBALVAR_H */
