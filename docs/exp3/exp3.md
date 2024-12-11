Mac addrese 

route add -net 172.16.81.0/24 gw 172.16.80.254

tux4
arp -d 172.16.81.1
arp -d 172.16.80.1
tux3
arp -d 172.16.80.254
tux2
arp -d 172.16.81.253


# A partir da experiencia 3 fazer na bancada 100

mac eth1 -> 00:c0:df:25:43:bc
mac eth2 -> 00:01:02:9f:7e:9c

route add -net 172.16.101.0/24 gw 172.16.100.254
route add -net 172.16.100.0/24 gw 172.16.101.253



