#This project started in August 2020

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

ICON = my.icns
RC_FILE += myapp.rc

CONFIG += c++11

macx{
LIBS+= \
    libqscintilla2_qt5.15.0.0.dylib
}

win32{
LIBS += \
    qscintilla2_qt5.dll
#LIBS += \
#        ScintillaEdit5.dll
}

unix:!macx:{

    LIBS += -L$PWD libqscintilla2_qt5.so

    QMAKE_RPATHDIR=$ORIGIN
    QMAKE_LFLAGS += -no-pie
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN/lib\'"
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN/platforms\'"
}

TRANSLATIONS += en.ts\
                cn.ts


mac {
    # Only include / compile these files on OS X
    OBJECTIVE_SOURCES += \
        OSXHideTitleBar.mm
    HEADERS  +=\
        OSXHideTitleBar.h

    # Additionally include Cocoa for OS X code

    LIBS += -framework Foundation -framework Cocoa
    INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers
}


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MyTabBar.cpp \
    MyTabPage.cpp \
    MyTabPopup.cpp \
    MyTabWidget.cpp \
    autoupdatedialog.cpp \
    dlgdecompile.cpp \
    dlgpreferences.cpp \
    filesystemwatcher.cpp \
    main.cpp \
    mainwindow.cpp \
    methods.cpp \
    minidialog.cpp \
    myapp.cpp \
    recentfiles.cpp

HEADERS += \
    MyTabBar.h \
    MyTabPage.h \
    MyTabPopup.h \
    Qsci/qsciabstractapis.h \
    Qsci/qsciapis.h \
    Qsci/qscicommand.h \
    Qsci/qscicommandset.h \
    Qsci/qscidocument.h \
    Qsci/qsciglobal.h \
    Qsci/qscilexer.h \
    Qsci/qscilexeravs.h \
    Qsci/qscilexerbash.h \
    Qsci/qscilexerbatch.h \
    Qsci/qscilexercmake.h \
    Qsci/qscilexercoffeescript.h \
    Qsci/qscilexercpp.h \
    Qsci/qscilexercsharp.h \
    Qsci/qscilexercss.h \
    Qsci/qscilexercustom.h \
    Qsci/qscilexerd.h \
    Qsci/qscilexerdiff.h \
    Qsci/qscilexeredifact.h \
    Qsci/qscilexerfortran.h \
    Qsci/qscilexerfortran77.h \
    Qsci/qscilexerhtml.h \
    Qsci/qscilexeridl.h \
    Qsci/qscilexerjava.h \
    Qsci/qscilexerjavascript.h \
    Qsci/qscilexerjson.h \
    Qsci/qscilexerlua.h \
    Qsci/qscilexermakefile.h \
    Qsci/qscilexermarkdown.h \
    Qsci/qscilexermatlab.h \
    Qsci/qscilexeroctave.h \
    Qsci/qscilexerpascal.h \
    Qsci/qscilexerperl.h \
    Qsci/qscilexerpo.h \
    Qsci/qscilexerpostscript.h \
    Qsci/qscilexerpov.h \
    Qsci/qscilexerproperties.h \
    Qsci/qscilexerpython.h \
    Qsci/qscilexerruby.h \
    Qsci/qscilexerspice.h \
    Qsci/qscilexersql.h \
    Qsci/qscilexertcl.h \
    Qsci/qscilexertex.h \
    Qsci/qscilexerverilog.h \
    Qsci/qscilexervhdl.h \
    Qsci/qscilexerxml.h \
    Qsci/qscilexeryaml.h \
    Qsci/qscimacro.h \
    Qsci/qsciprinter.h \
    Qsci/qsciscintilla.h \
    Qsci/qsciscintillabase.h \
    Qsci/qscistyle.h \
    Qsci/qscistyledtext.h \
    autoupdatedialog.h \
    dlgdecompile.h \
    dlgpreferences.h \
    filesystemwatcher.h \
    mainwindow.h \
    methods.h \
    minidialog.h \
    myapp.h \
    mytabwidget.h \
    recentfiles.h

FORMS += \
    autoupdatedialog.ui \
    dlgdecompile.ui \
    dlgpreferences.ui \
    mainwindow.ui \
    minidialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Info.plist \
    icon/4.png \
    myapp.rc

RESOURCES += \
    my.qrc

CONFIG(debug,debug|release) {
    DESTDIR = $$absolute_path($${_PRO_FILE_PWD_}/bin/debug)
} else {
    DESTDIR = $$absolute_path($${_PRO_FILE_PWD_}/bin/release)
}
