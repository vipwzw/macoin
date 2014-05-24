export MAKENSIS_HOME=/d/litecoin/libs/NSIS
export PATH=$PATH:$MAKENSIS_HOME:/d/litecoin/libs/qt/bin:/d/litecoin/libs/protobuf/bin
export DEPEND_HOME=/d/litecoin/libs
export PROTOC_BIN_HOME=/d/litecoin/libs/protobuf/bin
export PROTOC_INC_HOME=/d/litecoin/libs/protobuf/include
export PROTOC_LIB_HOME=/d/litecoin/libs/protobuf/lib
export QT_BIN_HOME=/d/litecoin/libs/qt/bin
export QT_LIB_HOME=/d/litecoin/libs/qt/lib
export QT_INC_HOME=/d/litecoin/libs/qt/include
export QT_PLUGIN_HOME=/d/litecoin/libs/qt/plugins
export LANG_CODECS_HOME=/d/litecoin/libs/qt/plugins/codecs

./configure \
--disable-debug \
--disable-ipv6 \
--disable-tests \
--with-boost="${DEPEND_HOME}/boost" \
--with-boost-libdir="${DEPEND_HOME}/boost/stage/lib" \
--with-boost-system=mgw48-mt-s-1_54 \
--with-boost-filesystem=mgw48-mt-s-1_54 \
--with-boost-program-options=mgw48-mt-s-1_54 \
--with-boost-thread=mgw48-mt-s-1_54 \
--with-boost-chrono=mgw48-mt-s-1_54 \
--with-miniupnpc=yes \
--with-gui=auto \
--with-qt-incdir=${QT_INC_HOME} \
--with-qt-libdir=${QT_LIB_HOME} \
--with-qt-bindir=${QT_BIN_HOME} \
--with-qt-plugindir=${QT_PLUGIN_HOME} \
--with-protoc-bindir=${PROTOC_BIN_HOME} \
CPPFLAGS="-DQT_NODLL -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 \
 -I${DEPEND_HOME}/db/build_unix -I${DEPEND_HOME}/miniupnpc -I${DEPEND_HOME}/openssl/include -I${DEPEND_HOME}/boost \
 -I${DEPEND_HOME}/zlib -I${DEPEND_HOME}/libpng -I${DEPEND_HOME}/qrencode -I${DEPEND_HOME}/protobuf/src" \
LDFLAGS="-L${DEPEND_HOME}/boost/stage/lib -L${DEPEND_HOME}/qt/plugins/codecs \
-L${DEPEND_HOME}/qt/plugins/accessible -L${DEPEND_HOME}/db/build_unix \
-L${DEPEND_HOME}/miniupnpc -L${DEPEND_HOME}/openssl -L${DEPEND_HOME}/zlib \
-L${DEPEND_HOME}/libpng -L${DEPEND_HOME}/qrencode \
-L${DEPEND_HOME}/protobuf/src/.libs"
