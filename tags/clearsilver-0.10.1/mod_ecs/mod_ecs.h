#ifndef __MOD_ECS_H_
#define __MOD_ECS_H_ 1

/* not a real .h file - just for the cgiplusplus interface */

/* not an enum - want a bitfield */
#define APACHE_CGI 0x1000
/* need nonzero even for a default :-) */
#define CGIPLUSPLUS 1
#define EXTENDED_CGI 2
#define USE_STRICT 4
#define OPTIMISED_DESCRIPTORS 8
#define ENV_AS_TABLE 0x10
#define USE_CGI_STDERR 0x20
#define STDERR_TO_HTML 0x40
#define NPH 0x100
#define STDIN_BUF 0x200
#define OWN_STDOUT 0x400
#define OWN_STDERR 0x800
/* one day we'll get round to implementing all the above */

#define ECS_MAGIC_TYPE "application/x-ecs-cgi"

#endif /* __MOD_ECS_H_ */
