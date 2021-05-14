#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <net/arp.h>


#define DEVICE_NAME "sniffer"

static char* link = "enp0s5";
static char* ifname = "vni%d";
module_param(link, charp, 0);

static struct net_device_stats stats;
static struct net_device *child = NULL;
struct priv {
    struct net_device *parent;
};

static char *incoming_mode = "INCOMING";
static char *transmiting_mode = "TRANSMITING";

#define IPV4_STR_MAX_SIZE (sizeof("xxx.xxx.xxx.xxx"))
static char saddr_msg[IPV4_STR_MAX_SIZE];
static char daddr_msg[IPV4_STR_MAX_SIZE];

struct ipv4_address {
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
};

// speedtest.net.		7196	IN	A	151.101.2.219
// speedtest.net.		7196	IN	A	151.101.194.219
// speedtest.net.		7196	IN	A	151.101.130.219
// speedtest.net.		7196	IN	A	151.101.66.219

#define TARGET_COUNT 4
static struct ipv4_address target_addrs[TARGET_COUNT] = {
    { 151, 101, 2, 219 },
    { 151, 101, 194, 219 },
    { 151, 101, 130, 219 },
    { 151, 101, 66, 219 }
};

#define IPV4_EQUALS(__addr1, __addr2) (                     \
    (__addr1.a == __addr2.a) && (__addr1.b == __addr2.b) && \
    (__addr1.c == __addr2.c) && (__addr1.d == __addr2.d)    \
)

#define PARSE_IPV4_ADDR(__in6_addr) (struct ipv4_address) {    \
        255 & (ntohl(__in6_addr) >> 24), \
        255 & (ntohl(__in6_addr) >> 16), \
        255 & (ntohl(__in6_addr) >> 8),  \
        255 & (ntohl(__in6_addr))        \
    };

#define IPV4_STR_FORM(__addr) __addr.a, __addr.b, __addr.c, __addr.d

#define PROC_STR_LEN 60
static struct proc_dir_entry* net_proc_entry;

static void ipv4_frame_process(struct sk_buff *skb, char *mode_name) {
    struct iphdr *ip = (struct iphdr *)skb_network_header(skb);

    struct ipv4_address src = PARSE_IPV4_ADDR(ip->saddr);
    struct ipv4_address dst = PARSE_IPV4_ADDR(ip->daddr);

    int i, result = 0;
    for (i = 0; i < TARGET_COUNT; i++) {
        int is_equal = 
            ((mode_name == transmiting_mode) && (IPV4_EQUALS(dst, target_addrs[i]))) || 
            ((mode_name == incoming_mode)    && (IPV4_EQUALS(src, target_addrs[i])));
        
        result = result || is_equal;
        if (is_equal) break;
    }

    if (result) {
        snprintf(saddr_msg, IPV4_STR_MAX_SIZE, "%d.%d.%d.%d", IPV4_STR_FORM(src));
        snprintf(daddr_msg, IPV4_STR_MAX_SIZE, "%d.%d.%d.%d", IPV4_STR_FORM(dst));

        printk(
            KERN_INFO "%s: Captured %s IPv4 frame, saddr:%*s,\tdaddr:%*s\n", THIS_MODULE->name, 
            mode_name, (int)IPV4_STR_MAX_SIZE, saddr_msg, (int)IPV4_STR_MAX_SIZE, daddr_msg
        );
    }
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
   if (child) {
        ipv4_frame_process(*pskb, incoming_mode);
        stats.rx_packets++;
        stats.rx_bytes += (*pskb)->len;
        (*pskb)->dev = child;
        return RX_HANDLER_ANOTHER;
    }
    return RX_HANDLER_PASS; 
} 

static int open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO "%s: device opened", dev->name);
    return 0;
}

static int stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: device closed", dev->name);
    return 0; 
}

static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
    struct priv *priv = netdev_priv(dev);

    ipv4_frame_process(skb, transmiting_mode);
    stats.tx_packets++;
    stats.tx_bytes += skb->len;

    if (priv->parent) {
        skb->dev = priv->parent;
        skb->priority = 1;
        dev_queue_xmit(skb);
        return 0;
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
    return &stats;
} 

