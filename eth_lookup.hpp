#pragma once

#include <stdint.h>
#include <linux/if_ether.h>
#include <stdint.h>

/// Takes Ethernet protocol ID & returns human readable name
/// - Convert to host first
/// - Ex: `eth_p_human(ntohs(proto))`
const char *eth_p_human(uint16_t proto);
