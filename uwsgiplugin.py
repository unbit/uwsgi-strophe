import os

NAME='strophe'
LDFLAGS=['-Llib']
LIBS=['-lstrophe', '-lexpat', '-lresolv']
CFLAGS=['-Iinclude']
GCC_LIST=['strophe']

def get_libstrophe():
    print os.getcwd()
    if not os.path.exists('libstrophe'):
        os.system('git clone https://github.com/strophe/libstrophe.git')
        # fix CFLAGS
        am = open('libstrophe/Makefile.am', 'a')
        am.write('CFLAGS += -fPIC\n')
        am.close()
    if not os.path.exists('lib/libstrophe.a') or not os.path.exists('include/strophe.h'):
        # for some misterious reason we need to call bootstrap.sh 2 times ...
        os.system('cd libstrophe && libtoolize && ./bootstrap.sh ; ./bootstrap.sh && ./configure --prefix=%s --disable-shared && make && make install' % os.getcwd())

get_libstrophe()

