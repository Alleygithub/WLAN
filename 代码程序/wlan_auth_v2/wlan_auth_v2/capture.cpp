/*
 * 捕获无线网管理帧，从管理帧中提取公开参数和无线接入点的系统时间。
*/

#include <iostream>
#include <stddef.h>
#include <errno.h>
#include <net/if.h>
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/genl/genl.h>
#include <libnl3/netlink/genl/family.h>
#include <libnl3/netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include "genl.h"

#include "capture.h"

using namespace std;

// Handler

static int error_handler(sockaddr_nl* nla, struct nlmsgerr* err, void* arg)
{
    int* ret = static_cast<int*>(arg);
    *ret = err->error;
    return NL_STOP;
}

static int finish_handler(nl_msg* msg, void* arg)
{
    int* ret = static_cast<int*>(arg);
    *ret = 0;
    return NL_SKIP;
}

static int ack_handler(nl_msg* msg, void* arg)
{
    int* ret = static_cast<int*>(arg);
    *ret = 0;
    return NL_STOP;
}

static nl_recvmsg_msg_cb_t registered_handler;
static void* registered_handler_data;

static void register_handler(nl_recvmsg_msg_cb_t handler, void *arg)
{
    registered_handler = handler;
    registered_handler_data = arg;
}

static int valid_handler(nl_msg* msg, void* arg)
{
    if (registered_handler)
        return registered_handler(msg, registered_handler_data);

    return NL_OK;
}

static int no_seq_check(struct nl_msg* msg, void* arg)
{
    return NL_OK;
}

// Handle Command

class nl80211_state
{
private:
    nl_sock* sock;
    int id;

public:
    int get_id() const { return id; }
    nl_sock* get_sock() const { return sock; }

    int init();
    void cleanup();
};

int nl80211_state::init()
{
    int err;

    sock = nl_socket_alloc(); // 生成netlink的socket
    if (!sock) {
        cerr << "Failed to allocate netlink socket." << endl;
        return -ENOMEM;
    }

    if (genl_connect(sock)) { // socket和内核连接
        cerr << "Failed to connect to generic netlink." << endl;
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    nl_socket_set_buffer_size(sock, 8192, 8192); // 调整缓存大小

    id = genl_ctrl_resolve(sock, "nl80211"); // 向内核查询一下协议族的标志
    if (id < 0) {
        cerr << "nl80211 not found." << endl;
        err = -ENOENT;
        goto out_handle_destroy;
    }

    return 0;

 out_handle_destroy:
    nl_socket_free(sock);
    return err;
}

void nl80211_state::cleanup()
{
    nl_socket_free(sock);
    return;
}

static int handle_nl80211_command(const nl80211_state& state, nl80211_commands cmd, int nl_msg_flags, const char* ifname)
{
    signed long long devidx = if_nametoindex(ifname); // 指定网络接口名称字符串作为参数；若该接口存在，则返回相应的索引，否则返回0
    if (!devidx)
        return -errno;

    int err = 0;

    nl_msg* msg = nlmsg_alloc(); // 生成要发送往内核的帧（还没有填充内容）
    if (!msg) {
        cerr << "failed to allocate netlink message" << endl;
        return 2;
    }

    nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT); // 生成回调函数
    nl_cb* s_cb = nl_cb_alloc(NL_CB_DEFAULT); // 生成回调函数
    if (!cb || !s_cb) {
        cerr << "failed to allocate netlink callbacks" << endl;
        err = 2;
        goto out;
    }

    genlmsg_put(msg, 0, 0, state.get_id(), 0, nl_msg_flags, cmd, 0); // 往刚生成的帧中填充头部信息

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devidx); // 向刚生成的帧内部添加一个属性值

    // 执行自定义处理过程

    nl_socket_set_cb(state.get_sock(), s_cb); // 设置回调函数

    err = nl_send_auto_complete(state.get_sock(), msg); // 发送刚生成的帧给内核。自此，内核当收到该请求时就会执行在帧中填充的命令索引和参数。比如搜索无线网，帧中就会填充scan命令对应的索引和要扫描的信道作为参数。
    if (err < 0)
        goto out;

    err = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

    while (err > 0)
        nl_recvmsgs(state.get_sock(), cb); // 等待接收内核的反馈

out:
    nl_cb_put(cb);
    nl_cb_put(s_cb);
    nlmsg_free(msg);

    return err;

nla_put_failure:
    cerr << "building message failed" << endl;
    return 2;
}

// Listen Events

static int prepare_listen_events(const nl80211_state& state)
{
    int mcid, ret;

    /* Configuration multicast group */
    mcid = nl_get_multicast_id(state.get_sock(), "nl80211", "config");
    if (mcid < 0)
        return mcid;

    ret = nl_socket_add_membership(state.get_sock(), mcid);
    if (ret)
        return ret;

    /* Scan multicast group */
    mcid = nl_get_multicast_id(state.get_sock(), "nl80211", "scan");
    if (mcid >= 0) {
        ret = nl_socket_add_membership(state.get_sock(), mcid);
        if (ret)
            return ret;
    }

    /* Regulatory multicast group */
    mcid = nl_get_multicast_id(state.get_sock(), "nl80211", "regulatory");
    if (mcid >= 0) {
        ret = nl_socket_add_membership(state.get_sock(), mcid);
        if (ret)
            return ret;
    }

    /* MLME multicast group */
    mcid = nl_get_multicast_id(state.get_sock(), "nl80211", "mlme");
    if (mcid >= 0) {
        ret = nl_socket_add_membership(state.get_sock(), mcid);
        if (ret)
            return ret;
    }

    mcid = nl_get_multicast_id(state.get_sock(), "nl80211", "vendor");
    if (mcid >= 0) {
        ret = nl_socket_add_membership(state.get_sock(), mcid);
        if (ret)
            return ret;
    }

    return 0;
}