static ssize_t proc_read(struct file* file, char __user *ubuf, size_t count, loff_t* ppos) {
    // One line for RX, second for TX + ADDRESSES LINE + GAP LINE
    size_t lines_count = (2 + 1 + TARGET_COUNT);
    size_t bytes_count = sizeof(char) * PROC_STR_LEN * lines_count;
    char *buf = kzalloc(bytes_count, GFP_KERNEL);

    printk(KERN_NOTICE "%s: Invoked proc_read\n", THIS_MODULE->name);

    int i;
    for (i = 0; i < TARGET_COUNT; i++) {
        snprintf(
            buf+(i*PROC_STR_LEN), 
            PROC_STR_LEN,
            "%d TARGET IP ADDRESS: %d.%d.%d.%d\n", 
            i + 1, IPV4_STR_FORM(target_addrs[i])
        );
    }

    snprintf(buf + (TARGET_COUNT * PROC_STR_LEN), PROC_STR_LEN, "--- STATS ---\n");
    snprintf(
            buf + ((TARGET_COUNT + 1) * PROC_STR_LEN), PROC_STR_LEN, 
            "RX = %ld packets (%ld bytes)\n", stats.rx_packets, stats.rx_bytes
        );
    snprintf(
            buf + ((TARGET_COUNT + 2) * PROC_STR_LEN), PROC_STR_LEN, 
            "TX = %ld packets (%ld bytes)\n", stats.tx_packets, stats.tx_bytes
        );

    if (*ppos > 0 || count < bytes_count){
        kfree(buf);
        return 0;
    }

    if (copy_to_user(ubuf, buf, bytes_count)){
        kfree(buf);
        return -EFAULT;
    }

    *ppos += bytes_count;
    kfree(buf);
    return bytes_count;
}

static struct net_device_ops vni_net_device_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_get_stats = get_stats,
    .ndo_start_xmit = start_xmit
};

static struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read  = proc_read
};

static void setup(struct net_device *dev) {
    int i;
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &vni_net_device_ops;

    //fill in the MAC address with a phoney
    for (i = 0; i < ETH_ALEN; i++)
        dev->dev_addr[i] = (char)i;
} 

int __init vni_init(void) {
    int err = 0;
    struct priv *priv;
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
    if (child == NULL) {
        printk(KERN_ERR "%s: allocate error", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        printk(KERN_ERR "%s: no such net: %s", THIS_MODULE->name, link);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        printk(KERN_ERR "%s: illegal net type", THIS_MODULE->name); 
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
    if ((err = dev_alloc_name(child, child->name))) {
        printk(KERN_ERR "%s: allocate name, error %i", THIS_MODULE->name, err);
        free_netdev(child);
        return -EIO;
    }

    register_netdev(child);
    rtnl_lock();
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
    printk(KERN_INFO "%s: Module loaded", THIS_MODULE->name);
    printk(KERN_INFO "%s: create link %s", THIS_MODULE->name, child->name);
    printk(KERN_INFO "%s: registered rx handler for %s", THIS_MODULE->name, priv->parent->name);

    net_proc_entry = proc_create(DEVICE_NAME, 0444, NULL, &proc_fops);
    if (!IS_ERR(net_proc_entry)) {
        printk(KERN_INFO "%s: /proc/%s is created\n", THIS_MODULE->name, DEVICE_NAME);
    }

    return 0; 
}

void __exit vni_exit(void) {
    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        printk(KERN_INFO "%s: unregister rx handler for %s", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);

    proc_remove(net_proc_entry);
    printk(KERN_INFO "%s: Proc /proc/%s destroyed\n", THIS_MODULE->name, DEVICE_NAME);

    printk(KERN_INFO "%s: Module unloaded", THIS_MODULE->name); 
} 

module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Sergey Fedorov");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Kernel Module to sniff incomming and outcomming IPv4 traffic by provided address");
