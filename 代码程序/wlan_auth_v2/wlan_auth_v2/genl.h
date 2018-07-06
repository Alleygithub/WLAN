#ifndef GENL_H
#define GENL_H

#include <asm/errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/genetlink.h>

#ifdef __cplusplus
extern "C" {
#endif

int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group);

#ifdef __cplusplus
}
#endif

#endif // GENL_H
