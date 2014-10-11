import os

NAME='strophe'
LDFLAGS=['-Llib']
LIBS=['-lstrophe', '-lexpat', '-lresolv']
CFLAGS=['-Iinclude']
GCC_LIST=['strophe']

def get_libstrophe():
    if not os.path.exists('libstrophe'):
        os.system('git clone https://github.com/strophe/libstrophe.git')
        # fix CFLAGS
        am = open('libstrophe/Makefile.am', 'a')
        am.write('CFLAGS += -fPIC\n')
        am.close()
    if not os.path.exists('lib/libstrophe.a') or not os.path.exists('include/strophe.h'):
        os.system('cd libstrophe || ./bootstrap.sh || ./configure --prefix=%s || make || make install' % os.getcwd())

get_libstrophe()

