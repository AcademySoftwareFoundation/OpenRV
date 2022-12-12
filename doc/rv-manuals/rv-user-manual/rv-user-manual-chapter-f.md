# F - Troubleshooting Networking

### Configure RV’s network settings

You can use the Network dialog under the RV Menu to configure RV’s network syncing (as in between two or more instances of RV), set the name you show up as in sync mode on other RV’s, and establish the port you want to listen on for network connections. From this dialog you can initiate a connection to another and manage your Contacts list.

The Contacts tab gives a list of users you have established connections with previously, but what is more important is the permission drop-down menu immediately below the contacts list. This drop-down allows you to set the default behavior for how to manage incoming connections. Once a user is added to the contacts list, the permission can be set per contact. Lastly, the Connections tab shows your active connections.

At the bottom are two important buttons: “Connect…” and “Start Network.” “Connect…” lets you type in another RV’s network name and port. RV can only connect by hostname or IP address. It is important that the RV reaching out to connect can see the remote RV through any firewall. You may have to setup port forwarding or some other DMZ configuration for this to work. Please contact your network administrator for these types of advanced setups.

Once you are satisfied with your settings, you can enable networking for RV by pressing the “Start Network” button.

### Connections only work from one direction or are always refused

Some operating systems have a firewall on by default that may be blocking the port RV is trying to use. When you start RV on the machine with the firewall and start networking it appears to be functioning correctly, but no one can connect to it. Check to see if the port that RV wants to use is available through the firewall.

This is almost certainly the case when the connection works from one direction but not the other. The side which can make the connection is usually the one that has the firewall blocking RV (it won't let other machines in).