uwsgi-strophe
=============

uWSGI plugin for libstrophe integration (xmpp)

```sh
git clone https://github.com/unbit/uwsgi-strophe
cd uwsgi-strophe
uwsgi --build-plugin .
```

Currently only alarms features are exposed:

```ini
[uwsgi]
plugin = strophe
; syntax: strophe:<jid> <password> <dest>
alarm = help strophe:uwsgi@jabber.server secret me@jabber.server
http-socket = :9090
; trigger the 'help' alarm at every request
route-run = alarm:help HELP ME !!!
```
