QT += testlib core network widgets
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

INCLUDEPATH = $$PWD/../../../../../src $$PWD/../../../../../src/platform/macos


TEMPLATE = app

SOURCES += functor_test.cpp

LIBS += \
        -L$$OUT_PWD/../../ui                       -lui \
        -L$$OUT_PWD/../../rosetta/protocols_8      -lprotocols_8 \
        -L$$OUT_PWD/../../rosetta/protocols_7      -lprotocols_7 \
        -L$$OUT_PWD/../../rosetta/protocols_e_6    -lprotocols_e_6 \
        -L$$OUT_PWD/../../rosetta/protocols_d_6    -lprotocols_d_6 \
        -L$$OUT_PWD/../../rosetta/protocols_c_6    -lprotocols_c_6 \
        -L$$OUT_PWD/../../rosetta/protocols_b_6    -lprotocols_b_6 \
        -L$$OUT_PWD/../../rosetta/protocols_a_6    -lprotocols_a_6 \
        -L$$OUT_PWD/../../rosetta/protocols_h_5    -lprotocols_h_5 \
        -L$$OUT_PWD/../../rosetta/protocols_g_5    -lprotocols_g_5 \
        -L$$OUT_PWD/../../rosetta/protocols_f_5    -lprotocols_f_5 \
        -L$$OUT_PWD/../../rosetta/protocols_e_5    -lprotocols_e_5 \
        -L$$OUT_PWD/../../rosetta/protocols_d_5    -lprotocols_d_5 \
        -L$$OUT_PWD/../../rosetta/protocols_c_5    -lprotocols_c_5 \
        -L$$OUT_PWD/../../rosetta/protocols_b_5    -lprotocols_b_5 \
        -L$$OUT_PWD/../../rosetta/protocols_a_5    -lprotocols_a_5 \
        -L$$OUT_PWD/../../rosetta/protocols_4      -lprotocols_4 \
        -L$$OUT_PWD/../../rosetta/protocols_3      -lprotocols_3 \
        -L$$OUT_PWD/../../rosetta/protocols_b_2    -lprotocols_b_2 \
        -L$$OUT_PWD/../../rosetta/protocols_a_2    -lprotocols_a_2 \
        -L$$OUT_PWD/../../rosetta/protocols_1      -lprotocols_1 \
        -L$$OUT_PWD/../../rosetta/core_5           -lcore_5 \
        -L$$OUT_PWD/../../rosetta/core_4           -lcore_4 \
        -L$$OUT_PWD/../../rosetta/core_3           -lcore_3 \
        -L$$OUT_PWD/../../rosetta/core_2           -lcore_2 \
        -L$$OUT_PWD/../../rosetta/core_1           -lcore_1 \
        -L$$OUT_PWD/../../rosetta/basic            -lbasic \
        -L$$OUT_PWD/../../rosetta/numeric          -lnumeric \
        -L$$OUT_PWD/../../rosetta/utility          -lutility \
        -L$$OUT_PWD/../../rosetta/ObjexxFCL        -lObjexxFCL \
        -L$$OUT_PWD/../../rosetta/libxml2          -llibxml2 \
        -L$$OUT_PWD/../../rosetta/cifparse         -lcifparse \
        -L$$OUT_PWD/../../rosetta/external         -lexternal \
        -lz