struct wait_event_argument {
public:
    int cmds_count;
    const __u32* cmds;
    __u32 cmd;

    wait_event_argument(int cmds_count, const __u32* cmds) : cmds_count(cmds_count), cmds(cmds), cmd(0) {}
};

static int wait_event(nl_msg* msg, void* arg)
{
    wait_event_argument* argument = static_cast<wait_event_argument*>(arg);
    genlmsghdr* gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));

    for (int i = 0; i < argument->cmds_count; i++)
        if (static_cast<__u32>(gnlh->cmd) == argument->cmds[i])
            argument->cmd = gnlh->cmd;

    return NL_SKIP;
}

static __u32 do_listen_events(const nl80211_state& state, const int cmds_count, const __u32 cmds[])
{
    nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT); // 生成回调函数
    if (!cb) {
        cerr << "failed to allocate netlink callbacks" << endl;
        return -ENOMEM;
    }

    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

    wait_event_argument argument(cmds_count, cmds);
    register_handler(wait_event, &argument);

    while (!argument.cmd)
        nl_recvmsgs(state.get_sock(), cb);

    nl_cb_put(cb);

    return argument.cmd;
}

static __u32 listen_events(const nl80211_state& state, const int cmds_count, const __u32* cmds)
{
    int ret = prepare_listen_events(state);
    if (ret)
        return ret;

    return do_listen_events(state, cmds_count, cmds);
}

// Capture Public Parameters and System Time from Management Frame

struct parameter_from_management_frame
{
public:
    const char* specific_ssid;
    int& capture_result;
    byte* physical_parameter;
    time_t& access_point_current_time;

    parameter_from_management_frame(const char* ssid, int& res, byte* param, time_t& time) : specific_ssid(ssid), capture_result(res), physical_parameter(param), access_point_current_time(time) { }
};

static int capture_physical_parameter(struct nl_msg* msg, void* arg)
{
    parameter_from_management_frame* argument = static_cast<parameter_from_management_frame*>(arg);

    nlattr* tb[NL80211_ATTR_MAX + 1];
    genlmsghdr* gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[NL80211_ATTR_BSS]) {
        cerr << "bss info missing!" << endl;
        return NL_SKIP;
    }

    nlattr* bss[NL80211_BSS_MAX + 1];
    static nla_policy bss_policy[NL80211_BSS_MAX + 1];
    bss_policy[NL80211_BSS_INFORMATION_ELEMENTS].type = NLA_UNSPEC;
    bss_policy[NL80211_BSS_BEACON_IES].type = NLA_UNSPEC;
    if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
        cerr << "failed to parse nested attributes!" << endl;
        return NL_SKIP;
    }

    if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
        byte* o_ie = static_cast<byte*>(nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
        int o_ielen = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
        bool is_specific_ssid = false;

        byte* ie = o_ie;
        int ielen = o_ielen;
        while (ielen >= 2 && ielen >= ie[1]) {
            if (ie[0] == 0) { // 搜索SSID
                is_specific_ssid = ie[1] == strlen(argument->specific_ssid) && memcmp(ie + 2, argument->specific_ssid, ie[1]) == 0; // 比较SSID是否为指定SSID
                break;
            }

            ielen -= ie[1] + 2;
            ie += ie[1] + 2;
        }

        if (argument->capture_result != 3 && is_specific_ssid) { // 如果SSID是指定SSID
            ie = o_ie;
            ielen = o_ielen;
            while (ielen >= 2 && ielen >= ie[1]) {
                if (ie[0] == 221) { // 搜索供应商自定义参数
                    byte* data = ie + 2;
                    int len = ielen;
                    if (len < 3)
                        continue;
                    if (data[0] == 0x11 && data[1] == 0x22 && data[2] == 0x33 && len == 13) { // 无线接入点当前时间。
                        argument->capture_result |= 1;
                        memcpy(&argument->access_point_current_time, data + 3, 8);
                    }
                    else if (data[0] == 0x33 && data[1] == 0x22 && data[2] == 0x11 && len == 50) { // 物理认证参数。
                        argument->capture_result |= 2;
                        memcpy(argument->physical_parameter, data + 3, 32);
                    }
                }

                ielen -= ie[1] + 2;
                ie += ie[1] + 2;
            }
        }
    }

    return NL_SKIP;
}

int capture_physical_parameter_and_system_time_from_80211_management_frame(const char* ifname, const char* specific_ssid, int& capture_res, byte physical_param[], time_t& ap_cur_time)
{
    int err = 0;

    parameter_from_management_frame argument(specific_ssid, capture_res, physical_param, ap_cur_time);

    nl80211_state state;
    err = state.init();
    if (err != 0)
        return err;

    err = handle_nl80211_command(state, NL80211_CMD_TRIGGER_SCAN, 0, ifname);
    if (err != 0)
        goto out;

    static const __u32 cmds[] = { static_cast<__u32>(NL80211_CMD_NEW_SCAN_RESULTS), static_cast<__u32>(NL80211_CMD_SCAN_ABORTED) };
    if (listen_events(state, sizeof(cmds)/sizeof(cmds[0]), cmds) == static_cast<__u32>(NL80211_CMD_SCAN_ABORTED)) {
        cerr << "scan aborted!" << endl;
        return 2;
    }

    register_handler(capture_physical_parameter, &argument);
    err = handle_nl80211_command(state, NL80211_CMD_GET_SCAN, NLM_F_DUMP, ifname);
    if (err != 0)
        goto out;

out:
    state.cleanup();
    return err;
}
