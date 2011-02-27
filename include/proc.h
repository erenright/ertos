/*
 * proc.h
 *
 * User-level task/process items.
 */

#ifndef _PROC_H
#define _PROC_H

#define STDOUT_SIZE 1024

// Various user-visible process-specific items
struct self {
	struct {
		char *ptr;
		int idx;
		int buf_enable;
		int buf_last;
	} stdout;
};

#define stdio_buf_disable() 					\
	self->stdout.buf_last = self->stdout.buf_enable;	\
	self->stdout.buf_enable = 0;

#define stdio_buf_enable()					\
	self->stdout.buf_last = self->stdout.buf_enable;	\
	self->stdout.buf_enable = 1;

#define stdio_buf_restore()					\
	self->stdout.buf_enable = self->stdout.buf_last;

#endif /* !_PROC_H */
