CONFIG_BL_IOT_FW_STA := y
CONFIG_BL_IOT_FW_AP := n

CONFIG_LWIP_IPV6 := y
CONFIG_LWIP_IPV6_AUTOCONFIG := y
CONFIG_LWIP_IPV6_MLD := y

CONFIG_LWIP_DHCP := n
CONFIG_LWIP_AUTOIP := n


PROJECT_DISABLE_COMPONENTS := blemesh https netutils aws-iot suas_drivers lwip_dhcpd sntp libcoap dns_server
INCLUDE_COMPONENTS := freertos vfs yloop lwip wifi bl602 bl602_std newlibc cli blog
