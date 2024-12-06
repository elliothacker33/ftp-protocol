Mac addrese 

route add -net 172.16.81.0/24 gw 172.16.80.254

tux4
arp -d 172.16.81.1
arp -d 172.16.80.1
tux3
arp -d 172.16.80.254
tux2
arp -d 172.16.81.253


